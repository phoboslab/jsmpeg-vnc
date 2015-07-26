#define WIN32_LEAN_AND_MEAN
#include <Windows.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "timer.h"

static double timer_frequency = 0.0f;

#define TIMER_GLOBAL_INIT() \
	if( timer_frequency == 0.0f ) { \
		LARGE_INTEGER freq; \
		QueryPerformanceFrequency(&freq); \
		timer_frequency = double(freq.QuadPart)/1000.0; \
	}

timer_t *timer_create() {
	TIMER_GLOBAL_INIT();

	timer_t *self = (timer_t *)malloc(sizeof(timer_t));
	memset(self, 0, sizeof(timer_t));

	timer_reset(self);
	return self;
}

void timer_destroy(timer_t *self) {
	free(self);
}

void timer_reset(timer_t *self) {
	LARGE_INTEGER time;
    QueryPerformanceCounter(&time);
	self->base_time = time.QuadPart;
}

double timer_delta(timer_t *self) {
	LARGE_INTEGER time;
    QueryPerformanceCounter(&time);
	return double(time.QuadPart-self->base_time)/timer_frequency;
}

__int64 __timer_measure_start() {
	TIMER_GLOBAL_INIT();

	LARGE_INTEGER time;
    QueryPerformanceCounter(&time);
	return time.QuadPart;
}

void __timer_measure_end(__int64 *start, double *result) {
	LARGE_INTEGER time;
    QueryPerformanceCounter(&time);
	*result += double(time.QuadPart - *start)/timer_frequency;
}
