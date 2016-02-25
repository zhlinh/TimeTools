/**
 * This program is to add CPU load.
 * 
 * USAGE:
 * gcc -o [filename] [filename].c -lrt
 *
 * if you want to assign CPU:
 * sudo taskset -c [cpu list, like 0, 1, 2...] ./load
 *  
*/

#include <sys/timex.h>
#include <sys/fcntl.h>
#include <sys/mman.h>
#include <time.h>
#include <unistd.h>
#include <stdio.h>
#include <syslog.h>
#include <string.h>
#include <errno.h>
#include <sys/stat.h>
#include <stdlib.h>
#include <linux/ioctl.h> 
#include <signal.h>   // for signal()   
#include <sys/time.h> // struct itimeral

#define CTIMES 230000000  // cpu-load 40% -- 50%

void printMsg(int);  

void printMsg(int num) {
    int i, b;
    int a = 16;
    
    printf ("coutting stars......%d times per second.\n", CTIMES);
    for(i = 0; i < CTIMES; i++) {
        b = a * a;
    }
}  

int main (int argc,char *argv[])
{
	// Get system call result to determine successful or failed   
    int res = 0;
  	// Register printMsg to SIGALRM    
  	signal(SIGALRM, printMsg);       
    struct itimerval tick;     
  	// Initialize struct  	  
	memset(&tick, 0, sizeof(tick));

	// Timeout to run function first time  	  
	tick.it_value.tv_sec = 0;  // sec  	  
	tick.it_value.tv_usec = 1000; // micro sec.  	  
	// Interval time to run function  	  
	tick.it_interval.tv_sec = 1;  	  
    tick.it_interval.tv_usec = 0;  	  
    // Set timer, ITIMER_REAL : real-time to decrease timer,  	  
    //                          send SIGALRM when timeout  	  
    res = setitimer(ITIMER_REAL, &tick, NULL);  	  
    if (res) {  	  
        printf("Set timer failed!!/n");  	  
    }   
    // Always sleep to catch SIGALRM signal   
    while(1) {  
        pause();  
    }
    return 0;

}


