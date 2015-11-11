#include <stdio.h>
#include <stdlib.h>


#include "encoder.h"
#include "rgb2yuv.h"


#ifdef USE_FFMPEG
#pragma comment(lib, "avcodec.lib")
#pragma comment(lib, "avutil.lib")
#pragma comment(lib, "swscale.lib")

encoder_t *encoder_create(int in_width, int in_height, int out_width, int out_height, int bitrate) {
	encoder_t *self = (encoder_t *)malloc(sizeof(encoder_t));
	memset(self, 0, sizeof(encoder_t));

	self->in_width = in_width;
	self->in_height = in_height;
	self->out_width = out_width;
	self->out_height = out_height;
	
	avcodec_register_all();
	self->codec = avcodec_find_encoder(AV_CODEC_ID_MPEG1VIDEO);
	
	self->context = avcodec_alloc_context3(self->codec);
	self->context->dct_algo = FF_DCT_FASTINT;
	self->context->bit_rate = bitrate;
	self->context->width = out_width;
	self->context->height = out_height;
	self->context->time_base.num = 1;
	self->context->time_base.den = 30;
	self->context->gop_size = 30;
	self->context->max_b_frames = 0;
	self->context->pix_fmt = PIX_FMT_YUV420P;
	
	avcodec_open2(self->context, self->codec, NULL);
	
	self->frame = avcodec_alloc_frame();
	self->frame->format = PIX_FMT_YUV420P;
	self->frame->width  = out_width;
	self->frame->height = out_height;
	self->frame->pts = 0;
	
	int frame_size = avpicture_get_size(PIX_FMT_YUV420P, out_width, out_height);
	self->frame_buffer = malloc(frame_size);
	avpicture_fill((AVPicture*)self->frame, (uint8_t*)self->frame_buffer, PIX_FMT_YUV420P, out_width, out_height);
	
	self->sws = sws_getContext(
		in_width, in_height, AV_PIX_FMT_RGB32,
		out_width, out_height, AV_PIX_FMT_YUV420P,
		SWS_FAST_BILINEAR, 0, 0, 0
	);
	
	return self;
}

void encoder_destroy(encoder_t *self) {
	if( self == NULL ) { return; }

	sws_freeContext(self->sws);
	avcodec_close(self->context);
	av_free(self->context);	
	av_free(self->frame);
	free(self->frame_buffer);
	free(self);
}

void encoder_encode(encoder_t *self, void *rgb_pixels, void *encoded_data, size_t *encoded_size) {
	uint8_t *in_data[1] = {(uint8_t *)rgb_pixels};
	int in_linesize[1] = {self->in_width * 4};
	sws_scale(self->sws, in_data, in_linesize, 0, self->in_height, self->frame->data, self->frame->linesize);
		
	int available_size = *encoded_size;
	*encoded_size = 0;
	self->frame->pts++;
	
	av_init_packet(&self->packet);
	int success = 0;
	avcodec_encode_video2(self->context, &self->packet, self->frame, &success);
	if( success ) {
		if( self->packet.size <= available_size ) {
			memcpy(encoded_data, self->packet.data, self->packet.size);
			*encoded_size = self->packet.size;
		}
		else {
			printf("Frame too large for buffer (size: %d needed: %d)\n", available_size, self->packet.size);
		}
	}
	av_free_packet(&self->packet);
}
#else

#include <memory.h>

#define DEFAULT_BUFFER_SIZE 1024*1024

#define CLIP(X) ( (X) > 255 ? 255 : (X) < 0 ? 0 : X)

    // RGB -> YUV
#define RGB2Y(R, G, B) CLIP(( (  66 * (R) + 129 * (G) +  25 * (B) + 128) >> 8) +  16)
#define RGB2U(R, G, B) CLIP(( ( -38 * (R) -  74 * (G) + 112 * (B) + 128) >> 8) + 128)
#define RGB2V(R, G, B) CLIP(( ( 112 * (R) -  94 * (G) -  18 * (B) + 128) >> 8) + 128)

