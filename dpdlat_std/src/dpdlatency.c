/*
 * Copyright (c) 2012 Xilinx, Inc.  All rights reserved.
 *
 * Xilinx, Inc.
 * XILINX IS PROVIDING THIS DESIGN, CODE, OR INFORMATION "AS IS" AS A
 * COURTESY TO YOU.  BY PROVIDING THIS DESIGN, CODE, OR INFORMATION AS
 * ONE POSSIBLE   IMPLEMENTATION OF THIS FEATURE, APPLICATION OR
 * STANDARD, XILINX IS MAKING NO REPRESENTATION THAT THIS IMPLEMENTATION
 * IS FREE FROM ANY CLAIMS OF INFRINGEMENT, AND YOU ARE RESPONSIBLE
 * FOR OBTAINING ANY RIGHTS YOU MAY REQUIRE FOR YOUR IMPLEMENTATION.
 * XILINX EXPRESSLY DISCLAIMS ANY WARRANTY WHATSOEVER WITH RESPECT TO
 * THE ADEQUACY OF THE IMPLEMENTATION, INCLUDING BUT NOT LIMITED TO
 * ANY WARRANTIES OR REPRESENTATIONS THAT THIS IMPLEMENTATION IS FREE
 * FROM CLAIMS OF INFRINGEMENT, IMPLIED WARRANTIES OF MERCHANTABILITY
 * AND FITNESS FOR A PARTICULAR PURPOSE.
 *
 */



#define _GNU_SOURCE

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <math.h>  
#include <fcntl.h>
#include <sched.h>  
#include <pthread.h>   
#include <assert.h>  
#include <errno.h>  
#include <string.h>  
//#include <sys/mman.h>
#include <sys/time.h>
#include <sys/types.h>
#include <sys/resource.h>

#define APP_STANDALONE
//#define APP_LINUX

// MEM_COPY_SIZE 32768, Linux, single DPD application, read loop average number: 102
// MEM_COPY_SIZE 16384? , Linux, single DPD application, read loop average number: 106
// MEM_COPY_SIZE 4096? , Linux, single DPD application, read loop average number: 109
#define MEM_COPY_SIZE 			32768  // 16384   8192, 4092
#define MEM_COPY_SIZE_KB		(MEM_COPY_SIZE/1024)

/* Configurable parameteres: START */
#define IOU_SCNTR_ADRS_BASE		0xFF250000 
#define IOU_SCNTR_ADRS_SIZE		0x10 
#define IOU_SCNTR_FREQ_MICRO	(100)  //100MHz
#define IOU_SCNTR_FREQ_MILLI	(IOU_SCNTR_FREQ_MICRO*1000)
#define IOU_SCNTR_FREQ			(IOU_SCNTR_FREQ_MILLI*1000)  //100MHz


#define MAP_SIZE 4096UL
#define MAP_MASK (MAP_SIZE - 1)


//typedef		__u32		uint32_t;
//typedef		__s32		int32_t;
//typedef		__u64		uint64_t;

typedef struct delay_params {
	unsigned long ul_us;
	unsigned long ul_cycles;
	unsigned long ul_counter_end;
	unsigned long ul_counter_begin;
	unsigned long ul_counter_read;
	unsigned long ul_loop;
}delay_params_t;


typedef struct latency_params {
	unsigned long ul_cpus;
	unsigned long ul_cpus2;
	unsigned long ul_mode;
	unsigned long ul_delay_us;
	unsigned long ul_test_seconds;
	unsigned long ul_memory_MB;
	volatile unsigned long *pu64_mapped_counter;
}latency_params_t;

#define print_var(xxx) 			printf("%s = %lu\r\n", #xxx, xxx )


inline volatile unsigned long read_sys_counter( volatile unsigned long *pu64_mapped_counter )
{
    volatile unsigned long u64_counter_delay_read=0;
    volatile unsigned long u64_counter_delay_read2=0;

	/*
	delay_params.ul_counter_begin = 141733920517 = 0x20FFFFFF05
	delay_params.ul_counter_end =   141733930517 = 0x2100002615
	delay_params.ul_counter_read =  146028888053 = 0x21FFFFFFF5.
	Counter value read 1: 141940035803 = 0x210C4910DB.
	*/
    for( ; ; ) 
	{
		u64_counter_delay_read = *pu64_mapped_counter;
		u64_counter_delay_read2 = *pu64_mapped_counter;
		if( (u64_counter_delay_read&0xfffffffffff00000) 
			== (u64_counter_delay_read2&0xfffffffffff00000) ) 
		{
			break;
		}
    }

    return u64_counter_delay_read2;
}


