#ifndef SERVER_H
#define SERVER_H

#include "libwebsocket/libwebsockets.h"

typedef struct server_client_t {
	libwebsocket *socket;
	struct server_client_t *next;
} client_t;

client_t *client_insert(client_t **head, libwebsocket *socket);
void client_remove(client_t **head, libwebsocket *socket);
#define client_foreach(HEAD, CLIENT) for(client_t *CLIENT = HEAD; CLIENT; CLIENT = CLIENT->next)

typedef enum {
	server_type_text = LWS_WRITE_TEXT,
	server_type_binary = LWS_WRITE_BINARY
} server_data_type_t;

typedef struct server_t {
	libwebsocket_context *context;
	size_t buffer_size;
	unsigned char *send_buffer_with_padding;
	unsigned char *send_buffer;
	void *user;
	
	int port;
	server_client_t *clients;
	
	void (*on_connect)(server_t *server, libwebsocket *wsi);
	void (*on_message)(server_t *server, libwebsocket *wsi, void *in, size_t len);
	void (*on_close)(server_t *server, libwebsocket *wsi);
	int (*on_http_req)(server_t *server, libwebsocket *wsi, char *request);
} server_t;


server_t *server_create(int port, size_t buffer_size);
void server_destroy(server_t *self);
char *server_get_host_address(server_t *self);
char *server_get_client_address(server_t *self, libwebsocket *wsi);
void server_update(server_t *self);
void server_send(server_t *self, libwebsocket *socket, void *data, size_t size, server_data_type_t type);
void server_broadcast(server_t *self, void *data, size_t size, server_data_type_t type);

#endif