#include <stdint.h>
void bgr2yuv(uint8_t *destination, uint8_t *rgb, size_t width, size_t height)
{
    size_t image_size = width * height;
    size_t upos = image_size;
    size_t vpos = upos + upos / 4;
    size_t i = 0;

    for( size_t line = 0; line < height; ++line )
    {
        if( !(line % 2) )
        {
            for( size_t x = 0; x < width; x += 2 )
            {
                uint8_t b = rgb[3 * i];
                uint8_t g = rgb[3 * i + 1];
                uint8_t r = rgb[3 * i + 2];

                destination[i++] = RGB2Y(r,g,b);

                destination[upos++] = RGB2U(r,g,b);
                destination[vpos++] = RGB2V(r,g,b);

                r = rgb[3 * i];
                g = rgb[3 * i + 1];
                b = rgb[3 * i + 2];

                destination[i++] = RGB2Y(r,g,b);
            }
        }
        else
        {
            for( size_t x = 0; x < width; x += 1 )
            {
                uint8_t r = rgb[3 * i];
                uint8_t g = rgb[3 * i + 1];
                uint8_t b = rgb[3 * i + 2];

                destination[i++] = ((66*r + 129*g + 25*b) >> 8) + 16;
            }
        }
    }
}

#define FAME_PARAMETERS_INITIALIZER_JSMPEG {		                             \
    352,					/* CIF width  */	             \
    288,					/* CIF height */	             \
    "I",					/* I sequence */	             \
    75,					/* average video quality */          \
    0,                                    /* variable bitrate */               \
    1,					/* 1 slice/frame */	             \
    0xffffffff,				/* unlimited length */	             \
    25,                                   /* 25 frames/second */               \
    1,                                    /* /1 */                             \
    100,                                  /* original shape */                 \
    0,                                    /* adaptative search range */        \
    1,                                    /* verbose mode */                   \
    "mpeg1",                              /* profile name */                   \
    0,                                    /* number of frames */               \
    NULL                                  /* stats retrieval callback */       \
}
encoder_t *encoder_create(int in_width, int in_height, int out_width, int out_height, int bitrate) {
    encoder_t *self = (encoder_t *)malloc(sizeof(encoder_t));
    memset(self, 0, sizeof(encoder_t));

    self->in_width = in_width;
    self->in_height = in_height;
    self->out_width = out_width;
    self->out_height = out_height;
    self->packet_size = DEFAULT_BUFFER_SIZE;

    fame_parameters_t fp = FAME_PARAMETERS_INITIALIZER_JSMPEG;

    fp.width = out_width ;
    fp.height = out_height ;
    fp.bitrate = bitrate;

    fame_yuv_t* yuv = new fame_yuv_t;
    yuv->w = in_width;
    yuv->h = in_height;
    yuv->p = in_width;
    yuv->y = (unsigned char *) malloc(in_width*in_height*12/8);
    yuv->u = yuv->y + in_width*in_height;
    yuv->v = yuv->u + in_width*in_height/4;
  
    self->packet = (unsigned char *) malloc(self->packet_size);
    self->frame = yuv;
    self->context = fame_open();
    
    fame_init(self->context, &fp, self->packet, self->packet_size);
    
    return self;
}

void encoder_destroy(encoder_t *self) {
    if( self == NULL ) { return; }

    int length = fame_close(self->context);
    free(self->frame->y);
    delete self->frame;
    free(self->packet);
    free(self);
}

void encoder_encode(encoder_t *self, void *rgb_pixels, void *encoded_data, size_t *encoded_size) {

    //ConvertRGB2YUV(self->frame->w,self->frame->h,(unsigned char *)rgb_pixels,(unsigned int *)self->frame->y);
    //bgr2yuv(self->frame->y, (uint8_t*)rgb_pixels, self->in_width, self->in_height);
    RGB2YUV(self->in_width, self->in_height, rgb_pixels, self->frame->y, self->frame->u, self->frame->v, 1);
    *encoded_size = 0;
    //self->frame->pts++;

    fame_start_frame(self->context, self->frame, NULL);
    //must set 1 slice per frame;
    *encoded_size = fame_encode_slice(self->context);
        // TEMP
    memcpy(encoded_data, self->packet, *encoded_size);
    fame_end_frame(self->context, NULL);
   
}
#endif