int udelay_sys_counter( volatile unsigned long *pu64_mapped_counter, int us, delay_params_t *p_delay_params )
{
    unsigned long ul_loop=0;
    unsigned long ul_delay_cycles;
    unsigned long u64_counter_delay_begin;
    unsigned long u64_counter_delay_end;
    volatile unsigned long u64_counter_delay_read;


	ul_delay_cycles = us;	
	ul_delay_cycles = ul_delay_cycles*(IOU_SCNTR_FREQ_MICRO);
	p_delay_params->ul_us = us;
	p_delay_params->ul_cycles = ul_delay_cycles;

	u64_counter_delay_begin = read_sys_counter( pu64_mapped_counter );
	u64_counter_delay_end = u64_counter_delay_begin+ul_delay_cycles;
    if( u64_counter_delay_end < u64_counter_delay_begin ) 
	{
		printf("System counter value end overflow.\n");
    }

	/*
	delay_params.ul_counter_begin = 141733920517 = 0x20FFFFFF05
	delay_params.ul_counter_end =   141733930517 = 0x2100002615
	delay_params.ul_counter_read =  146028888053 = 0x21FFFFFFF5.
	Counter value read 1: 141940035803 = 0x210C4910DB.
	*/
	u64_counter_delay_read = read_sys_counter( pu64_mapped_counter );
    for( ul_loop=0; u64_counter_delay_read<u64_counter_delay_end; ul_loop++) 
	{
		u64_counter_delay_read = read_sys_counter( pu64_mapped_counter );
    }
	p_delay_params->ul_counter_begin = u64_counter_delay_begin;
	p_delay_params->ul_counter_end = u64_counter_delay_end;
	p_delay_params->ul_counter_read = u64_counter_delay_read;
	p_delay_params->ul_loop = ul_loop;

    return ul_loop;
}


int udelay_begin_sys_counter( volatile unsigned long *pu64_mapped_counter, int us, unsigned long u64_counter_delay_begin, delay_params_t *p_delay_params )
{
    unsigned long ul_loop=0;
    unsigned long ul_delay_cycles;
    unsigned long u64_counter_delay_end;
    volatile unsigned long u64_counter_delay_read;

	ul_delay_cycles = us;	
	ul_delay_cycles = ul_delay_cycles*(IOU_SCNTR_FREQ_MICRO);
	p_delay_params->ul_us = us;
	p_delay_params->ul_cycles = ul_delay_cycles;

	u64_counter_delay_end = u64_counter_delay_begin+ul_delay_cycles;
    if( u64_counter_delay_end < u64_counter_delay_begin ) 
	{
		printf("System counter value end overflow.\n");
    }

	/*
	delay_params.ul_counter_begin = 141733920517 = 0x20FFFFFF05
	delay_params.ul_counter_end =   141733930517 = 0x2100002615
	delay_params.ul_counter_read =  146028888053 = 0x21FFFFFFF5.
	Counter value read 1: 141940035803 = 0x210C4910DB.
	*/
	u64_counter_delay_read = read_sys_counter( pu64_mapped_counter );
    for( ul_loop=0; u64_counter_delay_read<u64_counter_delay_end; ul_loop++) 
	{
		u64_counter_delay_read = read_sys_counter( pu64_mapped_counter );
    }
	p_delay_params->ul_counter_begin = u64_counter_delay_begin;
	p_delay_params->ul_counter_end = u64_counter_delay_end;
	p_delay_params->ul_counter_read = u64_counter_delay_read;
	p_delay_params->ul_loop = ul_loop;

    return ul_loop;
}


