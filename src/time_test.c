#include <stdlib.h>
#include <stdio.h>
#include <stdint.h>
#include "debug.h"

/*const float CPU_MHZ = 3000.164; //use cat /proc/cpuinfo get the value*/
#define     CPU_MHZ     (3000.164)
static const double CPU_TICK_COUNT_PER_SECOND = (CPU_MHZ*1000*1000);
static const uint64_t CPU_TICK_COUNT_PER_MILLI_SECOND = (CPU_MHZ*1000);
static const uint64_t CPU_TICK_COUNT_PER_MICRO_SECOND = (CPU_MHZ);
static const uint64_t CPU_TICK_COUNT_PER_NANO_SECOND = (CPU_MHZ/1000);

#ifdef _i386

static __inline__ unsigned long long rdtsc(void)
{
    unsigned long long int x;
    __asm__ volatile (".byte 0x0f, 0x31" : "=A" (x)); //.byte 0x0f,0x31等价于rdtsc，是另一种原始取机器码的方式
    return x;
} 

#elif __x86_64__
static __inline__ unsigned long long rdtsc(void)
{
    unsigned hi, lo;
    __asm__ __volatile__ ("rdtsc" : "=a"(lo), "=d"(hi));
    return ( (unsigned long long)lo)|( ((unsigned long long)hi)<<32 );
}

#endif

static int time_test(const register int start, const register int end)
{
    register double secs = (end - start)/CPU_TICK_COUNT_PER_SECOND;
    register double milli_secs = (end - start)/CPU_TICK_COUNT_PER_MILLI_SECOND;
    register double micro_secs = (end - start)/CPU_TICK_COUNT_PER_MICRO_SECOND;
    register double nano_secs = (end - start)/CPU_TICK_COUNT_PER_NANO_SECOND;

    DD("cpu tick counts : %ld", end - start);
    DD("seconds = %lf", secs);
    DD("milli seconds = %lf", milli_secs);
    DD("micro seconds = %lf", micro_secs);
    DD("nano seconds = %lf", nano_secs);
}

#ifdef  _TEST_

int test_func(void)
{
    int i = 0;
    register int start = 0;
    register int end = 0;
    const int MAX_COUNT = 1000000;
    volatile int sum = 0;
    start = rdtsc();
    for(i = 0 ; i< MAX_COUNT ; ++i)
    {
        sum+=1;
    }
    end = rdtsc();

    time_test(start, end);

    return 0;
}

int main(int argc, char **argv)
{
    test_func();
}

#endif
