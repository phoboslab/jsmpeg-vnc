#ifndef GRABBER_H
#define GRABBER_H

#define WIN32_LEAN_AND_MEAN
#include <Windows.h>

typedef struct {
	HWND window;
	
	HDC windowDC;
	HDC memoryDC;
	HBITMAP bitmap;
	BITMAPINFOHEADER bitmapInfo;
	
	int width;
	int height;
	
	void *pixels;
} grabber_t;


grabber_t *grabber_create(HWND window);
void grabber_destroy(grabber_t *self);
void *grabber_grab(grabber_t *self);

#endif