// Run one time for whole process, or for each thread
int cpu_affinity_set(int core)
{

#ifdef APP_LINUX

    int j;
    int ret;
    int num_cpus;
    pid_t pid;
    cpu_set_t cpuset;
    struct sched_param param;

    num_cpus = sysconf(_SC_NPROCESSORS_CONF);/*get the cpu nums*/
    printf("CPU number in system: %d\n", num_cpus);
    assert(num_cpus > 0);

	printf("cpu core mask:%x\n", core);

	pid = getpid( );
    ret = sched_getaffinity(pid, sizeof(cpu_set_t), &cpuset);
	//printf("sched_getaffinity return: %d\n", ret);
	//printf("errno: %d\n", errno);
    assert(ret == 0);
	for (j = 0; (j < CPU_SETSIZE) && (j < num_cpus); j++)
	{
		if( CPU_ISSET(j, &cpuset)) 
		{
			printf("Before setting, PID:%d program is bound on CPU %d\n", pid, j);
		}
	}

	// Set cpuset.
    CPU_ZERO(&cpuset);
    for (j = 0; (j < CPU_SETSIZE) && (j < num_cpus); j++)
    {
       if ( 0 != ( core&(1<<j) ) )
       {
    	   printf("set program bind on CPU %d\n", j);
		   CPU_SET(j, &cpuset);
       }
    }
	
    ret = sched_setaffinity(pid, sizeof(cpu_set_t), &cpuset);
	//printf("sched_setaffinity return: %d\n", ret);
	//printf("errno: %d\n", errno);
    assert(ret == 0);


    ret = sched_getaffinity(pid, sizeof(cpu_set_t), &cpuset);
	//printf("sched_getaffinity return: %d\n", ret);
	//printf("errno: %d\n", errno);
    assert(ret == 0);
	for (j = 0; (j < CPU_SETSIZE) && (j < num_cpus); j++)
	{
		if( CPU_ISSET(j, &cpuset)) 
		{
			printf("After setting, PID:%d program is bound on CPU %d\n", pid, j);
		}
	}

	//  int setpriority(int which,int who, int prio);
	printf("Before setting, PID:%d program priority is: %d\n", pid, getpriority(PRIO_PROCESS, 0));
	ret = setpriority(PRIO_PROCESS, 0, -10);
	printf("After setting, PID:%d program priority is: %d\n", pid, getpriority(PRIO_PROCESS, 0));
    assert(ret == 0);

	// int sched_setscheduler(pid_t pid, int policy, const struct sched_param *param);
	//
	param.sched_priority = sched_get_priority_max(SCHED_FIFO);
	ret = sched_setscheduler(0, SCHED_FIFO, &param);
    assert(ret == 0);

#endif // APP_LINUX

    return 0;
}


