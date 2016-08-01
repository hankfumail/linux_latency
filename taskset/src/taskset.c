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
#include <sched.h>  
#include <pthread.h>   
#include <assert.h>  
#include <errno.h>  


int main(int argc, char **argv)
{
    cpu_set_t cpuset;
    cpu_set_t cpuset2;
    int i_pid;
    int ret, j, num_cpus, core;
    pid_t pid;

	// ps | cut -c 2-5 | xargs /mnt/taskset 
	printf("Executable command:%s\n", argv[0] );
	printf("Process ID:%ld\n", (long)getpid( ) );
	printf("Compilation time:%s-%s\n\n", __DATE__,__TIME__);
	printf("EFAULT:%d\n", EFAULT);
	printf("EINVAL:%d\n", EINVAL);
	printf("EPERM:%d\n", EPERM);
	printf("ESRCH:%d\n", ESRCH);	

    num_cpus = sysconf(_SC_NPROCESSORS_CONF);/*get the cpu nums*/
    printf("cpu num:%d\n", num_cpus);
    assert(num_cpus > 0);

	if(argc<2) {
		 printf("Usage: taskset pid cpu_core_mask_in_hex\n");
		 exit(0);
	 }
	if(argc==3) {
		core = strtoul(argv[2], NULL, 16);
	}
	else{
		core = 3; //default
	}
	printf("cpu core mask:%x\n", core);
	i_pid = strtoul(argv[1], NULL, 10);
	printf("PID:%d\n", i_pid);
	pid = (pid_t)i_pid;

    CPU_ZERO(&cpuset);
    for (j = 0; (j < CPU_SETSIZE) && (j < num_cpus); j++)
    {
           if ( 0 != ( core&(1<<j) ) )
           {
        	   printf("set program bind on CPU %d\n", j);
			   CPU_SET(j, &cpuset);
           }
    }

    //printf("cpu cpuset mask:%x\n",  (unsigned int)cpuset);

    ret = sched_setaffinity(pid, sizeof(cpu_set_t), &cpuset);
	printf("sched_setaffinity return: %d\n", ret);
	printf("errno: %d\n", errno);
    //assert(ret == 0);


    ret = sched_getaffinity(pid, sizeof(cpu_set_t), &cpuset2);
	printf("sched_getaffinity return: %d\n", ret);
	printf("errno: %d\n", errno);
    //assert(ret == 0);
    //printf("cpu cpuset2 mask:%x\n", (unsigned int)cpuset2);

    for (j = 0; (j < CPU_SETSIZE) && (j < num_cpus); j++)
    {
    		if (CPU_ISSET(j, &cpuset))
           {
        	   printf("after set program bind on CPU %d\n", j);
           }
    }

    return 0;
}
