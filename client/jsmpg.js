do (window) ->
  'use strict'
  # jsmpeg by Dominic Szablewski - phoboslab.org, github.com/phoboslab
  #
  # Consider this to be under MIT license. It's largely based an an Open Source
  # Decoder for Java under GPL, while I looked at another Decoder from Nokia 
  # (under no particular license?) for certain aspects.
  # I'm not sure if this work is "derivative" enough to have a different license
  # but then again, who still cares about MPEG1?
  #
  # Based on "Java MPEG-1 Video Decoder and Player" by Korandi Zoltan:
  # http://sourceforge.net/projects/javampeg1video/
  #
  # Inspired by "MPEG Decoder in Java ME" by Nokia:
  # http://www.developer.nokia.com/Community/Wiki/MPEG_decoder_in_Java_ME
  requestAnimFrame = do ->
    window.requestAnimationFrame or window.webkitRequestAnimationFrame or window.mozRequestAnimationFrame or (callback) ->
      window.setTimeout callback, 1000 / 60
      return
  jsmpeg = 
  window.jsmpeg = (url, opts) ->
    opts = opts or {}
    @benchmark = ! !opts.benchmark
    @canvas = opts.canvas or document.createElement('canvas')
    @autoplay = ! !opts.autoplay
    @loop = ! !opts.loop
    @seekable = ! !opts.seekable
    @externalLoadCallback = opts.onload or null
    @externalDecodeCallback = opts.ondecodeframe or null
    @externalFinishedCallback = opts.onfinished or null
    @customIntraQuantMatrix = new Uint8Array(64)
    @customNonIntraQuantMatrix = new Uint8Array(64)
    @blockData = new Int32Array(64)
    @zeroBlockData = new Int32Array(64)
    @fillArray @zeroBlockData, 0
    # use WebGL for YCbCrToRGBA conversion if possible (much faster)
    if !opts.forceCanvas2D and @initWebGL()
      @renderFrame = @renderFrameGL
    else
      @canvasContext = @canvas.getContext('2d')
      @renderFrame = @renderFrame2D
    if url instanceof WebSocket
      @client = url
      @client.onopen = @initSocketClient.bind(this)
    else
      @load url
    return

  # ----------------------------------------------------------------------------
  # Streaming over WebSockets
  jsmpeg::waitForIntraFrame = true
  jsmpeg::socketBufferSize = 512 * 1024
  # 512kb each

  jsmpeg::initSocketClient = (client) ->
    @buffer = new BitReader(new ArrayBuffer(@socketBufferSize))
    @buffer.writePos = 0
    @client.binaryType = 'arraybuffer'
    @client.onmessage = @receiveSocketMessage.bind(this)
    return

  jsmpeg::decodeSocketHeader = (data) ->
    # Custom header sent to all newly connected clients when streaming
    # over websockets:
    # struct { char magic[4] = "jsmp"; unsigned short width, height; };
    if data[0] == SOCKET_MAGIC_BYTES.charCodeAt(0) and data[1] == SOCKET_MAGIC_BYTES.charCodeAt(1) and data[2] == SOCKET_MAGIC_BYTES.charCodeAt(2) and data[3] == SOCKET_MAGIC_BYTES.charCodeAt(3)
      @width = data[4] * 256 + data[5]
      @height = data[6] * 256 + data[7]
      @initBuffers()
    return

  jsmpeg::currentPacketType = 0
  jsmpeg::currentPacketLength = 0

  jsmpeg::receiveSocketMessage = (event) ->
    messageData = new Uint8Array(event.data)
    if !@sequenceStarted
      @decodeSocketHeader messageData
      return
    if messageData.length >= 8 and messageData[0] == 0x00 and messageData[1] == 0x00 and messageData[2] == 0x01 and (messageData[3] == START_PACKET_VIDEO or messageData[3] == START_PACKET_AUDIO)
      @currentPacketType = messageData[3]
      @currentPacketLength = (messageData[4] << 24) + (messageData[5] << 16) + (messageData[6] << 8) + messageData[7]
    if @currentPacketType == START_PACKET_VIDEO
      @buffer.bytes.set messageData, @buffer.writePos
      @buffer.writePos += messageData.length
      if @buffer.writePos >= @currentPacketLength
        if @findStartCode(START_PICTURE) == BitReader.NOT_FOUND
          return
        @decodePicture()
        @buffer.index = 0
        @buffer.writePos = 0
    else if @currentPacketType == START_PACKET_AUDIO
    else
    return

  jsmpeg::scheduleDecoding = ->
    @decodePicture()
    @currentPictureDecoded = true
    return

  # ----------------------------------------------------------------------------
  # Recording from WebSockets
  jsmpeg::isRecording = false
  jsmpeg::recorderWaitForIntraFrame = false
  jsmpeg::recordedFrames = 0
  jsmpeg::recordedSize = 0
  jsmpeg::didStartRecordingCallback = null
  jsmpeg::recordBuffers = []

  jsmpeg::canRecord = ->
    @client and @client.readyState == @client.OPEN

  jsmpeg::startRecording = (callback) ->
    if !@canRecord()
      return
    # Discard old buffers and set for recording
    @discardRecordBuffers()
    @isRecording = true
    @recorderWaitForIntraFrame = true
    @didStartRecordingCallback = callback or null
    @recordedFrames = 0
    @recordedSize = 0
    # Fudge a simple Sequence Header for the MPEG file
    # 3 bytes width & height, 12 bits each
    wh1 = @width >> 4
    wh2 = (@width & 0xf) << 4 | @height >> 8
    wh3 = @height & 0xff
    @recordBuffers.push new Uint8Array([
      0x00
      0x00
      0x01
      0xb3
      wh1
      wh2
      wh3
      0x13
      0xff
      0xff
      0xe1
      0x58
      0x00
      0x00
      0x01
      0xb8
      0x00
      0x08
      0x00
    ])
    return

  jsmpeg::recordFrame = ->
    if !@isRecording
      return
    if @recorderWaitForIntraFrame
      # Not an intra frame? Exit.
      if @pictureCodingType != PICTURE_TYPE_I
        return
      # Start recording!
      @recorderWaitForIntraFrame = false
      if @didStartRecordingCallback
        @didStartRecordingCallback this
    # Copy the actual subrange for the current picture into a new Buffer
    @recordBuffers.push new Uint8Array(@buffer.bytes.subarray(8, @buffer.writePos))
    @recordedFrames++
    @recordedSize += @buffer.writePos
    return

  jsmpeg::discardRecordBuffers = ->
    @recordBuffers = []
    @recordedFrames = 0
    return

  jsmpeg::stopRecording = ->
    blob = new Blob(@recordBuffers, type: 'video/mpeg')
    @discardRecordBuffers()
    @isRecording = false
    blob

  # ----------------------------------------------------------------------------
  # Loading via Ajax
  jsmpeg::intraFrames = []
  jsmpeg::currentFrame = -1
  jsmpeg::currentTime = 0
  jsmpeg::frameCount = 0
  jsmpeg::duration = 0

  jsmpeg::load = (url) ->
    @url = url
    request = new XMLHttpRequest
    that = this

    request.onreadystatechange = ->
      if request.readyState == request.DONE and request.status == 200
        that.loadCallback request.response
      return

    request.onprogress = if @gl then @updateLoaderGL.bind(this) else @updateLoader2D.bind(this)
    request.open 'GET', url
    request.responseType = 'arraybuffer'
    request.send()
    return

  jsmpeg::updateLoader2D = (ev) ->
    p = ev.loaded / ev.total
    w = @canvas.width
    h = @canvas.height
    ctx = @canvasContext
    ctx.fillStyle = '#222'
    ctx.fillRect 0, 0, w, h
    ctx.fillStyle = '#fff'
    ctx.fillRect 0, h - (h * p), w, h * p
    return

  jsmpeg::updateLoaderGL = (ev) ->
    gl = @gl
    gl.uniform1f gl.getUniformLocation(@loadingProgram, 'loaded'), ev.loaded / ev.total
    gl.drawArrays gl.TRIANGLE_STRIP, 0, 4
    return

  jsmpeg::loadCallback = (file) ->
    @buffer = new BitReader(file)
    if @seekable
      @collectIntraFrames()
      @buffer.index = 0
    @findStartCode START_SEQUENCE
    @firstSequenceHeader = @buffer.index
    @decodeSequenceHeader()
    # Calculate the duration. This only works if the video is seekable and we have a frame count
    @duration = @frameCount / @pictureRate
    # Load the first frame
    @nextFrame()
    if @autoplay
      @play()
    if @externalLoadCallback
      @externalLoadCallback this
    return

  jsmpeg::collectIntraFrames = ->
    # Loop through the whole buffer and collect all intraFrames to build our seek index.
    # We also keep track of total frame count here
    frame = undefined
    frame = 0
    while @findStartCode(START_PICTURE) != BitReader.NOT_FOUND
      # Check if the found picture is an intra frame and remember the position
      @buffer.advance 10
      # skip temporalReference
      if @buffer.getBits(3) == PICTURE_TYPE_I
        # Remember index 13 bits back, before temporalReference and picture type
        @intraFrames.push
          frame: frame
          index: @buffer.index - 13
      frame++
    @frameCount = frame
    return

  jsmpeg::seekToFrame = (seekFrame, seekExact) ->
    if seekFrame < 0 or seekFrame >= @frameCount or !@intraFrames.length
      return false
    # Find the last intra frame before or equal to seek frame
    target = null
    i = 0
    while i < @intraFrames.length and @intraFrames[i].frame <= seekFrame
      target = @intraFrames[i]
      i++
    @buffer.index = target.index
    @currentFrame = target.frame - 1
    # If we're seeking to the exact frame, we may have to decode some more frames before
    # the one we want
    if seekExact
      frame = target.frame
      while frame < seekFrame
        @decodePicture DECODE_SKIP_OUTPUT
        @findStartCode START_PICTURE
        frame++
      @currentFrame = seekFrame - 1
    # Decode and display the picture we have seeked to
    @decodePicture()
    true

  jsmpeg::seekToTime = (time, seekExact) ->
    @seekToFrame time * @pictureRate | 0, seekExact
    return

  jsmpeg::play = (file) ->
    if @playing
      return
    @targetTime = @now()
    @playing = true
    @scheduleNextFrame()
    return

  jsmpeg::pause = (file) ->
    @playing = false
    return

  jsmpeg::stop = (file) ->
    @currentFrame = -1
    if @buffer
      @buffer.index = @firstSequenceHeader
    @playing = false
    if @client
      @client.close()
      @client = null
    return

  # ----------------------------------------------------------------------------
  # Utilities

  jsmpeg::readCode = (codeTable) ->
    state = 0
    loop
      state = codeTable[state + @buffer.getBits(1)]
      unless state >= 0 and codeTable[state] != 0
        break
    codeTable[state + 2]

  jsmpeg::findStartCode = (code) ->
    current = 0
    loop
      current = @buffer.findNextMPEGStartCode()
      if current == code or current == BitReader.NOT_FOUND
        return current
    BitReader.NOT_FOUND

  jsmpeg::fillArray = (a, value) ->
    i = 0
    length = a.length
    while i < length
      a[i] = value
      i++
    return

  # ----------------------------------------------------------------------------
  # Sequence Layer
  jsmpeg::pictureRate = 30
  jsmpeg::lateTime = 0
  jsmpeg::firstSequenceHeader = 0
  jsmpeg::targetTime = 0
  jsmpeg::benchmark = false
  jsmpeg::benchFrame = 0
  jsmpeg::benchDecodeTimes = 0
  jsmpeg::benchAvgFrameTime = 0

  jsmpeg::now = ->
    if window.performance then window.performance.now() else Date.now()

  jsmpeg::nextFrame = ->
    if !@buffer
      return
    frameStart = @now()
    loop
      code = @buffer.findNextMPEGStartCode()
      if code == START_SEQUENCE
        @decodeSequenceHeader()
      else if code == START_PICTURE
        if @playing
          @scheduleNextFrame()
        @decodePicture()
        @benchDecodeTimes += @now() - frameStart
        return @canvas
      else if code == BitReader.NOT_FOUND
        @stop()
        # Jump back to the beginning
        if @externalFinishedCallback
          @externalFinishedCallback this
        # Only loop if we found a sequence header
        if @loop and @sequenceStarted
          @play()
        return null
      else
        # ignore (GROUP, USER_DATA, EXTENSION, SLICES...)
    return

  jsmpeg::scheduleNextFrame = ->
    @lateTime = @now() - (@targetTime)
    wait = Math.max(0, 1000 / @pictureRate - (@lateTime))
    @targetTime = @now() + wait
    if @benchmark
      @benchFrame++
      if @benchFrame >= 120
        @benchAvgFrameTime = @benchDecodeTimes / @benchFrame
        @benchFrame = 0
        @benchDecodeTimes = 0
        if window.console
          console.log 'Average time per frame:', @benchAvgFrameTime, 'ms'
      setTimeout @nextFrame.bind(this), 0
    else if wait < 18
      @scheduleAnimation()
    else
      setTimeout @scheduleAnimation.bind(this), wait
    return

  jsmpeg::scheduleAnimation = ->
    requestAnimFrame @nextFrame.bind(this), @canvas
    return

  jsmpeg::decodeSequenceHeader = ->
    `var i`
    @width = @buffer.getBits(12)
    @height = @buffer.getBits(12)
    @buffer.advance 4
    # skip pixel aspect ratio
    @pictureRate = PICTURE_RATE[@buffer.getBits(4)]
    @buffer.advance 18 + 1 + 10 + 1
    # skip bitRate, marker, bufferSize and constrained bit
    @initBuffers()
    if @buffer.getBits(1)
      # load custom intra quant matrix?
      i = 0
      while i < 64
        @customIntraQuantMatrix[ZIG_ZAG[i]] = @buffer.getBits(8)
        i++
      @intraQuantMatrix = @customIntraQuantMatrix
    if @buffer.getBits(1)
      # load custom non intra quant matrix?
      i = 0
      while i < 64
        @customNonIntraQuantMatrix[ZIG_ZAG[i]] = @buffer.getBits(8)
        i++
      @nonIntraQuantMatrix = @customNonIntraQuantMatrix
    return

  jsmpeg::initBuffers = ->
    @intraQuantMatrix = DEFAULT_INTRA_QUANT_MATRIX
    @nonIntraQuantMatrix = DEFAULT_NON_INTRA_QUANT_MATRIX
    @mbWidth = @width + 15 >> 4
    @mbHeight = @height + 15 >> 4
    @mbSize = @mbWidth * @mbHeight
    @codedWidth = @mbWidth << 4
    @codedHeight = @mbHeight << 4
    @codedSize = @codedWidth * @codedHeight
    @halfWidth = @mbWidth << 3
    @halfHeight = @mbHeight << 3
    @quarterSize = @codedSize >> 2
    # Sequence already started? Don't allocate buffers again
    if @sequenceStarted
      return
    @sequenceStarted = true
    # Manually clamp values when writing macroblocks for shitty browsers
    # that don't support Uint8ClampedArray
    MaybeClampedUint8Array = window.Uint8ClampedArray or window.Uint8Array
    if !window.Uint8ClampedArray
      @copyBlockToDestination = @copyBlockToDestinationClamp
      @addBlockToDestination = @addBlockToDestinationClamp
    # Allocated buffers and resize the canvas
    @currentY = new MaybeClampedUint8Array(@codedSize)
    @currentY32 = new Uint32Array(@currentY.buffer)
    @currentCr = new MaybeClampedUint8Array(@codedSize >> 2)
    @currentCr32 = new Uint32Array(@currentCr.buffer)
    @currentCb = new MaybeClampedUint8Array(@codedSize >> 2)
    @currentCb32 = new Uint32Array(@currentCb.buffer)
    @forwardY = new MaybeClampedUint8Array(@codedSize)
    @forwardY32 = new Uint32Array(@forwardY.buffer)
    @forwardCr = new MaybeClampedUint8Array(@codedSize >> 2)
    @forwardCr32 = new Uint32Array(@forwardCr.buffer)
    @forwardCb = new MaybeClampedUint8Array(@codedSize >> 2)
    @forwardCb32 = new Uint32Array(@forwardCb.buffer)
    @canvas.width = @width
    @canvas.height = @height
    if @gl
      @gl.useProgram @program
      @gl.viewport 0, 0, @width, @height
    else
      @currentRGBA = @canvasContext.getImageData(0, 0, @width, @height)
      @fillArray @currentRGBA.data, 255
    return

  # ----------------------------------------------------------------------------
  # Picture Layer
  jsmpeg::currentY = null
  jsmpeg::currentCr = null
  jsmpeg::currentCb = null
  jsmpeg::currentRGBA = null
  jsmpeg::pictureCodingType = 0
  # Buffers for motion compensation
  jsmpeg::forwardY = null
  jsmpeg::forwardCr = null
  jsmpeg::forwardCb = null
  jsmpeg::fullPelForward = false
  jsmpeg::forwardFCode = 0
  jsmpeg::forwardRSize = 0
  jsmpeg::forwardF = 0

  jsmpeg::decodePicture = (skipOutput) ->
    pictureStart = @buffer.index - 32
    @currentFrame++
    @currentTime = @currentFrame / @pictureRate
    @buffer.advance 10
    # skip temporalReference
    @pictureCodingType = @buffer.getBits(3)
    @buffer.advance 16
    # skip vbv_delay
    # Skip B and D frames or unknown coding type
    if @pictureCodingType <= 0 or @pictureCodingType >= PICTURE_TYPE_B
      return
    # full_pel_forward, forward_f_code
    if @pictureCodingType == PICTURE_TYPE_P
      @fullPelForward = @buffer.getBits(1)
      @forwardFCode = @buffer.getBits(3)
      if @forwardFCode == 0
        # Ignore picture with zero forward_f_code
        return
      @forwardRSize = @forwardFCode - 1
      @forwardF = 1 << @forwardRSize
    code = 0
    loop
      code = @buffer.findNextMPEGStartCode()
      unless code == START_EXTENSION or code == START_USER_DATA
        break
    while code >= START_SLICE_FIRST and code <= START_SLICE_LAST
      @decodeSlice code & 0x000000FF
      code = @buffer.findNextMPEGStartCode()
    if code != BitReader.NOT_FOUND
      # We found the next start code; rewind 32bits and let the main loop handle it.
      @buffer.rewind 32
    # Record this frame, if the recorder wants it
    @recordFrame()
    if skipOutput != DECODE_SKIP_OUTPUT
      @renderFrame()
      if @externalDecodeCallback
        @externalDecodeCallback this, @canvas
    # If this is a reference picutre then rotate the prediction pointers
    if @pictureCodingType == PICTURE_TYPE_I or @pictureCodingType == PICTURE_TYPE_P
      tmpY = @forwardY
      tmpY32 = @forwardY32
      tmpCr = @forwardCr
      tmpCr32 = @forwardCr32
      tmpCb = @forwardCb
      tmpCb32 = @forwardCb32
      @forwardY = @currentY
      @forwardY32 = @currentY32
      @forwardCr = @currentCr
      @forwardCr32 = @currentCr32
      @forwardCb = @currentCb
      @forwardCb32 = @currentCb32
      @currentY = tmpY
      @currentY32 = tmpY32
      @currentCr = tmpCr
      @currentCr32 = tmpCr32
      @currentCb = tmpCb
      @currentCb32 = tmpCb32
    return

  jsmpeg::YCbCrToRGBA = ->
    pY = @currentY
    pCb = @currentCb
    pCr = @currentCr
    pRGBA = @currentRGBA.data
    # Chroma values are the same for each block of 4 pixels, so we proccess
    # 2 lines at a time, 2 neighboring pixels each.
    # I wish we could use 32bit writes to the RGBA buffer instead of writing
    # each byte separately, but we need the automatic clamping of the RGBA
    # buffer.
    yIndex1 = 0
    yIndex2 = @codedWidth
    yNext2Lines = @codedWidth + @codedWidth - (@width)
    cIndex = 0
    cNextLine = @halfWidth - (@width >> 1)
    rgbaIndex1 = 0
    rgbaIndex2 = @width * 4
    rgbaNext2Lines = @width * 4
    cols = @width >> 1
    rows = @height >> 1
    y = undefined
    cb = undefined
    cr = undefined
    r = undefined
    g = undefined
    b = undefined
    row = 0
    while row < rows
      col = 0
      while col < cols
        cb = pCb[cIndex]
        cr = pCr[cIndex]
        cIndex++
        r = cr + (cr * 103 >> 8) - 179
        g = (cb * 88 >> 8) - 44 + (cr * 183 >> 8) - 91
        b = cb + (cb * 198 >> 8) - 227
        # Line 1
        y1 = pY[yIndex1++]
        y2 = pY[yIndex1++]
        pRGBA[rgbaIndex1] = y1 + r
        pRGBA[rgbaIndex1 + 1] = y1 - g
        pRGBA[rgbaIndex1 + 2] = y1 + b
        pRGBA[rgbaIndex1 + 4] = y2 + r
        pRGBA[rgbaIndex1 + 5] = y2 - g
        pRGBA[rgbaIndex1 + 6] = y2 + b
        rgbaIndex1 += 8
        # Line 2
        y3 = pY[yIndex2++]
        y4 = pY[yIndex2++]
        pRGBA[rgbaIndex2] = y3 + r
        pRGBA[rgbaIndex2 + 1] = y3 - g
        pRGBA[rgbaIndex2 + 2] = y3 + b
        pRGBA[rgbaIndex2 + 4] = y4 + r
        pRGBA[rgbaIndex2 + 5] = y4 - g
        pRGBA[rgbaIndex2 + 6] = y4 + b
        rgbaIndex2 += 8
        col++
      yIndex1 += yNext2Lines
      yIndex2 += yNext2Lines
      rgbaIndex1 += rgbaNext2Lines
      rgbaIndex2 += rgbaNext2Lines
      cIndex += cNextLine
      row++
    return

  jsmpeg::renderFrame2D = ->
    @YCbCrToRGBA()
    @canvasContext.putImageData @currentRGBA, 0, 0
    return

  # ----------------------------------------------------------------------------
  # Accelerated WebGL YCbCrToRGBA conversion
  jsmpeg::gl = null
  jsmpeg::program = null
  jsmpeg::YTexture = null
  jsmpeg::CBTexture = null
  jsmpeg::CRTexture = null

  jsmpeg::createTexture = (index, name) ->
    gl = @gl
    texture = gl.createTexture()
    gl.bindTexture gl.TEXTURE_2D, texture
    gl.texParameteri gl.TEXTURE_2D, gl.TEXTURE_MAG_FILTER, gl.LINEAR
    gl.texParameteri gl.TEXTURE_2D, gl.TEXTURE_MIN_FILTER, gl.LINEAR
    gl.texParameteri gl.TEXTURE_2D, gl.TEXTURE_WRAP_S, gl.CLAMP_TO_EDGE
    gl.texParameteri gl.TEXTURE_2D, gl.TEXTURE_WRAP_T, gl.CLAMP_TO_EDGE
    gl.uniform1i gl.getUniformLocation(@program, name), index
    texture

  jsmpeg::compileShader = (type, source) ->
    gl = @gl
    shader = gl.createShader(type)
    gl.shaderSource shader, source
    gl.compileShader shader
    if !gl.getShaderParameter(shader, gl.COMPILE_STATUS)
      throw new Error(gl.getShaderInfoLog(shader))
    shader

  jsmpeg::initWebGL = ->
    # attempt to get a webgl context
    try
      gl = @gl = @canvas.getContext('webgl') or @canvas.getContext('experimental-webgl')
    catch e
      return false
    if !gl
      return false
    # init buffers
    @buffer = gl.createBuffer()
    gl.bindBuffer gl.ARRAY_BUFFER, @buffer
    gl.bufferData gl.ARRAY_BUFFER, new Float32Array([
      0
      0
      0
      1
      1
      0
      1
      1
    ]), gl.STATIC_DRAW
    # The main YCbCrToRGBA Shader
    @program = gl.createProgram()
    gl.attachShader @program, @compileShader(gl.VERTEX_SHADER, SHADER_VERTEX_IDENTITY)
    gl.attachShader @program, @compileShader(gl.FRAGMENT_SHADER, SHADER_FRAGMENT_YCBCRTORGBA)
    gl.linkProgram @program
    if !gl.getProgramParameter(@program, gl.LINK_STATUS)
      throw new Error(gl.getProgramInfoLog(@program))
    gl.useProgram @program
    # setup textures
    @YTexture = @createTexture(0, 'YTexture')
    @CBTexture = @createTexture(1, 'CBTexture')
    @CRTexture = @createTexture(2, 'CRTexture')
    vertexAttr = gl.getAttribLocation(@program, 'vertex')
    gl.enableVertexAttribArray vertexAttr
    gl.vertexAttribPointer vertexAttr, 2, gl.FLOAT, false, 0, 0
    # Shader for the loading screen
    @loadingProgram = gl.createProgram()
    gl.attachShader @loadingProgram, @compileShader(gl.VERTEX_SHADER, SHADER_VERTEX_IDENTITY)
    gl.attachShader @loadingProgram, @compileShader(gl.FRAGMENT_SHADER, SHADER_FRAGMENT_LOADING)
    gl.linkProgram @loadingProgram
    gl.useProgram @loadingProgram
    vertexAttr = gl.getAttribLocation(@loadingProgram, 'vertex')
    gl.enableVertexAttribArray vertexAttr
    gl.vertexAttribPointer vertexAttr, 2, gl.FLOAT, false, 0, 0
    true

  jsmpeg::renderFrameGL = ->
    gl = @gl
    # WebGL doesn't like Uint8ClampedArrays, so we have to create a Uint8Array view for 
    # each plane
    uint8Y = new Uint8Array(@currentY.buffer)
    uint8Cr = new Uint8Array(@currentCr.buffer)
    uint8Cb = new Uint8Array(@currentCb.buffer)
    gl.activeTexture gl.TEXTURE0
    gl.bindTexture gl.TEXTURE_2D, @YTexture
    gl.texImage2D gl.TEXTURE_2D, 0, gl.LUMINANCE, @codedWidth, @height, 0, gl.LUMINANCE, gl.UNSIGNED_BYTE, uint8Y
    gl.activeTexture gl.TEXTURE1
    gl.bindTexture gl.TEXTURE_2D, @CBTexture
    gl.texImage2D gl.TEXTURE_2D, 0, gl.LUMINANCE, @halfWidth, @height / 2, 0, gl.LUMINANCE, gl.UNSIGNED_BYTE, uint8Cr
    gl.activeTexture gl.TEXTURE2
    gl.bindTexture gl.TEXTURE_2D, @CRTexture
    gl.texImage2D gl.TEXTURE_2D, 0, gl.LUMINANCE, @halfWidth, @height / 2, 0, gl.LUMINANCE, gl.UNSIGNED_BYTE, uint8Cb
    gl.drawArrays gl.TRIANGLE_STRIP, 0, 4
    return

  # ----------------------------------------------------------------------------
  # Slice Layer
  jsmpeg::quantizerScale = 0
  jsmpeg::sliceBegin = false

  jsmpeg::decodeSlice = (slice) ->
    @sliceBegin = true
    @macroblockAddress = (slice - 1) * @mbWidth - 1
    # Reset motion vectors and DC predictors
    @motionFwH = @motionFwHPrev = 0
    @motionFwV = @motionFwVPrev = 0
    @dcPredictorY = 128
    @dcPredictorCr = 128
    @dcPredictorCb = 128
    @quantizerScale = @buffer.getBits(5)
    # skip extra bits
    while @buffer.getBits(1)
      @buffer.advance 8
    loop
      @decodeMacroblock()
      # We may have to ignore Video Stream Start Codes here (0xE0)!?
      unless !@buffer.nextBytesAreStartCode()
        break
    return

  # ----------------------------------------------------------------------------
  # Macroblock Layer
  jsmpeg::macroblockAddress = 0
  jsmpeg::mbRow = 0
  jsmpeg::mbCol = 0
  jsmpeg::macroblockType = 0
  jsmpeg::macroblockIntra = false
  jsmpeg::macroblockMotFw = false
  jsmpeg::motionFwH = 0
  jsmpeg::motionFwV = 0
  jsmpeg::motionFwHPrev = 0
  jsmpeg::motionFwVPrev = 0

  jsmpeg::decodeMacroblock = ->
    # Decode macroblock_address_increment
    increment = 0
    t = @readCode(MACROBLOCK_ADDRESS_INCREMENT)
    while t == 34
      # macroblock_stuffing
      t = @readCode(MACROBLOCK_ADDRESS_INCREMENT)
    while t == 35
      # macroblock_escape
      increment += 33
      t = @readCode(MACROBLOCK_ADDRESS_INCREMENT)
    increment += t
    # Process any skipped macroblocks
    if @sliceBegin
      # The first macroblock_address_increment of each slice is relative
      # to beginning of the preverious row, not the preverious macroblock
      @sliceBegin = false
      @macroblockAddress += increment
    else
      if @macroblockAddress + increment >= @mbSize
        # Illegal (too large) macroblock_address_increment
        return
      if increment > 1
        # Skipped macroblocks reset DC predictors
        @dcPredictorY = 128
        @dcPredictorCr = 128
        @dcPredictorCb = 128
        # Skipped macroblocks in P-pictures reset motion vectors
        if @pictureCodingType == PICTURE_TYPE_P
          @motionFwH = @motionFwHPrev = 0
          @motionFwV = @motionFwVPrev = 0
      # Predict skipped macroblocks
      while increment > 1
        @macroblockAddress++
        @mbRow = @macroblockAddress / @mbWidth | 0
        @mbCol = @macroblockAddress % @mbWidth
        @copyMacroblock @motionFwH, @motionFwV, @forwardY, @forwardCr, @forwardCb
        increment--
      @macroblockAddress++
    @mbRow = @macroblockAddress / @mbWidth | 0
    @mbCol = @macroblockAddress % @mbWidth
    # Process the current macroblock
    @macroblockType = @readCode(MACROBLOCK_TYPE_TABLES[@pictureCodingType])
    @macroblockIntra = @macroblockType & 0x01
    @macroblockMotFw = @macroblockType & 0x08
    # Quantizer scale
    if (@macroblockType & 0x10) != 0
      @quantizerScale = @buffer.getBits(5)
    if @macroblockIntra
      # Intra-coded macroblocks reset motion vectors
      @motionFwH = @motionFwHPrev = 0
      @motionFwV = @motionFwVPrev = 0
    else
      # Non-intra macroblocks reset DC predictors
      @dcPredictorY = 128
      @dcPredictorCr = 128
      @dcPredictorCb = 128
      @decodeMotionVectors()
      @copyMacroblock @motionFwH, @motionFwV, @forwardY, @forwardCr, @forwardCb
    # Decode blocks
    cbp = if (@macroblockType & 0x02) != 0 then @readCode(CODE_BLOCK_PATTERN) else if @macroblockIntra then 0x3f else 0
    block = 0
    mask = 0x20
    while block < 6
      if (cbp & mask) != 0
        @decodeBlock block
      mask >>= 1
      block++
    return

  jsmpeg::decodeMotionVectors = ->
    code = undefined
    d = undefined
    r = 0
    # Forward
    if @macroblockMotFw
      # Horizontal forward
      code = @readCode(MOTION)
      if code != 0 and @forwardF != 1
        r = @buffer.getBits(@forwardRSize)
        d = (Math.abs(code) - 1 << @forwardRSize) + r + 1
        if code < 0
          d = -d
      else
        d = code
      @motionFwHPrev += d
      if @motionFwHPrev > (@forwardF << 4) - 1
        @motionFwHPrev -= @forwardF << 5
      else if @motionFwHPrev < -@forwardF << 4
        @motionFwHPrev += @forwardF << 5
      @motionFwH = @motionFwHPrev
      if @fullPelForward
        @motionFwH <<= 1
      # Vertical forward
      code = @readCode(MOTION)
      if code != 0 and @forwardF != 1
        r = @buffer.getBits(@forwardRSize)
        d = (Math.abs(code) - 1 << @forwardRSize) + r + 1
        if code < 0
          d = -d
      else
        d = code
      @motionFwVPrev += d
      if @motionFwVPrev > (@forwardF << 4) - 1
        @motionFwVPrev -= @forwardF << 5
      else if @motionFwVPrev < -@forwardF << 4
        @motionFwVPrev += @forwardF << 5
      @motionFwV = @motionFwVPrev
      if @fullPelForward
        @motionFwV <<= 1
    else if @pictureCodingType == PICTURE_TYPE_P
      # No motion information in P-picture, reset vectors
      @motionFwH = @motionFwHPrev = 0
      @motionFwV = @motionFwVPrev = 0
    return

  jsmpeg::copyMacroblock = (motionH, motionV, sY, sCr, sCb) ->
    `var x`
    `var x`
    `var x`
    `var x`
    `var x`
    `var x`
    `var x`
    width = undefined
    scan = undefined
    H = undefined
    V = undefined
    oddH = undefined
    oddV = undefined
    src = undefined
    dest = undefined
    last = undefined
    # We use 32bit writes here
    dY = @currentY32
    dCb = @currentCb32
    dCr = @currentCr32
    # Luminance
    width = @codedWidth
    scan = width - 16
    H = motionH >> 1
    V = motionV >> 1
    oddH = (motionH & 1) == 1
    oddV = (motionV & 1) == 1
    src = ((@mbRow << 4) + V) * width + (@mbCol << 4) + H
    dest = @mbRow * width + @mbCol << 2
    last = dest + (width << 2)
    y1 = undefined
    y2 = undefined
    y = undefined
    if oddH
      if oddV
        while dest < last
          y1 = sY[src] + sY[src + width]
          src++
          x = 0
          while x < 4
            y2 = sY[src] + sY[src + width]
            src++
            y = y1 + y2 + 2 >> 2 & 0xff
            y1 = sY[src] + sY[src + width]
            src++
            y |= y1 + y2 + 2 << 6 & 0xff00
            y2 = sY[src] + sY[src + width]
            src++
            y |= y1 + y2 + 2 << 14 & 0xff0000
            y1 = sY[src] + sY[src + width]
            src++
            y |= y1 + y2 + 2 << 22 & 0xff000000
            dY[dest++] = y
            x++
          dest += scan >> 2
          src += scan - 1
      else
        while dest < last
          y1 = sY[src++]
          x = 0
          while x < 4
            y2 = sY[src++]
            y = y1 + y2 + 1 >> 1 & 0xff
            y1 = sY[src++]
            y |= y1 + y2 + 1 << 7 & 0xff00
            y2 = sY[src++]
            y |= y1 + y2 + 1 << 15 & 0xff0000
            y1 = sY[src++]
            y |= y1 + y2 + 1 << 23 & 0xff000000
            dY[dest++] = y
            x++
          dest += scan >> 2
          src += scan - 1
    else
      if oddV
        while dest < last
          x = 0
          while x < 4
            y = sY[src] + sY[src + width] + 1 >> 1 & 0xff
            src++
            y |= sY[src] + sY[src + width] + 1 << 7 & 0xff00
            src++
            y |= sY[src] + sY[src + width] + 1 << 15 & 0xff0000
            src++
            y |= sY[src] + sY[src + width] + 1 << 23 & 0xff000000
            src++
            dY[dest++] = y
            x++
          dest += scan >> 2
          src += scan
      else
        while dest < last
          x = 0
          while x < 4
            y = sY[src]
            src++
            y |= sY[src] << 8
            src++
            y |= sY[src] << 16
            src++
            y |= sY[src] << 24
            src++
            dY[dest++] = y
            x++
          dest += scan >> 2
          src += scan
    # Chrominance
    width = @halfWidth
    scan = width - 8
    H = motionH / 2 >> 1
    V = motionV / 2 >> 1
    oddH = (motionH / 2 & 1) == 1
    oddV = (motionV / 2 & 1) == 1
    src = ((@mbRow << 3) + V) * width + (@mbCol << 3) + H
    dest = @mbRow * width + @mbCol << 1
    last = dest + (width << 1)
    cr1 = undefined
    cr2 = undefined
    cr = undefined
    cb1 = undefined
    cb2 = undefined
    cb = undefined
    if oddH
      if oddV
        while dest < last
          cr1 = sCr[src] + sCr[src + width]
          cb1 = sCb[src] + sCb[src + width]
          src++
          x = 0
          while x < 2
            cr2 = sCr[src] + sCr[src + width]
            cb2 = sCb[src] + sCb[src + width]
            src++
            cr = cr1 + cr2 + 2 >> 2 & 0xff
            cb = cb1 + cb2 + 2 >> 2 & 0xff
            cr1 = sCr[src] + sCr[src + width]
            cb1 = sCb[src] + sCb[src + width]
            src++
            cr |= cr1 + cr2 + 2 << 6 & 0xff00
            cb |= cb1 + cb2 + 2 << 6 & 0xff00
            cr2 = sCr[src] + sCr[src + width]
            cb2 = sCb[src] + sCb[src + width]
            src++
            cr |= cr1 + cr2 + 2 << 14 & 0xff0000
            cb |= cb1 + cb2 + 2 << 14 & 0xff0000
            cr1 = sCr[src] + sCr[src + width]
            cb1 = sCb[src] + sCb[src + width]
            src++
            cr |= cr1 + cr2 + 2 << 22 & 0xff000000
            cb |= cb1 + cb2 + 2 << 22 & 0xff000000
            dCr[dest] = cr
            dCb[dest] = cb
            dest++
            x++
          dest += scan >> 2
          src += scan - 1
      else
        while dest < last
          cr1 = sCr[src]
          cb1 = sCb[src]
          src++
          x = 0
          while x < 2
            cr2 = sCr[src]
            cb2 = sCb[src++]
            cr = cr1 + cr2 + 1 >> 1 & 0xff
            cb = cb1 + cb2 + 1 >> 1 & 0xff
            cr1 = sCr[src]
            cb1 = sCb[src++]
            cr |= cr1 + cr2 + 1 << 7 & 0xff00
            cb |= cb1 + cb2 + 1 << 7 & 0xff00
            cr2 = sCr[src]
            cb2 = sCb[src++]
            cr |= cr1 + cr2 + 1 << 15 & 0xff0000
            cb |= cb1 + cb2 + 1 << 15 & 0xff0000
            cr1 = sCr[src]
            cb1 = sCb[src++]
            cr |= cr1 + cr2 + 1 << 23 & 0xff000000
            cb |= cb1 + cb2 + 1 << 23 & 0xff000000
            dCr[dest] = cr
            dCb[dest] = cb
            dest++
            x++
          dest += scan >> 2
          src += scan - 1
    else
      if oddV
        while dest < last
          x = 0
          while x < 2
            cr = sCr[src] + sCr[src + width] + 1 >> 1 & 0xff
            cb = sCb[src] + sCb[src + width] + 1 >> 1 & 0xff
            src++
            cr |= sCr[src] + sCr[src + width] + 1 << 7 & 0xff00
            cb |= sCb[src] + sCb[src + width] + 1 << 7 & 0xff00
            src++
            cr |= sCr[src] + sCr[src + width] + 1 << 15 & 0xff0000
            cb |= sCb[src] + sCb[src + width] + 1 << 15 & 0xff0000
            src++
            cr |= sCr[src] + sCr[src + width] + 1 << 23 & 0xff000000
            cb |= sCb[src] + sCb[src + width] + 1 << 23 & 0xff000000
            src++
            dCr[dest] = cr
            dCb[dest] = cb
            dest++
            x++
          dest += scan >> 2
          src += scan
      else
        while dest < last
          x = 0
          while x < 2
            cr = sCr[src]
            cb = sCb[src]
            src++
            cr |= sCr[src] << 8
            cb |= sCb[src] << 8
            src++
            cr |= sCr[src] << 16
            cb |= sCb[src] << 16
            src++
            cr |= sCr[src] << 24
            cb |= sCb[src] << 24
            src++
            dCr[dest] = cr
            dCb[dest] = cb
            dest++
            x++
          dest += scan >> 2
          src += scan
    return

  # ----------------------------------------------------------------------------
  # Block layer
  jsmpeg::dcPredictorY
  jsmpeg::dcPredictorCr
  jsmpeg::dcPredictorCb
  jsmpeg::blockData = null

  jsmpeg::decodeBlock = (block) ->
    n = 0
    quantMatrix = undefined
    # Decode DC coefficient of intra-coded blocks
    if @macroblockIntra
      predictor = undefined
      dctSize = undefined
      # DC prediction
      if block < 4
        predictor = @dcPredictorY
        dctSize = @readCode(DCT_DC_SIZE_LUMINANCE)
      else
        predictor = if block == 4 then @dcPredictorCr else @dcPredictorCb
        dctSize = @readCode(DCT_DC_SIZE_CHROMINANCE)
      # Read DC coeff
      if dctSize > 0
        differential = @buffer.getBits(dctSize)
        if (differential & 1 << dctSize - 1) != 0
          @blockData[0] = predictor + differential
        else
          @blockData[0] = predictor + (-1 << dctSize | differential + 1)
      else
        @blockData[0] = predictor
      # Save predictor value
      if block < 4
        @dcPredictorY = @blockData[0]
      else if block == 4
        @dcPredictorCr = @blockData[0]
      else
        @dcPredictorCb = @blockData[0]
      # Dequantize + premultiply
      @blockData[0] <<= 3 + 5
      quantMatrix = @intraQuantMatrix
      n = 1
    else
      quantMatrix = @nonIntraQuantMatrix
    # Decode AC coefficients (+DC for non-intra)
    level = 0
    loop
      run = 0
      coeff = @readCode(DCT_COEFF)
      if coeff == 0x0001 and n > 0 and @buffer.getBits(1) == 0
        # end_of_block
        break
      if coeff == 0xffff
        # escape
        run = @buffer.getBits(6)
        level = @buffer.getBits(8)
        if level == 0
          level = @buffer.getBits(8)
        else if level == 128
          level = @buffer.getBits(8) - 256
        else if level > 128
          level = level - 256
      else
        run = coeff >> 8
        level = coeff & 0xff
        if @buffer.getBits(1)
          level = -level
      n += run
      dezigZagged = ZIG_ZAG[n]
      n++
      # Dequantize, oddify, clip
      level <<= 1
      if !@macroblockIntra
        level += if level < 0 then -1 else 1
      level = level * @quantizerScale * quantMatrix[dezigZagged] >> 4
      if (level & 1) == 0
        level -= if level > 0 then 1 else -1
      if level > 2047
        level = 2047
      else if level < -2048
        level = -2048
      # Save premultiplied coefficient
      @blockData[dezigZagged] = level * PREMULTIPLIER_MATRIX[dezigZagged]
    # Move block to its place
    destArray = undefined
    destIndex = undefined
    scan = undefined
    if block < 4
      destArray = @currentY
      scan = @codedWidth - 8
      destIndex = @mbRow * @codedWidth + @mbCol << 4
      if (block & 1) != 0
        destIndex += 8
      if (block & 2) != 0
        destIndex += @codedWidth << 3
    else
      destArray = if block == 4 then @currentCb else @currentCr
      scan = (@codedWidth >> 1) - 8
      destIndex = (@mbRow * @codedWidth << 2) + (@mbCol << 3)
    if @macroblockIntra
      # Overwrite (no prediction)
      if n == 1
        @copyValueToDestination @blockData[0] + 128 >> 8, destArray, destIndex, scan
        @blockData[0] = 0
      else
        @IDCT()
        @copyBlockToDestination @blockData, destArray, destIndex, scan
        @blockData.set @zeroBlockData
    else
      # Add data to the predicted macroblock
      if n == 1
        @addValueToDestination @blockData[0] + 128 >> 8, destArray, destIndex, scan
        @blockData[0] = 0
      else
        @IDCT()
        @addBlockToDestination @blockData, destArray, destIndex, scan
        @blockData.set @zeroBlockData
    n = 0
    return

  jsmpeg::copyBlockToDestination = (blockData, destArray, destIndex, scan) ->
    n = 0
    while n < 64
      destArray[destIndex + 0] = blockData[n + 0]
      destArray[destIndex + 1] = blockData[n + 1]
      destArray[destIndex + 2] = blockData[n + 2]
      destArray[destIndex + 3] = blockData[n + 3]
      destArray[destIndex + 4] = blockData[n + 4]
      destArray[destIndex + 5] = blockData[n + 5]
      destArray[destIndex + 6] = blockData[n + 6]
      destArray[destIndex + 7] = blockData[n + 7]
      n += 8
      destIndex += scan + 8
    return

  jsmpeg::addBlockToDestination = (blockData, destArray, destIndex, scan) ->
    n = 0
    while n < 64
      destArray[destIndex + 0] += blockData[n + 0]
      destArray[destIndex + 1] += blockData[n + 1]
      destArray[destIndex + 2] += blockData[n + 2]
      destArray[destIndex + 3] += blockData[n + 3]
      destArray[destIndex + 4] += blockData[n + 4]
      destArray[destIndex + 5] += blockData[n + 5]
      destArray[destIndex + 6] += blockData[n + 6]
      destArray[destIndex + 7] += blockData[n + 7]
      n += 8
      destIndex += scan + 8
    return

  jsmpeg::copyValueToDestination = (value, destArray, destIndex, scan) ->
    n = 0
    while n < 64
      destArray[destIndex + 0] = value
      destArray[destIndex + 1] = value
      destArray[destIndex + 2] = value
      destArray[destIndex + 3] = value
      destArray[destIndex + 4] = value
      destArray[destIndex + 5] = value
      destArray[destIndex + 6] = value
      destArray[destIndex + 7] = value
      n += 8
      destIndex += scan + 8
    return

  jsmpeg::addValueToDestination = (value, destArray, destIndex, scan) ->
    n = 0
    while n < 64
      destArray[destIndex + 0] += value
      destArray[destIndex + 1] += value
      destArray[destIndex + 2] += value
      destArray[destIndex + 3] += value
      destArray[destIndex + 4] += value
      destArray[destIndex + 5] += value
      destArray[destIndex + 6] += value
      destArray[destIndex + 7] += value
      n += 8
      destIndex += scan + 8
    return

  # Clamping version for shitty browsers (IE) that don't support Uint8ClampedArray

  jsmpeg::copyBlockToDestinationClamp = (blockData, destArray, destIndex, scan) ->
    n = 0
    i = 0
    while i < 8
      j = 0
      while j < 8
        p = blockData[n++]
        destArray[destIndex++] = if p > 255 then 255 else if p < 0 then 0 else p
        j++
      destIndex += scan
      i++
    return

  jsmpeg::addBlockToDestinationClamp = (blockData, destArray, destIndex, scan) ->
    n = 0
    i = 0
    while i < 8
      j = 0
      while j < 8
        p = blockData[n++] + destArray[destIndex]
        destArray[destIndex++] = if p > 255 then 255 else if p < 0 then 0 else p
        j++
      destIndex += scan
      i++
    return

  jsmpeg::IDCT = ->
    # See http://vsr.informatik.tu-chemnitz.de/~jan/MPEG/HTML/IDCT.html
    # for more info.
    b1 = undefined
    b3 = undefined
    b4 = undefined
    b6 = undefined
    b7 = undefined
    tmp1 = undefined
    tmp2 = undefined
    m0 = undefined
    x0 = undefined
    x1 = undefined
    x2 = undefined
    x3 = undefined
    x4 = undefined
    y3 = undefined
    y4 = undefined
    y5 = undefined
    y6 = undefined
    y7 = undefined
    i = undefined
    blockData = @blockData
    # Transform columns
    i = 0
    while i < 8
      b1 = blockData[4 * 8 + i]
      b3 = blockData[2 * 8 + i] + blockData[6 * 8 + i]
      b4 = blockData[5 * 8 + i] - (blockData[3 * 8 + i])
      tmp1 = blockData[1 * 8 + i] + blockData[7 * 8 + i]
      tmp2 = blockData[3 * 8 + i] + blockData[5 * 8 + i]
      b6 = blockData[1 * 8 + i] - (blockData[7 * 8 + i])
      b7 = tmp1 + tmp2
      m0 = blockData[0 * 8 + i]
      x4 = (b6 * 473 - (b4 * 196) + 128 >> 8) - b7
      x0 = x4 - ((tmp1 - tmp2) * 362 + 128 >> 8)
      x1 = m0 - b1
      x2 = ((blockData[2 * 8 + i] - (blockData[6 * 8 + i])) * 362 + 128 >> 8) - b3
      x3 = m0 + b1
      y3 = x1 + x2
      y4 = x3 + b3
      y5 = x1 - x2
      y6 = x3 - b3
      y7 = -x0 - (b4 * 473 + b6 * 196 + 128 >> 8)
      blockData[0 * 8 + i] = b7 + y4
      blockData[1 * 8 + i] = x4 + y3
      blockData[2 * 8 + i] = y5 - x0
      blockData[3 * 8 + i] = y6 - y7
      blockData[4 * 8 + i] = y6 + y7
      blockData[5 * 8 + i] = x0 + y5
      blockData[6 * 8 + i] = y3 - x4
      blockData[7 * 8 + i] = y4 - b7
      ++i
    # Transform rows
    i = 0
    while i < 64
      b1 = blockData[4 + i]
      b3 = blockData[2 + i] + blockData[6 + i]
      b4 = blockData[5 + i] - (blockData[3 + i])
      tmp1 = blockData[1 + i] + blockData[7 + i]
      tmp2 = blockData[3 + i] + blockData[5 + i]
      b6 = blockData[1 + i] - (blockData[7 + i])
      b7 = tmp1 + tmp2
      m0 = blockData[0 + i]
      x4 = (b6 * 473 - (b4 * 196) + 128 >> 8) - b7
      x0 = x4 - ((tmp1 - tmp2) * 362 + 128 >> 8)
      x1 = m0 - b1
      x2 = ((blockData[2 + i] - (blockData[6 + i])) * 362 + 128 >> 8) - b3
      x3 = m0 + b1
      y3 = x1 + x2
      y4 = x3 + b3
      y5 = x1 - x2
      y6 = x3 - b3
      y7 = -x0 - (b4 * 473 + b6 * 196 + 128 >> 8)
      blockData[0 + i] = b7 + y4 + 128 >> 8
      blockData[1 + i] = x4 + y3 + 128 >> 8
      blockData[2 + i] = y5 - x0 + 128 >> 8
      blockData[3 + i] = y6 - y7 + 128 >> 8
      blockData[4 + i] = y6 + y7 + 128 >> 8
      blockData[5 + i] = x0 + y5 + 128 >> 8
      blockData[6 + i] = y3 - x4 + 128 >> 8
      blockData[7 + i] = y4 - b7 + 128 >> 8
      i += 8
    return

  # ----------------------------------------------------------------------------
  # VLC Tables and Constants
  SOCKET_MAGIC_BYTES = 'jsmp'
  DECODE_SKIP_OUTPUT = 1
  PICTURE_RATE = [
    0.000
    23.976
    24.000
    25.000
    29.970
    30.000
    50.000
    59.940
    60.000
    0.000
    0.000
    0.000
    0.000
    0.000
    0.000
    0.000
  ]
  ZIG_ZAG = new Uint8Array([
    0
    1
    8
    16
    9
    2
    3
    10
    17
    24
    32
    25
    18
    11
    4
    5
    12
    19
    26
    33
    40
    48
    41
    34
    27
    20
    13
    6
    7
    14
    21
    28
    35
    42
    49
    56
    57
    50
    43
    36
    29
    22
    15
    23
    30
    37
    44
    51
    58
    59
    52
    45
    38
    31
    39
    46
    53
    60
    61
    54
    47
    55
    62
    63
  ])
  DEFAULT_INTRA_QUANT_MATRIX = new Uint8Array([
    8
    16
    19
    22
    26
    27
    29
    34
    16
    16
    22
    24
    27
    29
    34
    37
    19
    22
    26
    27
    29
    34
    34
    38
    22
    22
    26
    27
    29
    34
    37
    40
    22
    26
    27
    29
    32
    35
    40
    48
    26
    27
    29
    32
    35
    40
    48
    58
    26
    27
    29
    34
    38
    46
    56
    69
    27
    29
    35
    38
    46
    56
    69
    83
  ])
  DEFAULT_NON_INTRA_QUANT_MATRIX = new Uint8Array([
    16
    16
    16
    16
    16
    16
    16
    16
    16
    16
    16
    16
    16
    16
    16
    16
    16
    16
    16
    16
    16
    16
    16
    16
    16
    16
    16
    16
    16
    16
    16
    16
    16
    16
    16
    16
    16
    16
    16
    16
    16
    16
    16
    16
    16
    16
    16
    16
    16
    16
    16
    16
    16
    16
    16
    16
    16
    16
    16
    16
    16
    16
    16
    16
  ])
  PREMULTIPLIER_MATRIX = new Uint8Array([
    32
    44
    42
    38
    32
    25
    17
    9
    44
    62
    58
    52
    44
    35
    24
    12
    42
    58
    55
    49
    42
    33
    23
    12
    38
    52
    49
    44
    38
    30
    20
    10
    32
    44
    42
    38
    32
    25
    17
    9
    25
    35
    33
    30
    25
    20
    14
    7
    17
    24
    23
    20
    17
    14
    9
    5
    9
    12
    12
    10
    9
    7
    5
    2
  ])
  MACROBLOCK_ADDRESS_INCREMENT = new Int16Array([
    1 * 3
    2 * 3
    0
    3 * 3
    4 * 3
    0
    0
    0
    1
    5 * 3
    6 * 3
    0
    7 * 3
    8 * 3
    0
    9 * 3
    10 * 3
    0
    11 * 3
    12 * 3
    0
    0
    0
    3
    0
    0
    2
    13 * 3
    14 * 3
    0
    15 * 3
    16 * 3
    0
    0
    0
    5
    0
    0
    4
    17 * 3
    18 * 3
    0
    19 * 3
    20 * 3
    0
    0
    0
    7
    0
    0
    6
    21 * 3
    22 * 3
    0
    23 * 3
    24 * 3
    0
    25 * 3
    26 * 3
    0
    27 * 3
    28 * 3
    0
    -1
    29 * 3
    0
    -1
    30 * 3
    0
    31 * 3
    32 * 3
    0
    33 * 3
    34 * 3
    0
    35 * 3
    36 * 3
    0
    37 * 3
    38 * 3
    0
    0
    0
    9
    0
    0
    8
    39 * 3
    40 * 3
    0
    41 * 3
    42 * 3
    0
    43 * 3
    44 * 3
    0
    45 * 3
    46 * 3
    0
    0
    0
    15
    0
    0
    14
    0
    0
    13
    0
    0
    12
    0
    0
    11
    0
    0
    10
    47 * 3
    -1
    0
    -1
    48 * 3
    0
    49 * 3
    50 * 3
    0
    51 * 3
    52 * 3
    0
    53 * 3
    54 * 3
    0
    55 * 3
    56 * 3
    0
    57 * 3
    58 * 3
    0
    59 * 3
    60 * 3
    0
    61 * 3
    -1
    0
    -1
    62 * 3
    0
    63 * 3
    64 * 3
    0
    65 * 3
    66 * 3
    0
    67 * 3
    68 * 3
    0
    69 * 3
    70 * 3
    0
    71 * 3
    72 * 3
    0
    73 * 3
    74 * 3
    0
    0
    0
    21
    0
    0
    20
    0
    0
    19
    0
    0
    18
    0
    0
    17
    0
    0
    16
    0
    0
    35
    0
    0
    34
    0
    0
    33
    0
    0
    32
    0
    0
    31
    0
    0
    30
    0
    0
    29
    0
    0
    28
    0
    0
    27
    0
    0
    26
    0
    0
    25
    0
    0
    24
    0
    0
    23
    0
    0
    22
  ])
  MACROBLOCK_TYPE_I = new Int8Array([
    1 * 3
    2 * 3
    0
    -1
    3 * 3
    0
    0
    0
    0x01
    0
    0
    0x11
  ])
  MACROBLOCK_TYPE_P = new Int8Array([
    1 * 3
    2 * 3
    0
    3 * 3
    4 * 3
    0
    0
    0
    0x0a
    5 * 3
    6 * 3
    0
    0
    0
    0x02
    7 * 3
    8 * 3
    0
    0
    0
    0x08
    9 * 3
    10 * 3
    0
    11 * 3
    12 * 3
    0
    -1
    13 * 3
    0
    0
    0
    0x12
    0
    0
    0x1a
    0
    0
    0x01
    0
    0
    0x11
  ])
  MACROBLOCK_TYPE_B = new Int8Array([
    1 * 3
    2 * 3
    0
    3 * 3
    5 * 3
    0
    4 * 3
    6 * 3
    0
    8 * 3
    7 * 3
    0
    0
    0
    0x0c
    9 * 3
    10 * 3
    0
    0
    0
    0x0e
    13 * 3
    14 * 3
    0
    12 * 3
    11 * 3
    0
    0
    0
    0x04
    0
    0
    0x06
    18 * 3
    16 * 3
    0
    15 * 3
    17 * 3
    0
    0
    0
    0x08
    0
    0
    0x0a
    -1
    19 * 3
    0
    0
    0
    0x01
    20 * 3
    21 * 3
    0
    0
    0
    0x1e
    0
    0
    0x11
    0
    0
    0x16
    0
    0
    0x1a
  ])
  CODE_BLOCK_PATTERN = new Int16Array([
    2 * 3
    1 * 3
    0
    3 * 3
    6 * 3
    0
    4 * 3
    5 * 3
    0
    8 * 3
    11 * 3
    0
    12 * 3
    13 * 3
    0
    9 * 3
    7 * 3
    0
    10 * 3
    14 * 3
    0
    20 * 3
    19 * 3
    0
    18 * 3
    16 * 3
    0
    23 * 3
    17 * 3
    0
    27 * 3
    25 * 3
    0
    21 * 3
    28 * 3
    0
    15 * 3
    22 * 3
    0
    24 * 3
    26 * 3
    0
    0
    0
    60
    35 * 3
    40 * 3
    0
    44 * 3
    48 * 3
    0
    38 * 3
    36 * 3
    0
    42 * 3
    47 * 3
    0
    29 * 3
    31 * 3
    0
    39 * 3
    32 * 3
    0
    0
    0
    32
    45 * 3
    46 * 3
    0
    33 * 3
    41 * 3
    0
    43 * 3
    34 * 3
    0
    0
    0
    4
    30 * 3
    37 * 3
    0
    0
    0
    8
    0
    0
    16
    0
    0
    44
    50 * 3
    56 * 3
    0
    0
    0
    28
    0
    0
    52
    0
    0
    62
    61 * 3
    59 * 3
    0
    52 * 3
    60 * 3
    0
    0
    0
    1
    55 * 3
    54 * 3
    0
    0
    0
    61
    0
    0
    56
    57 * 3
    58 * 3
    0
    0
    0
    2
    0
    0
    40
    51 * 3
    62 * 3
    0
    0
    0
    48
    64 * 3
    63 * 3
    0
    49 * 3
    53 * 3
    0
    0
    0
    20
    0
    0
    12
    80 * 3
    83 * 3
    0
    0
    0
    63
    77 * 3
    75 * 3
    0
    65 * 3
    73 * 3
    0
    84 * 3
    66 * 3
    0
    0
    0
    24
    0
    0
    36
    0
    0
    3
    69 * 3
    87 * 3
    0
    81 * 3
    79 * 3
    0
    68 * 3
    71 * 3
    0
    70 * 3
    78 * 3
    0
    67 * 3
    76 * 3
    0
    72 * 3
    74 * 3
    0
    86 * 3
    85 * 3
    0
    88 * 3
    82 * 3
    0
    -1
    94 * 3
    0
    95 * 3
    97 * 3
    0
    0
    0
    33
    0
    0
    9
    106 * 3
    110 * 3
    0
    102 * 3
    116 * 3
    0
    0
    0
    5
    0
    0
    10
    93 * 3
    89 * 3
    0
    0
    0
    6
    0
    0
    18
    0
    0
    17
    0
    0
    34
    113 * 3
    119 * 3
    0
    103 * 3
    104 * 3
    0
    90 * 3
    92 * 3
    0
    109 * 3
    107 * 3
    0
    117 * 3
    118 * 3
    0
    101 * 3
    99 * 3
    0
    98 * 3
    96 * 3
    0
    100 * 3
    91 * 3
    0
    114 * 3
    115 * 3
    0
    105 * 3
    108 * 3
    0
    112 * 3
    111 * 3
    0
    121 * 3
    125 * 3
    0
    0
    0
    41
    0
    0
    14
    0
    0
    21
    124 * 3
    122 * 3
    0
    120 * 3
    123 * 3
    0
    0
    0
    11
    0
    0
    19
    0
    0
    7
    0
    0
    35
    0
    0
    13
    0
    0
    50
    0
    0
    49
    0
    0
    58
    0
    0
    37
    0
    0
    25
    0
    0
    45
    0
    0
    57
    0
    0
    26
    0
    0
    29
    0
    0
    38
    0
    0
    53
    0
    0
    23
    0
    0
    43
    0
    0
    46
    0
    0
    42
    0
    0
    22
    0
    0
    54
    0
    0
    51
    0
    0
    15
    0
    0
    30
    0
    0
    39
    0
    0
    47
    0
    0
    55
    0
    0
    27
    0
    0
    59
    0
    0
    31
  ])
  MOTION = new Int16Array([
    1 * 3
    2 * 3
    0
    4 * 3
    3 * 3
    0
    0
    0
    0
    6 * 3
    5 * 3
    0
    8 * 3
    7 * 3
    0
    0
    0
    -1
    0
    0
    1
    9 * 3
    10 * 3
    0
    12 * 3
    11 * 3
    0
    0
    0
    2
    0
    0
    -2
    14 * 3
    15 * 3
    0
    16 * 3
    13 * 3
    0
    20 * 3
    18 * 3
    0
    0
    0
    3
    0
    0
    -3
    17 * 3
    19 * 3
    0
    -1
    23 * 3
    0
    27 * 3
    25 * 3
    0
    26 * 3
    21 * 3
    0
    24 * 3
    22 * 3
    0
    32 * 3
    28 * 3
    0
    29 * 3
    31 * 3
    0
    -1
    33 * 3
    0
    36 * 3
    35 * 3
    0
    0
    0
    -4
    30 * 3
    34 * 3
    0
    0
    0
    4
    0
    0
    -7
    0
    0
    5
    37 * 3
    41 * 3
    0
    0
    0
    -5
    0
    0
    7
    38 * 3
    40 * 3
    0
    42 * 3
    39 * 3
    0
    0
    0
    -6
    0
    0
    6
    51 * 3
    54 * 3
    0
    50 * 3
    49 * 3
    0
    45 * 3
    46 * 3
    0
    52 * 3
    47 * 3
    0
    43 * 3
    53 * 3
    0
    44 * 3
    48 * 3
    0
    0
    0
    10
    0
    0
    9
    0
    0
    8
    0
    0
    -8
    57 * 3
    66 * 3
    0
    0
    0
    -9
    60 * 3
    64 * 3
    0
    56 * 3
    61 * 3
    0
    55 * 3
    62 * 3
    0
    58 * 3
    63 * 3
    0
    0
    0
    -10
    59 * 3
    65 * 3
    0
    0
    0
    12
    0
    0
    16
    0
    0
    13
    0
    0
    14
    0
    0
    11
    0
    0
    15
    0
    0
    -16
    0
    0
    -12
    0
    0
    -14
    0
    0
    -15
    0
    0
    -11
    0
    0
    -13
  ])
  DCT_DC_SIZE_LUMINANCE = new Int8Array([
    2 * 3
    1 * 3
    0
    6 * 3
    5 * 3
    0
    3 * 3
    4 * 3
    0
    0
    0
    1
    0
    0
    2
    9 * 3
    8 * 3
    0
    7 * 3
    10 * 3
    0
    0
    0
    0
    12 * 3
    11 * 3
    0
    0
    0
    4
    0
    0
    3
    13 * 3
    14 * 3
    0
    0
    0
    5
    0
    0
    6
    16 * 3
    15 * 3
    0
    17 * 3
    -1
    0
    0
    0
    7
    0
    0
    8
  ])
  DCT_DC_SIZE_CHROMINANCE = new Int8Array([
    2 * 3
    1 * 3
    0
    4 * 3
    3 * 3
    0
    6 * 3
    5 * 3
    0
    8 * 3
    7 * 3
    0
    0
    0
    2
    0
    0
    1
    0
    0
    0
    10 * 3
    9 * 3
    0
    0
    0
    3
    12 * 3
    11 * 3
    0
    0
    0
    4
    14 * 3
    13 * 3
    0
    0
    0
    5
    16 * 3
    15 * 3
    0
    0
    0
    6
    17 * 3
    -1
    0
    0
    0
    7
    0
    0
    8
  ])
  DCT_COEFF = new Int32Array([
    1 * 3
    2 * 3
    0
    4 * 3
    3 * 3
    0
    0
    0
    0x0001
    7 * 3
    8 * 3
    0
    6 * 3
    5 * 3
    0
    13 * 3
    9 * 3
    0
    11 * 3
    10 * 3
    0
    14 * 3
    12 * 3
    0
    0
    0
    0x0101
    20 * 3
    22 * 3
    0
    18 * 3
    21 * 3
    0
    16 * 3
    19 * 3
    0
    0
    0
    0x0201
    17 * 3
    15 * 3
    0
    0
    0
    0x0002
    0
    0
    0x0003
    27 * 3
    25 * 3
    0
    29 * 3
    31 * 3
    0
    24 * 3
    26 * 3
    0
    32 * 3
    30 * 3
    0
    0
    0
    0x0401
    23 * 3
    28 * 3
    0
    0
    0
    0x0301
    0
    0
    0x0102
    0
    0
    0x0701
    0
    0
    0xffff
    0
    0
    0x0601
    37 * 3
    36 * 3
    0
    0
    0
    0x0501
    35 * 3
    34 * 3
    0
    39 * 3
    38 * 3
    0
    33 * 3
    42 * 3
    0
    40 * 3
    41 * 3
    0
    52 * 3
    50 * 3
    0
    54 * 3
    53 * 3
    0
    48 * 3
    49 * 3
    0
    43 * 3
    45 * 3
    0
    46 * 3
    44 * 3
    0
    0
    0
    0x0801
    0
    0
    0x0004
    0
    0
    0x0202
    0
    0
    0x0901
    51 * 3
    47 * 3
    0
    55 * 3
    57 * 3
    0
    60 * 3
    56 * 3
    0
    59 * 3
    58 * 3
    0
    61 * 3
    62 * 3
    0
    0
    0
    0x0a01
    0
    0
    0x0d01
    0
    0
    0x0006
    0
    0
    0x0103
    0
    0
    0x0005
    0
    0
    0x0302
    0
    0
    0x0b01
    0
    0
    0x0c01
    76 * 3
    75 * 3
    0
    67 * 3
    70 * 3
    0
    73 * 3
    71 * 3
    0
    78 * 3
    74 * 3
    0
    72 * 3
    77 * 3
    0
    69 * 3
    64 * 3
    0
    68 * 3
    63 * 3
    0
    66 * 3
    65 * 3
    0
    81 * 3
    87 * 3
    0
    91 * 3
    80 * 3
    0
    82 * 3
    79 * 3
    0
    83 * 3
    86 * 3
    0
    93 * 3
    92 * 3
    0
    84 * 3
    85 * 3
    0
    90 * 3
    94 * 3
    0
    88 * 3
    89 * 3
    0
    0
    0
    0x0203
    0
    0
    0x0104
    0
    0
    0x0007
    0
    0
    0x0402
    0
    0
    0x0502
    0
    0
    0x1001
    0
    0
    0x0f01
    0
    0
    0x0e01
    105 * 3
    107 * 3
    0
    111 * 3
    114 * 3
    0
    104 * 3
    97 * 3
    0
    125 * 3
    119 * 3
    0
    96 * 3
    98 * 3
    0
    -1
    123 * 3
    0
    95 * 3
    101 * 3
    0
    106 * 3
    121 * 3
    0
    99 * 3
    102 * 3
    0
    113 * 3
    103 * 3
    0
    112 * 3
    116 * 3
    0
    110 * 3
    100 * 3
    0
    124 * 3
    115 * 3
    0
    117 * 3
    122 * 3
    0
    109 * 3
    118 * 3
    0
    120 * 3
    108 * 3
    0
    127 * 3
    136 * 3
    0
    139 * 3
    140 * 3
    0
    130 * 3
    126 * 3
    0
    145 * 3
    146 * 3
    0
    128 * 3
    129 * 3
    0
    0
    0
    0x0802
    132 * 3
    134 * 3
    0
    155 * 3
    154 * 3
    0
    0
    0
    0x0008
    137 * 3
    133 * 3
    0
    143 * 3
    144 * 3
    0
    151 * 3
    138 * 3
    0
    142 * 3
    141 * 3
    0
    0
    0
    0x000a
    0
    0
    0x0009
    0
    0
    0x000b
    0
    0
    0x1501
    0
    0
    0x0602
    0
    0
    0x0303
    0
    0
    0x1401
    0
    0
    0x0702
    0
    0
    0x1101
    0
    0
    0x1201
    0
    0
    0x1301
    148 * 3
    152 * 3
    0
    0
    0
    0x0403
    153 * 3
    150 * 3
    0
    0
    0
    0x0105
    131 * 3
    135 * 3
    0
    0
    0
    0x0204
    149 * 3
    147 * 3
    0
    172 * 3
    173 * 3
    0
    162 * 3
    158 * 3
    0
    170 * 3
    161 * 3
    0
    168 * 3
    166 * 3
    0
    157 * 3
    179 * 3
    0
    169 * 3
    167 * 3
    0
    174 * 3
    171 * 3
    0
    178 * 3
    177 * 3
    0
    156 * 3
    159 * 3
    0
    164 * 3
    165 * 3
    0
    183 * 3
    182 * 3
    0
    175 * 3
    176 * 3
    0
    0
    0
    0x0107
    0
    0
    0x0a02
    0
    0
    0x0902
    0
    0
    0x1601
    0
    0
    0x1701
    0
    0
    0x1901
    0
    0
    0x1801
    0
    0
    0x0503
    0
    0
    0x0304
    0
    0
    0x000d
    0
    0
    0x000c
    0
    0
    0x000e
    0
    0
    0x000f
    0
    0
    0x0205
    0
    0
    0x1a01
    0
    0
    0x0106
    180 * 3
    181 * 3
    0
    160 * 3
    163 * 3
    0
    196 * 3
    199 * 3
    0
    0
    0
    0x001b
    203 * 3
    185 * 3
    0
    202 * 3
    201 * 3
    0
    0
    0
    0x0013
    0
    0
    0x0016
    197 * 3
    207 * 3
    0
    0
    0
    0x0012
    191 * 3
    192 * 3
    0
    188 * 3
    190 * 3
    0
    0
    0
    0x0014
    184 * 3
    194 * 3
    0
    0
    0
    0x0015
    186 * 3
    193 * 3
    0
    0
    0
    0x0017
    204 * 3
    198 * 3
    0
    0
    0
    0x0019
    0
    0
    0x0018
    200 * 3
    205 * 3
    0
    0
    0
    0x001f
    0
    0
    0x001e
    0
    0
    0x001c
    0
    0
    0x001d
    0
    0
    0x001a
    0
    0
    0x0011
    0
    0
    0x0010
    189 * 3
    206 * 3
    0
    187 * 3
    195 * 3
    0
    218 * 3
    211 * 3
    0
    0
    0
    0x0025
    215 * 3
    216 * 3
    0
    0
    0
    0x0024
    210 * 3
    212 * 3
    0
    0
    0
    0x0022
    213 * 3
    209 * 3
    0
    221 * 3
    222 * 3
    0
    219 * 3
    208 * 3
    0
    217 * 3
    214 * 3
    0
    223 * 3
    220 * 3
    0
    0
    0
    0x0023
    0
    0
    0x010b
    0
    0
    0x0028
    0
    0
    0x010c
    0
    0
    0x010a
    0
    0
    0x0020
    0
    0
    0x0108
    0
    0
    0x0109
    0
    0
    0x0026
    0
    0
    0x010d
    0
    0
    0x010e
    0
    0
    0x0021
    0
    0
    0x0027
    0
    0
    0x1f01
    0
    0
    0x1b01
    0
    0
    0x1e01
    0
    0
    0x1002
    0
    0
    0x1d01
    0
    0
    0x1c01
    0
    0
    0x010f
    0
    0
    0x0112
    0
    0
    0x0111
    0
    0
    0x0110
    0
    0
    0x0603
    0
    0
    0x0b02
    0
    0
    0x0e02
    0
    0
    0x0d02
    0
    0
    0x0c02
    0
    0
    0x0f02
  ])
  PICTURE_TYPE_I = 1
  PICTURE_TYPE_P = 2
  PICTURE_TYPE_B = 3
  PICTURE_TYPE_D = 4
  START_SEQUENCE = 0xB3
  START_SLICE_FIRST = 0x01
  START_SLICE_LAST = 0xAF
  START_PICTURE = 0x00
  START_EXTENSION = 0xB5
  START_USER_DATA = 0xB2
  START_PACKET_VIDEO = 0xFA
  START_PACKET_AUDIO = 0xFB
  SHADER_FRAGMENT_YCBCRTORGBA = [
    'precision mediump float;'
    'uniform sampler2D YTexture;'
    'uniform sampler2D CBTexture;'
    'uniform sampler2D CRTexture;'
    'varying vec2 texCoord;'
    'void main() {'
    'float y = texture2D(YTexture, texCoord).r;'
    'float cr = texture2D(CBTexture, texCoord).r - 0.5;'
    'float cb = texture2D(CRTexture, texCoord).r - 0.5;'
    'gl_FragColor = vec4('
    'y + 1.4 * cr,'
    'y + -0.343 * cb - 0.711 * cr,'
    'y + 1.765 * cb,'
    '1.0'
    ');'
    '}'
  ].join('\n')
  SHADER_FRAGMENT_LOADING = [
    'precision mediump float;'
    'uniform float loaded;'
    'varying vec2 texCoord;'
    'void main() {'
    'float c = ceil(loaded-(1.0-texCoord.y));'
    'gl_FragColor = vec4(c,c,c,1);'
    '}'
  ].join('\n')
  SHADER_VERTEX_IDENTITY = [
    'attribute vec2 vertex;'
    'varying vec2 texCoord;'
    'void main() {'
    'texCoord = vertex;'
    'gl_Position = vec4((vertex * 2.0 - 1.0) * vec2(1, -1), 0.0, 1.0);'
    '}'
  ].join('\n')
  MACROBLOCK_TYPE_TABLES = [
    null
    MACROBLOCK_TYPE_I
    MACROBLOCK_TYPE_P
    MACROBLOCK_TYPE_B
  ]
  # ----------------------------------------------------------------------------
  # Bit Reader 

  BitReader = (arrayBuffer) ->
    @bytes = if arrayBuffer instanceof Uint8Array then arrayBuffer else new Uint8Array(arrayBuffer)
    @length = @bytes.length
    @writePos = @bytes.length
    @index = 0
    return

  BitReader.NOT_FOUND = -1

  BitReader::findNextMPEGStartCode = ->
    i = @index + 7 >> 3
    while i < @writePos
      if @bytes[i] == 0x00 and @bytes[i + 1] == 0x00 and @bytes[i + 2] == 0x01
        @index = i + 4 << 3
        return @bytes[i + 3]
      i++
    @index = @writePos << 3
    BitReader.NOT_FOUND

  BitReader::nextBytesAreStartCode = ->
    i = @index + 7 >> 3
    i >= @writePos or @bytes[i] == 0x00 and @bytes[i + 1] == 0x00 and @bytes[i + 2] == 0x01

  BitReader::nextBits = (count) ->
    byteOffset = @index >> 3
    room = 8 - (@index % 8)
    if room >= count
      return @bytes[byteOffset] >> room - count & 0xff >> 8 - count
    leftover = (@index + count) % 8
    end = @index + count - 1 >> 3
    value = @bytes[byteOffset] & 0xff >> 8 - room
    # Fill out first byte
    byteOffset++
    while byteOffset < end
      value <<= 8
      # Shift and
      value |= @bytes[byteOffset]
      # Put next byte
      byteOffset++
    if leftover > 0
      value <<= leftover
      # Make room for remaining bits
      value |= @bytes[byteOffset] >> 8 - leftover
    else
      value <<= 8
      value |= @bytes[byteOffset]
    value

  BitReader::peek = BitReader::nextBits

  BitReader::getBits = (count) ->
    value = @nextBits(count)
    @index += count
    value

  BitReader::read = BitReader::getBits

  BitReader::align = ->
    @index = ((@index + 7) / 8 | 0) * 8
    return

  BitReader::advance = (count) ->
    @index += count

  BitReader::rewind = (count) ->
    @index -= count

  window.BitReader = BitReader
  return

# ---
# generated by js2coffee 2.2.0