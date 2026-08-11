/* A guest that exercises the green-thread path: it spawns N pthreads that each
 * increment a shared counter under a mutex a fixed number of times, joins them,
 * and reports the total. Correct cooperative scheduling + futex-backed
 * pthread_mutex + join must yield exactly N*ITERS. */
#include <emulibc.h>
#include <pthread.h>
#include <stdint.h>
#include <stdio.h>

#define NTHREADS 4
#define ITERS 1000

static pthread_mutex_t g_lock = PTHREAD_MUTEX_INITIALIZER;
static volatile uint64_t g_counter;

static void *worker(void *arg) {
	(void)arg;
	for (int i = 0; i < ITERS; i++) {
		pthread_mutex_lock(&g_lock);
		g_counter++;
		pthread_mutex_unlock(&g_lock);
	}
	return 0;
}

ECL_EXPORT uint64_t RunThreads(void) {
	pthread_t th[NTHREADS];
	g_counter = 0;
	for (int i = 0; i < NTHREADS; i++)
		if (pthread_create(&th[i], 0, worker, 0) != 0) { fprintf(stderr, "pthread_create failed\n"); return 0; }
	for (int i = 0; i < NTHREADS; i++)
		pthread_join(th[i], 0);
	return g_counter;   /* must equal NTHREADS*ITERS */
}

/* a producer/consumer round using a condition variable, exercising futex
 * wait/wake and requeue via pthread_cond */
static pthread_mutex_t g_cvlock = PTHREAD_MUTEX_INITIALIZER;
static pthread_cond_t g_cv = PTHREAD_COND_INITIALIZER;
static volatile int g_ready;
static volatile uint64_t g_produced;

static void *consumer(void *arg) {
	(void)arg;
	pthread_mutex_lock(&g_cvlock);
	while (!g_ready) pthread_cond_wait(&g_cv, &g_cvlock);
	uint64_t v = g_produced;
	pthread_mutex_unlock(&g_cvlock);
	return (void *)(uintptr_t)v;
}

ECL_EXPORT uint64_t RunCondvar(uint64_t value) {
	pthread_t c;
	g_ready = 0; g_produced = 0;
	pthread_create(&c, 0, consumer, 0);
	pthread_mutex_lock(&g_cvlock);
	g_produced = value * 2 + 1;
	g_ready = 1;
	pthread_cond_signal(&g_cv);
	pthread_mutex_unlock(&g_cvlock);
	void *r = 0;
	pthread_join(c, &r);
	return (uint64_t)(uintptr_t)r;   /* must equal value*2+1 */
}

ECL_EXPORT int Init(void) { return 1; }
int main(void) { return 0; }
