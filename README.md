# LA64_cache
Exploring the behavior of caching during matrix multiplication.
16-threads L3 hit with HUGTLB: gcc main_threads.c kenel_16x6.S -lpthread -DLA3C5000 -DNUM_THREADS=16 -DL3_HIT -DMMAP -o 16-threads-L3.out