int latency_check( latency_params_t *p_params )
{
    unsigned long ul_loop;
	unsigned long ul_test_seconds;
    unsigned long ul_delay_us=50;
    unsigned long ul_mode=1;	
    unsigned long ul_test_cycles;
    unsigned long ul_delay_loop=0;
    unsigned long ul_delay_loop_min=((unsigned long)(-1));
    unsigned long ul_delay_loop_max=0;
    unsigned long ul_delay_loop_tot=0;
    unsigned long ul_delay_loop_average=0;
    unsigned long u64_counter_begin;
    unsigned long u64_counter_read1;
    unsigned long u64_counter_read2;
    unsigned long u64_counter_end;
    unsigned long u64_counter_last;
    unsigned long u64_counter_last_min=((unsigned long)(-1));
    unsigned long u64_counter_last_max=0;
    unsigned long u64_counter_last_tot=0;
    unsigned long u64_counter_last_average=0;
    volatile unsigned long *pu64_mapped_counter;
    unsigned long *pu64_mem_src;
    unsigned long *pu64_mem_dst;
    unsigned long u64_mem_test_offset=0;

	delay_params_t delay_params;

	ul_test_seconds = p_params->ul_test_seconds;
    ul_delay_us = p_params->ul_delay_us;
    pu64_mapped_counter = p_params->pu64_mapped_counter;
	ul_test_cycles = ul_test_seconds*(IOU_SCNTR_FREQ);
	printf("\n");
	printf("Test %lu seconds\n", ul_test_seconds);
	printf("Test %lu cycles\n", ul_test_cycles);
	printf("Delay %lu us in loop\n", p_params->ul_delay_us);
	printf("Test on CPUs: %lu=0X%lx\n", p_params->ul_cpus, p_params->ul_cpus);
	printf("Test mode(1: busy delay, 2: sleep): %lu.\n", ul_mode);
	
    if( p_params->ul_memory_MB < 2 ) 
	{
		p_params->ul_memory_MB=2;
    }
	else if( p_params->ul_memory_MB > 128 ) 
	{
		p_params->ul_memory_MB=128;
	}
	else
	{
	}

	printf("Memory copy size: %d = %dKB.\n", MEM_COPY_SIZE, MEM_COPY_SIZE_KB);
 	pu64_mem_src=malloc(p_params->ul_memory_MB*1024*1024);
    if( NULL == pu64_mem_src ) 
	{
		printf("Failed to allocate source memory.\n");
		return 0;
    }
	printf("Source memory is allocated.\n");

    pu64_mem_dst=malloc(p_params->ul_memory_MB*1024*1024);
    if( NULL == pu64_mem_dst ) 
	{
		printf("Failed to allocate destination memory.\n");
		free(pu64_mem_src);
		return 0;
    }
	printf("Destination memory is allocated.\n");

	cpu_affinity_set(p_params->ul_cpus);

	u64_counter_begin = read_sys_counter( pu64_mapped_counter );
	u64_counter_end = u64_counter_begin+ul_test_cycles;
	printf("System counter value loop begin: %lu.\n", u64_counter_begin);
	printf("System counter value loop end: %lu.\n", u64_counter_end);
    if( u64_counter_end < u64_counter_begin ) 
	{
		printf("System counter value end overflow.\n");
    }

	u64_counter_read2 = read_sys_counter( pu64_mapped_counter );
    for(ul_loop=0; u64_counter_read2<u64_counter_end; ul_loop++) {
		
		u64_counter_read1 = read_sys_counter( pu64_mapped_counter );
		if( 1==ul_mode )
		{
			unsigned long *pu64_mem_src_temp;
			unsigned long *pu64_mem_dst_temp;

			// copy data to generate cache miss and MMU table update
			pu64_mem_src_temp = &pu64_mem_src[u64_mem_test_offset];
			pu64_mem_dst_temp = &pu64_mem_dst[u64_mem_test_offset];
			memcpy(pu64_mem_dst_temp, pu64_mem_src_temp, MEM_COPY_SIZE);
			u64_mem_test_offset += MEM_COPY_SIZE/sizeof(unsigned long);
			u64_mem_test_offset = u64_mem_test_offset%(p_params->ul_memory_MB*(1024-MEM_COPY_SIZE_KB)*1024/sizeof(unsigned long));
			ul_delay_loop = udelay_begin_sys_counter( pu64_mapped_counter, ul_delay_us, u64_counter_read1, &delay_params );
		}
		else
		{
			usleep(ul_delay_us);
		}
		u64_counter_read2 = read_sys_counter( pu64_mapped_counter );
		//printf("File: %s, Func: %s, Line: %d.\n", __FILE__, __func__, __LINE__ );
		
		if( ul_delay_loop_min > ul_delay_loop ) 
		{
			ul_delay_loop_min = ul_delay_loop;
			printf("\n");
			printf("Delay %lu us, minimum counter reading loop: %lu at No.%lu outer loop, memory offset: %lu.\n",
					ul_delay_us, ul_delay_loop_min, ul_loop, u64_mem_test_offset);
		}		
		if( ul_delay_loop > ul_delay_loop_max ) 
		{
			ul_delay_loop_max = ul_delay_loop;
			printf("\n");
			printf("Delay %lu us, maximum counter reading loop: %lu at No.%lu outer loop, memory offset: %lu.\n",
					ul_delay_us, ul_delay_loop_max, ul_loop, u64_mem_test_offset);
		}
		ul_delay_loop_tot+=ul_delay_loop;
		
		if( u64_counter_read2 > u64_counter_read1 ) 
		{
			u64_counter_last = u64_counter_read2 -u64_counter_read1;
		}
		else{
			u64_counter_last = u64_counter_read1 -u64_counter_read2;
		}
		
		if( u64_counter_last_min > u64_counter_last ) 
		{
			u64_counter_last_min = u64_counter_last;
			printf("\n");
			printf("Counter value read 1: %lu=0x%lx.\n", u64_counter_read1, u64_counter_read1);
			printf("Counter value read 2: %lu=0x%lx.\n", u64_counter_read2, u64_counter_read2);
			printf("counter value difference minimum: %lu = %lu us at No.%lu outer loop, memory offset: %lu.\n",
						u64_counter_last_min, u64_counter_last_min/IOU_SCNTR_FREQ_MICRO, ul_loop, u64_mem_test_offset);

			print_var(delay_params.ul_us);
			print_var(delay_params.ul_cycles);
			print_var(delay_params.ul_counter_begin);
			print_var(delay_params.ul_counter_end);
			print_var(delay_params.ul_counter_read);
			print_var(delay_params.ul_loop);

		}		
		if( u64_counter_last > u64_counter_last_max ) 
		{
			u64_counter_last_max = u64_counter_last;
			printf("\n");
			printf("Counter value read 1: %lu=0x%lx.\n", u64_counter_read1, u64_counter_read1);
			printf("Counter value read 2: %lu=0x%lx.\n", u64_counter_read2, u64_counter_read2);
			printf("counter value difference maximum: %lu = %lu us at No.%lu outer loop, memory offset: %lu.\n",
						u64_counter_last_max, u64_counter_last_max/IOU_SCNTR_FREQ_MICRO, ul_loop, u64_mem_test_offset);
		}
		u64_counter_last_tot+=u64_counter_last;
		
		// It takes about 57~97 microsecond to delay 1us in Linux.
		// So it loops about 20*1000 times per seconds
		// The process might be interrupted by new process, new command on shell.
		if( 0 == ((ul_loop+1)%(20000)) ) {
			printf("\n");

			if( 1==ul_mode )
			{
				ul_delay_loop_average = ul_delay_loop_tot/ul_loop;
				printf("Delay %lu us, counter reading loop: %lu.\n", ul_delay_us, ul_delay_loop);
				printf("Delay %lu us, minimum counter reading loop: %lu.\n", ul_delay_us, ul_delay_loop_min);
				printf("Delay %lu us, maximum counter reading loop: %lu.\n", ul_delay_us, ul_delay_loop_max);
				printf("Delay %lu us, average counter reading loop: %lu.\n", ul_delay_us, ul_delay_loop_average);
			}
			
			u64_counter_last_average = u64_counter_last_tot/ul_loop;
			printf("Counter value read 1: %lu=0x%lx.\n", u64_counter_read1, u64_counter_read1);
			printf("Counter value read 2: %lu=0x%lx.\n", u64_counter_read2, u64_counter_read2);
			printf("Counter value difference data for %lu us delay: %lu, memory offset: %lu\n", 
						ul_delay_us, u64_counter_last, u64_mem_test_offset);
			printf("counter value difference minimum: %lu = %lu us.\n",
						u64_counter_last_min, u64_counter_last_min/IOU_SCNTR_FREQ_MICRO);
			printf("counter value difference maximum: %lu = %lu us.\n",
						u64_counter_last_max, u64_counter_last_max/IOU_SCNTR_FREQ_MICRO);
			printf("Counter value difference average: %lu = %lu us.\n",
						u64_counter_last_average, u64_counter_last_average/IOU_SCNTR_FREQ_MICRO);
			printf("Loop count: %lu.\n", ul_loop);
		}

    }

	// Print summary information.
	printf("\n");
	
	if( 1==ul_mode )
	{
		ul_delay_loop_average = ul_delay_loop_tot/ul_loop;
		printf("Delay %lu us, counter reading loop: %lu.\n", ul_delay_us, ul_delay_loop);
		printf("Delay %lu us, minimum counter reading loop: %lu.\n", ul_delay_us, ul_delay_loop_min);
		printf("Delay %lu us, maximum counter reading loop: %lu.\n", ul_delay_us, ul_delay_loop_max);
		printf("Delay %lu us, average counter reading loop: %lu.\n", ul_delay_us, ul_delay_loop_average);
	}
	
	u64_counter_last_average = u64_counter_last_tot/ul_loop;
	printf("Counter value read 1: %lu=0x%lx.\n", u64_counter_read1, u64_counter_read1);
	printf("Counter value read 2: %lu=0x%lx.\n", u64_counter_read2, u64_counter_read2);
	printf("Counter value difference data for %lu us delay: %lu, memory offset: %lu\n", 
				ul_delay_us, u64_counter_last, u64_mem_test_offset);
	printf("counter value difference minimum: %lu = %lu us.\n",
				u64_counter_last_min, u64_counter_last_min/IOU_SCNTR_FREQ_MICRO);
	printf("counter value difference maximum: %lu = %lu us.\n",
				u64_counter_last_max, u64_counter_last_max/IOU_SCNTR_FREQ_MICRO);
	printf("Counter value difference average: %lu = %lu us.\n",
				u64_counter_last_average, u64_counter_last_average/IOU_SCNTR_FREQ_MICRO);
	printf("Loop count: %lu.\n", ul_loop);

	//printf("File: %s, Func: %s, Line: %d.\n", __FILE__, __func__, __LINE__ );
    return 0;
}


