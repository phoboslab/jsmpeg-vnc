#ifndef GRABBER_H
#define GRABBER_H

#define WIN32_LEAN_AND_MEAN
#include <Windows.h>

typedef struct {
	int x, y, width, height;
} grabber_crop_area_t;

typedef struct {
	HWND window;
	
	HDC windowDC;
	HDC memoryDC;
	HBITMAP bitmap;
	BITMAPINFOHEADER bitmapInfo;
	
	int width;
	int height;
	
	void *pixels;
	grabber_crop_area_t crop;
} grabber_t;

grabber_t *grabber_create(HWND window, grabber_crop_area_t crop);
void grabber_destroy(grabber_t *self);
void *grabber_grab(grabber_t *self);

#endif
