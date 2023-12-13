#define _GNU_SOURCE

#include <stdlib.h>
#include <stdio.h>
#include <stdint.h>
#include <pthread.h>
#include <sys/mman.h>
#include "pagemap.h"

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
    void* a;
};
pthread_t threads[NUM_THREADS];

#define MMAP_ACCESS (PROT_READ | PROT_WRITE)
#define MMAP_POLICY (MAP_PRIVATE | MAP_ANONYMOUS)

// return ns
static inline uint64_t read_time(void)
{
    uint64_t a = 0, id = 0;
    asm volatile ( "rdtime.d  %0, %1" : "=r"(a), "=r"(id) :: "memory" );
    return a * 10; // 100MHz * 10
}

static void alloc_mem_aligned(struct thread_param* in) {
    // Each thread is allocated a 2MB huge page
    in->a = mmap(0, MEM, MMAP_ACCESS, MMAP_POLICY, -1, 0);
    if (in->a == MAP_FAILED) {
	    printf("Mmap werror!");
	    return;
    }
    in->sa = (void*)(((unsigned long long)in->a + ALIGN) & (~(ALIGN - 1)));
    in->sb = (void*)((unsigned long long)in->sa + (MEM) / 2);
    pid_t pid = getpid();
    // Init
    *(int*)in->sa = 1;
    *(int*)in->sb = 1;
    uintptr_t paddr_sa = 0, paddr_sb = 0;
    if (lkmc_pagemap_virt_to_phys_user(&paddr_sa, pid, ((uintptr_t)in->sa))) {
        return -1;
    }
    if (lkmc_pagemap_virt_to_phys_user(&paddr_sb, pid, ((uintptr_t)in->sb))) {
        return -1;
    }
    printf("sa virt address: %lx , sa phys address: %lx, sb virt address: %lx , sb phys address: %lx \n",
	   (uintptr_t)in->sa, paddr_sa, (uintptr_t)in->sb, paddr_sb);
}

static void free_mem_aligned(struct thread_param* in) {
    munmap(in->a, MEM);
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
    struct thread_param param[NUM_THREADS];

    for (int i = 0; i < NUM_THREADS; i++) {
	 param[i].id = i;
	 alloc_mem_aligned(&param[i]);
         //printf("%lx, %lx\n", (unsigned long long)param[i].sa, (unsigned long long)param[i].sb);
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
 
    for (int i = 0; i < NUM_THREADS; i++) {
	 free_mem_aligned(&param[i]);
    };
}