int main(int argc, char **argv)
{

#ifdef APP_LINUX
	int fd_mem;
#endif // APP_LINUX
	
	latency_params_t params;
    volatile unsigned long *pu64_mapped_counter;
	
    unsigned long u64_counter_init_delay_begin;
    unsigned long u64_counter_init_delay_end;
    unsigned long u64_counter_init_delay_last;

	delay_params_t delay_params;
	
#ifdef APP_LINUX
	printf("Executable command:%s\n", argv[0] );
	printf("Process ID:%ld\n", (long)getpid( ) );
#endif // APP_LINUX
	printf("Compilation time:%s-%s\n\n", __DATE__,__TIME__);
	
	/*
	sizeof(int):4
	sizeof(unsigned long):8
	sizeof(long):8
	sizeof(long long):8
	sizeof(unsigned long long):8
	sizeof(void *):8
	*/
	printf("sizeof(int):%ld\n", sizeof(int) );
	printf("sizeof(unsigned long):%ld\n", sizeof(unsigned long) );
	printf("sizeof(long):%ld\n", sizeof(long) );
	printf("sizeof(long long):%ld\n", sizeof(long long) );
	printf("sizeof(unsigned long long):%ld\n", sizeof(unsigned long long) );
	printf("sizeof(void *):%ld\n\n", sizeof(void *) );
	//printf("File: %s, Func: %s, Line: %d.\n", __FILE__, __func__, __LINE__ );

    /* sanity check */
    if(argc<2) 
	{
        printf("Usage: dpdlat test-time-in-seconds loop-dealy-time-in-us cpu-affinity-bit-mask test-mode test-memory-MB\n");
        printf("time-in-seconds(Default: 10000 seconds)\n");
        printf("loop-dealy-time-in-us(Default: 100 us)\n");
        printf("cpu-affinity-bit-mask(Default: 8, CPU 4)\n");
        printf("test-mode(1: busy delay, 2: sleep, Default: 1)\n");
        printf("test-memory-MB(memory size used by test, minimum: 2MB, maximum: 128MB)\n");

		params.ul_test_seconds = 10000;
    }
	else
	{
		params.ul_test_seconds = strtoul(argv[1], NULL, 10);
    }
	printf("Test %lu seconds\n", params.ul_test_seconds);

    if(argc<3) 
	{
		params.ul_delay_us = 100;
    }
	else
	{
		params.ul_delay_us = strtoul(argv[2], NULL, 10);
    }
	printf("Delay %lu us in loop\n", params.ul_delay_us);

    if(argc<4) 
	{
		params.ul_cpus = 8;
    }
	else
	{
		params.ul_cpus = strtoul(argv[3], NULL, 10);
    }
	printf("Test on CPUs: %lu=0X%lx\n", params.ul_cpus, params.ul_cpus);

    if(argc<5) 
	{
		params.ul_mode = 1; // busy delay
    }
	else
	{
		params.ul_mode = strtoul(argv[4], NULL, 10); // 1: busy delay, 2: sleep
    }
	printf("Test mode(1: busy delay, 2: sleep): %lu.\n", params.ul_mode);

    if(argc<6) 
	{
		params.ul_memory_MB = 16; // 
    }
	else
	{
		params.ul_memory_MB = strtoul(argv[5], NULL, 10); // 1: busy delay, 2: sleep
    }
	printf("Test memory size: %lu MB.\n", params.ul_memory_MB);

#ifdef APP_LINUX
    //fd_mem = open("/dev/mem", O_RDWR | O_SYNC);
    fd_mem = open("/dev/mem", O_RDWR | O_SYNC);
    if (fd_mem == -1) {
        printf("Can't open /dev/mem.\n");
		//printf("File: %s, Func: %s, Line: %d\n", __FILE__, __func__, __LINE__ );
        exit(-1);
    }
    printf("/dev/mem opened.\n");
	//printf("File: %s, Func: %s, Line: %d.\n", __FILE__, __func__, __LINE__ );

    /* Map SLCR register space into user space */
    //pu64_mapped_counter = mmap(0, IOU_SCNTR_ADRS_SIZE, PROT_READ | PROT_WRITE, MAP_SHARED, 
    //		fd_mem, IOU_SCNTR_ADRS_BASE  & ~MAP_MASK);
    pu64_mapped_counter = mmap(0, IOU_SCNTR_ADRS_SIZE, PROT_READ, MAP_SHARED, 
    		fd_mem, IOU_SCNTR_ADRS_BASE  & ~MAP_MASK);
    if (pu64_mapped_counter == (volatile unsigned long *) -1) {
        printf("Can't map system counter registers to user space.\n");
        goto fail_map;

    }
    printf("Mapped system counter registers at virtual address %p.\n", pu64_mapped_counter);
#else
    printf("Use system counter registers as physical address.\n");
	pu64_mapped_counter = (volatile unsigned long *)IOU_SCNTR_ADRS_BASE;
#endif   /* APP_LINUX */

    printf("System counter frequency: %d.\n", IOU_SCNTR_FREQ);
    printf("System counter value 1: %lu.\n", *pu64_mapped_counter);
    printf("System counter value 2: %lu.\n", *pu64_mapped_counter);
	params.pu64_mapped_counter = pu64_mapped_counter;
    usleep(100000);

	// Test counter value for 1 seocnd delay
	u64_counter_init_delay_begin = *pu64_mapped_counter;
    sleep(1);
	u64_counter_init_delay_end = *pu64_mapped_counter;
    if( u64_counter_init_delay_end > u64_counter_init_delay_begin ) 
	{
		u64_counter_init_delay_last = u64_counter_init_delay_end -u64_counter_init_delay_begin;
    }
	else
	{
		u64_counter_init_delay_last = u64_counter_init_delay_begin -u64_counter_init_delay_end;
    }
    printf("System counter value begin for 1 seconds delay: %lu.\n", u64_counter_init_delay_begin);
    printf("System counter value end for 1 seconds delay: %lu.\n", u64_counter_init_delay_end);
    printf("System counter value difference for 1 seconds delay: %lu.\n", u64_counter_init_delay_last);
	//printf("File: %s, Func: %s, Line: %d.\n", __FILE__, __func__, __LINE__ );

	u64_counter_init_delay_begin = *pu64_mapped_counter;
	udelay_sys_counter( pu64_mapped_counter, 0, &delay_params );
	u64_counter_init_delay_end = *pu64_mapped_counter;
    if( u64_counter_init_delay_end > u64_counter_init_delay_begin ) 
	{
		u64_counter_init_delay_last = u64_counter_init_delay_end -u64_counter_init_delay_begin;
    }
	else
	{
		u64_counter_init_delay_last = u64_counter_init_delay_begin -u64_counter_init_delay_end;
    }
    printf("System counter value begin for udelay_sys_counter(): %lu.\n", u64_counter_init_delay_begin);
    printf("System counter value end for udelay_sys_counter(): %lu.\n", u64_counter_init_delay_end);
    printf("System counter value difference for udelay_sys_counter(): %lu.\n", u64_counter_init_delay_last);

	// Test latency. Call or Thread?
	latency_check( &params );
	
#ifdef APP_LINUX
fail_map:
	//printf("fail_map, File: %s, Func: %s, Line: %d\n", __FILE__, __func__, __LINE__ );
	munmap( (void *)pu64_mapped_counter, IOU_SCNTR_ADRS_SIZE);
    close(fd_mem);
#endif   /* APP_LINUX */
    return 0;
}
