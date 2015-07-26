#ifndef TIMER_H
#define TIMER_H

typedef struct timer_t {
	__int64 base_time;
} timer_t;

timer_t *timer_create();
void timer_destroy(timer_t *self);
void timer_reset(timer_t *self);
double timer_delta(timer_t *self);

__int64 __timer_measure_start();
void __timer_measure_end(__int64 *start, double *result);

// double elapsed = timer_measure(elapsed) { <statements to measure> }
#define timer_measure(TIME) 0; \
	for( \
		__int64 TIME##__timer_start = 0; \
		TIME##__timer_start == 0 && (TIME##__timer_start = __timer_measure_start()) != 0; \
		__timer_measure_end(&TIME##__timer_start, &TIME) \
	)

#endif
