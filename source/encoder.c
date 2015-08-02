#include <stdio.h>
#include <stdlib.h>

#include "encoder.h"

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
