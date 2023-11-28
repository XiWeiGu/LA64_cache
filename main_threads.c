#define _GNU_SOURCE

#include <stdlib.h>
#include <stdio.h>
#include <stdint.h>
#include <pthread.h>

void kernel_16x6_L1_lasx(double* a, double* b, int64_t Loops); // L1 cache hit
void kernel_16x6_L2_lasx(double* a, double* b, int64_t Loops); // L2 cache hit
void kernel_16x6_L3_lasx(double* a, double* b, int64_t Loops); // L3 cache hit


#define LOOPS 4194304000
#define ALIGN 64
#ifndef NUM_THREADS
#define NUM_THREADS 16
#endif
#define MEM (100 << 20)
#define OFFSET (MEM / NUM_THREADS)

#ifdef LA3C5000
#define MAX_PEAK 35.2
#else
#define MAX_PEAK 40.0
#endif

struct thread_param {
    int   id;
    void* sa;
    void* sb;
};

void* a;
void* b;
pthread_t threads[NUM_THREADS];

// return ns
static inline uint64_t read_time(void)
{
    uint64_t a = 0, id = 0;
    asm volatile ( "rdtime.d  %0, %1" : "=r"(a), "=r"(id) :: "memory" );
    return a * 10; // 100MHz * 10
}

static void alloc_mem_aligned(int64_t size) {
    posix_memalign(&a, ALIGN, size);
    posix_memalign(&b, ALIGN, size);
}

static void free_mem_aligned() {
    free(a);
    free(b);
}

/* A + B = 512KB + 192KB */
void* kernel_16x6_L3_call_back (void* in) {
    struct thread_param* param  = (struct thread_param*) in;
    int64_t start, end;
    double peak, avg;
    int64_t loops;
    cpu_set_t cpuset;
    CPU_ZERO(&cpuset);
    CPU_SET(param->id, &cpuset);

    pthread_setaffinity_np(threads[param->id], sizeof(cpu_set_t), &cpuset);
    start = read_time();
    loops = LOOPS / 4096;
    kernel_16x6_L3_lasx((double*)(param->sa), (double*)(param->sb), loops);
    end = read_time();
    avg = (end - start) / (double)(loops * 4096);
    peak = (16.0 * 6 * 2) / avg;
    printf("When L3 cache hit, thread NUM %d peak performance is %.2f GFlops, peak floating-point performance ratio %.2f %\n", param->id, peak, peak / MAX_PEAK * 100.0);
}

/* A + B = 80KB + 30KB */
void* kernel_16x6_L2_call_back (void* in) {
    struct thread_param* param  = (struct thread_param*) in;
    int64_t start, end;
    double peak, avg;
    int64_t loops;
    cpu_set_t cpuset;
    CPU_ZERO(&cpuset);
    CPU_SET(param->id, &cpuset);

    pthread_setaffinity_np(threads[param->id], sizeof(cpu_set_t), &cpuset);
    start = read_time();
    loops = LOOPS / 640;
    kernel_16x6_L2_lasx((double*)(param->sa), (double*)(param->sb), loops);
    end = read_time();
    avg = (end - start) / (double)(loops * 640);
    peak = (16.0 * 6 * 2) / avg;
    printf("When L2 cache hit, thread NUM %d peak performance is %.2f GFlops, peak floating-point performance ratio %.2f %\n", param->id, peak, peak / MAX_PEAK * 100.0);
}

/* A + B = 128B + 48B */
void* kernel_16x6_L1_call_back (void* in) {
    struct thread_param* param  = (struct thread_param*) in;
    int64_t start, end;
    double peak, avg;
    int64_t loops;
    cpu_set_t cpuset;
    CPU_ZERO(&cpuset);
    CPU_SET(param->id, &cpuset);

    pthread_setaffinity_np(threads[param->id], sizeof(cpu_set_t), &cpuset);
    start = read_time();
    loops = LOOPS;
    kernel_16x6_L1_lasx((double*)(param->sa), (double*)(param->sb), loops);
    end = read_time();
    avg = (end - start) / (double)(loops);
    peak = (16.0 * 6 * 2) / avg;
    printf("When L1 cache hit, thread NUM %d peak performance is %.2f GFlops, peak floating-point performance ratio %.2f %\n", param->id, peak, peak / MAX_PEAK * 100.0);
}

void main () {
    int rc;
    alloc_mem_aligned(MEM); // Alloc 100MB for a and b

    struct thread_param param[NUM_THREADS];

    for (int i = 0; i < NUM_THREADS; i++) {
	 param[i].id = i;
	 param[i].sa = (void*)((unsigned long long)a + i * OFFSET);
	 param[i].sb = (void*)((unsigned long long)b + i * OFFSET);
#ifdef L3_HIT
	 pthread_create(&threads[i], NULL, kernel_16x6_L3_call_back, (void *)(&param[i]));
#elif defined(L2_HIT)
	 pthread_create(&threads[i], NULL, kernel_16x6_L2_call_back, (void *)(&param[i]));
#else
	 pthread_create(&threads[i], NULL, kernel_16x6_L1_call_back, (void *)(&param[i]));
#endif
    }

    for (int t = 0; t < NUM_THREADS; ++t) {
        rc = pthread_join(threads[t], NULL);
        if (rc) {
            printf("Error in pthread_join(): %d\n", rc);
            return;
        }
    }
    free_mem_aligned();
}
