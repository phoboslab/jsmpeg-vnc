#ifndef ENCODER_H
#define ENCODER_H

#ifdef USE_FFMPEG
extern "C" {
	#include "ffmpeg/include/libavutil/avutil.h"
	#include "ffmpeg/include/libavcodec/avcodec.h"
	#include "ffmpeg/include/libswscale/swscale.h"	

}
typedef struct {
    AVCodec *codec;
    AVCodecContext *context;
    AVFrame *frame;
    void *frame_buffer;

    int in_width, in_height;
    int out_width, out_height;

    AVPacket packet;
    SwsContext *sws;
} encoder_t;

#else
//#include "rgb2yuv/include/rgb2yuv.hpp"
extern "C" {
#include "fame/include/fame.h"
}
typedef struct {
    //AVCodec *codec;
    fame_context_t *context;
    fame_yuv_t *frame;

    int in_width, in_height;
    int out_width, out_height;

    unsigned char* packet;
    unsigned int packet_size;
} encoder_t;
#pragma comment(lib,"libfame.lib")
#endif


encoder_t *encoder_create(int in_width, int in_height, int out_width, int out_height, int bitrate, int video_quality);
void encoder_destroy(encoder_t *self);
void encoder_encode(encoder_t *self, void *rgb_pixels, void *encoded_data, size_t *encoded_size);

#endif
