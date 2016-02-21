/**
 * This program is to read IOTime.
 * IOTime: pcie ptp-card time from driver directly
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
#include <sys/time.h> // struct itimeral. setitimer()  

#define MY_HOUR 10
#define MY_MIN 55 
#define MY_SEC 0

#define ADJ_FREQ_MAX 512000

#define OFFSET_1980 315532800
#define OFFSET_1990 631152000
#define OFFSET_2000 946684800
#define OFFSET_2010	1262304000

#define IO_MAGIC '='
//IOCTL
//It may get the pcie time from pcie_host_driver
//timespec struct define
//tv_sec: seconds
//tv_nsec:nanoseconds
#define HOST_GET_PCIE_TIME _IOR(IO_MAGIC,1,struct timespec)
#define HOST_GET_LOCAL_SYSTEM_TIME _IOR(IO_MAGIC,2,struct timespec)

#define PCIE_DEV "/dev/pcietime0"

void get_system_time (struct timespec *time);//Get current system time
void get_pcie_time (struct timespec *time); // Get the pcie rtc time 
void printMsg(int);  

static int fd;
static int rd_num = 0;
void get_system_time (struct timespec *time)
{
	ioctl (fd, HOST_GET_LOCAL_SYSTEM_TIME, time);
}

void get_pcie_time (struct timespec *time) 
{
	ioctl(fd, HOST_GET_PCIE_TIME, time);
}

void printMsg(int num) {
   struct timespec timePCIE;
   get_pcie_time(&timePCIE);
   printf("called pcie_time %ld(s).%09lu(ns), (%d).\n", \
		 timePCIE.tv_sec, timePCIE.tv_nsec, rd_num++);
}  

int main (int argc,char *argv[])
{
	  // Get system call result to determine successful or failed   
    int res = 0;
    unsigned long long utime;
  	// Register printMsg to SIGALRM    
  	signal(SIGALRM, printMsg); 
    struct timeval tv;
	  struct timezone tz;       
  	struct itimerval tick;     
  	// Initialize struct  	  
	  memset(&tick, 0, sizeof(tick));
    time_t timep;
	  struct tm *tmp;
	  time(&timep);
	  //printf("time() : %d \n",timep);
	  tmp=localtime(&timep);
	  tmp->tm_hour = MY_HOUR; 
          tmp->tm_min = MY_MIN;
          tmp->tm_sec = MY_SEC;
          printf("my_timer set time at %02d-%02d-%02d %02d:%02d:%02d\n", \
          tmp->tm_year+1900, tmp->tm_mon+1, tmp->tm_mday, \
	  tmp->tm_hour, tmp->tm_min, tmp->tm_sec);
	  timep = mktime(tmp); 
          gettimeofday (&tv , &tz);
          utime = timep * 1000000 - (tv.tv_sec * 1000000 + tv.tv_usec);	  
	  // Timeout to run function first time  	  
	  tick.it_value.tv_sec = utime / 1000000;  // sec  	  
	  tick.it_value.tv_usec = utime % 1000000; // micro sec.  	  
	  // Interval time to run function  	  
	  tick.it_interval.tv_sec = 1;  	  
	  tick.it_interval.tv_usec = 0;  	  
	  // Set timer, ITIMER_REAL : real-time to decrease timer,  	  
	  //                          send SIGALRM when timeout  	  
	  res = setitimer(ITIMER_REAL, &tick, NULL);  	  
	  if (res) {  	  
	    printf("Set timer failed!!/n");  	  
	  }   
	  fd = open (PCIE_DEV, O_RDWR);
	  if (fd == -1) {
		printf ("Please check the PCIE card and try again!\n");
		return -1;
	 }
	 // Always sleep to catch SIGALRM signal   
 	 while(1) {  
    		pause();  
  	 }  	 
	 return 0;
		
}


