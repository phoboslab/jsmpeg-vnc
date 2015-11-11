#define WIN32_LEAN_AND_MEAN
#include <Windows.h>
#include <stdio.h>
#include <stdlib.h>

#include "grabber.h"

grabber_t *grabber_create(HWND window, int destWitdh, int destHeight) {
	grabber_t *self = (grabber_t *)malloc(sizeof(grabber_t));
	memset(self, 0, sizeof(grabber_t));
	
	RECT rect;
	GetClientRect(window, &rect);
	
	self->window = window;
	
    
	self->width = rect.right-rect.left;
	self->height = rect.bottom-rect.top;

    self->destWidth = destWitdh;
    self->destHeight = destHeight;
	
	self->windowDC = GetDC(window);
    //This is the best stretch mode
    SetStretchBltMode(self->windowDC,HALFTONE);

	self->memoryDC = CreateCompatibleDC(self->windowDC);
	self->bitmap = CreateCompatibleBitmap(self->windowDC, self->width, self->height);
	
	self->bitmapInfo.biSize = sizeof(BITMAPINFOHEADER);
	self->bitmapInfo.biPlanes = 1;
#ifdef USE_FFMPEG
	self->bitmapInfo.biBitCount = 32;
#else
    self->bitmapInfo.biBitCount = 32;
#endif
	self->bitmapInfo.biWidth = self->width;
	self->bitmapInfo.biHeight = -self->height;
	self->bitmapInfo.biCompression = BI_RGB;
	self->bitmapInfo.biSizeImage = 0;
	
	self->pixels = malloc(self->width * self->height * 4);

	return self;
}

void grabber_destroy(grabber_t *self) {
	if( self == NULL ) { return; }

	ReleaseDC(self->window, self->windowDC);
    DeleteDC(self->memoryDC);
    DeleteObject(self->bitmap);
	
	free(self->pixels);
	free(self);
}

void *grabber_grab(grabber_t *self) {
	SelectObject(self->memoryDC, self->bitmap);

    if ((self->destWidth != 0) && (self->destWidth != self->width || self->destHeight != self->height))
    {
        StretchBlt(self->memoryDC, 0, 0, self->destWidth, self->destHeight, self->windowDC, 0, 0, self->width, self->height, SRCCOPY);
    }
    else
	    BitBlt(self->memoryDC, 0, 0, self->width, self->height, self->windowDC, 0, 0, SRCCOPY);
	GetDIBits(self->memoryDC, self->bitmap, 0, self->height, self->pixels, (BITMAPINFO*)&(self->bitmapInfo), DIB_RGB_COLORS);
	
	return self->pixels;
}

