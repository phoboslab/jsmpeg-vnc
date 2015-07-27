#define WIN32_LEAN_AND_MEAN
#include <Windows.h>
#include <stdio.h>
#include "app.h"

typedef struct {
	char *prefix;
	HWND window;
} window_with_prefix_data_t;

BOOL CALLBACK window_with_prefix_callback(HWND window, LPARAM lParam) {
	window_with_prefix_data_t *find = (window_with_prefix_data_t *)lParam;
	
	char title[80];
	GetWindowTextA(window, title, sizeof(title));

	if( !find->window && strncmp(find->prefix, title, strlen(find->prefix)) == 0 ) {
		find->window = window;
	}
	return TRUE;
}

HWND window_with_prefix(char *title_prefix) {
	window_with_prefix_data_t find = {title_prefix, NULL};
	EnumWindows(window_with_prefix_callback, (LPARAM)&find);

	return find.window;
}

void exit_usage(char *self_name) {
	printf(
		"Usage: %s [options] <window name>\n\n"

		"Options:\n"
		"	-b bitrate in kilobit/s (default: estimated by output size)\n"
		"	-s output size as WxH. E.g: -s 640x480 (default: same as window size)\n"
		"	-f target framerate (default: 60)\n"
		"	-p port (default: 8080)\n\n"

		"Use \"desktop\" as the window name to capture the whole Desktop. Use \"cursor\"\n"
		"to capture the window at the current cursor position.\n\n"

		"To enable mouse lock in the browser (useful for games that require relative\n"
		"mouse movements, not absolute ones), append \"?mouselock\" at the target URL.\n"
		"i.e: http://<server-ip>:8080/?mouselock\n\n",		
		self_name
	);
	exit(0);
}

int main(int argc, char* argv[]) {
	if( argc < 2 ) {
		exit_usage(argv[0]);
	}

	int bit_rate = 0,
		fps = 60,
		port = 8080,
		width = 0,
		height = 0;

	// Parse command line options
	for( int i = 1; i < argc-1; i+=2 ) {
		if( strlen(argv[i]) < 2 || i >= argc-2 || argv[i][0] != '-' ) {
			exit_usage(argv[0]);
		}

		switch( argv[i][1] ) {
			case 'b': bit_rate = atoi(argv[i+1]) * 1000; break;
			case 'p': port = atoi(argv[i+1]); break;
			case 's': sscanf(argv[i+1], "%dx%d", &width, &height); break;
			case 'f': fps = atoi(argv[i+1]); break;
			default: exit_usage(argv[0]);
		}
	}

	// Find target window
	char *window_title = argv[argc-1];
	HWND window = NULL;
	if( strcmp(window_title, "desktop") == 0 ) {
		window = GetDesktopWindow();
	}
	else if( strcmp(window_title, "cursor") == 0 ) {
		POINT cursor;
		GetCursorPos(&cursor);
		window = WindowFromPoint(cursor);
	}
	else {
		window = window_with_prefix(window_title);
	}

	if( !window ) {
		printf("No window with title starting with \"%s\"\n", window_title);
		return 0;
	}

	// Start the app
	app_t *app = app_create(window, port, bit_rate, width, height);

	char real_window_title[56];
	GetWindowTextA(window, real_window_title, sizeof(real_window_title));
	printf(
		"Window 0x%08x: \"%s\"\n"
		"Window size: %dx%d, output size: %dx%d, bit rate: %d kb/s\n\n"
		"Server started on: http://%s:%d/\n\n",
		window, real_window_title,
		app->grabber->width, app->grabber->height,
		app->encoder->out_width, app->encoder->out_height,
		app->encoder->context->bit_rate / 1000,
		server_get_host_address(app->server), app->server->port
	);

	app_run(app, fps);

	app_destroy(app);	

	return 0;
}

