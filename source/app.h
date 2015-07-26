#ifndef APP_H
#define APP_H

#define WIN32_LEAN_AND_MEAN
#include <Windows.h>

#include "encoder.h"
#include "grabber.h"
#include "server.h"

#define APP_MOUSE_SPEED 5.0f
#define APP_FRAME_BUFFER_SIZE (1024*1024)

typedef struct {
	encoder_t *encoder;
	grabber_t *grabber;
	server_t *server;

	float mouse_speed;
} app_t;


app_t *app_create(HWND window, int port, int bit_rate, int out_width, int out_height);
void app_destroy(app_t *self);
void app_run(app_t *self, int targt_fps);

int app_on_http_req(app_t *self, libwebsocket *socket, char *request);
void app_on_connect(app_t *self, libwebsocket *socket);
void app_on_close(app_t *self, libwebsocket *socket);
void app_on_message(app_t *self, libwebsocket *socket, void *data, size_t len);

#endif
