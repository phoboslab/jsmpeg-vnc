# jsmpeg-vnc

The only way to play gta on your browser

[Download Binaries](https://github.com/seyedeliasfakoorian/jsmpeg-vnc/releases)


## Usage & Performance Considerations

```
jsmpeg-vnc.exe [options] <window name>

Options:
	-b bitrate in kilobit/s (default: estimated by output size)
	-s output size as WxH. E.g: -s 640x480 (default: same as window size)
	-f target framerate (default: 60)
	-p port (default: 8080)
	-c crop area in the captured window as X,Y,W,H. E.g.: -c 200,300,640,480
	-i enable/disable remote input. E.g. -i 0 (default: 1)

Use "desktop" as the window name to capture the whole Desktop. Use "cursor"
to capture the window at the current cursor position.

Example:
jsmpeg-vnc.exe -b 2000 -s 640x480 -f 30 -p 9006 "Quake 3: Arena"

To enable mouse lock in the browser (useful for games that require relative
mouse movements, not absolute ones), append "?mouselock" at the target URL
i.e: http://<server-ip>:8080/?mouselock
```	

For sharing the whole Desktop, Windows' Aero theme should be disabled as it slows down screen capture significantly. When serving a single window (e.g. games), Aero only has a marginal performance impact and can be left enabled.

Capturing and encoding 1920x1080 video narrowly amounts to 60fps on my system and occupies a whole CPU core. Capturing smaller windows significantly speeds up the process. Depending on your Wifi network quality you may also want to dial down the bitrate for large video sizes.

If Windows complains about a missing MSVCR100.dll, install the [Microsoft Visual C++ 2010 Redistributable Package](https://www.microsoft.com/en-us/download/details.aspx?id=5555).


## Technology & License

This App uses [ffmpeg](https://github.com/FFmpeg/FFmpeg) for encoding, [libwebsockets](https://github.com/warmcat/libwebsockets) for the WebSocket server and jsmpeg for decoding in the browser. Note that the jsmpeg version in this repository has been modified to get rid of an extra frame of latency. The server sends each frame with a custom header, so the resulting WebSocket stream is not a valid MPEG video anymore.

The client application (the thing that runs in the browser) is very rudimentary. In particular, the mobile version has some quirks with mouse input and only has touch buttons for the arrow keys, ESC and Enter, though this can be easily extended.

jsmpeg-vnc is published under the [GPLv3 License](http://www.gnu.org/licenses/gpl-3.0.en.html).
