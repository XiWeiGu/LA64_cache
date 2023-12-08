# LA64_cache
Exploring the behavior of caching during matrix multiplication.
echo 16 > /proc/sys/vm/nr_hugepages # 设置大页面数目，3C5000默认大页面32MB
16-threads L3 hit with HUGTLB: gcc main_threads.c kenel_16x6.S -lpthread -DLA3C5000 -DNUM_THREADS=16 -DL3_HIT -o 16-threads-L3.out
16-threads L2 hit with HUGTLB: gcc main_threads.c kenel_16x6.S -lpthread -DLA3C5000 -DNUM_THREADS=16 -DL2_HIT -o 16-threads-L2.out

