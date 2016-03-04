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

#define ADJ_FREQ_MAX  512000
#define PCIE_DEV "pcietime0"

#define OFFSET_1980     315532800
#define OFFSET_1990 	631152000
#define OFFSET_2000  	946684800
#define OFFSET_2010		1262304000

#define IO_MAGIC '='
//IOCTL
//It may get the pcie time from pcie_host_driver
//timespec struct define
//tv_sec: seconds
//tv_nsec:nanoseconds
#define HOST_GET_PCIE_TIME _IOR(IO_MAGIC,1,struct timespec)
#define HOST_GET_LOCAL_SYSTEM_TIME _IOR(IO_MAGIC,2,struct timespec)

void get_system_time (struct timespec *time);//Get current system time
void get_pcie_time (struct timespec *time); // Get the pcie rtc time 


static int fd;
void get_system_time (struct timespec *time)
{
	ioctl (fd, HOST_GET_LOCAL_SYSTEM_TIME, time);
}

void get_pcie_time (struct timespec *time) 
{
	ioctl (fd, HOST_GET_PCIE_TIME, time);
}

 
int main (int argc,char *argv[])
{
	  struct timespec timeSystem;
	  struct timespec timePCIE;
	  int ret;  
	  fd = open (PCIE_DEV,O_RDWR);
	  if (fd == -1) {
		printf ("Please check the PCIE card and try again!\n");
		return -1;
	 }
	 
	 while (1)
	 {
		get_system_time(&timeSystem); 
		get_pcie_time(&timePCIE);
		printf ("sysTime %ld(s).%09lu(ns), pcieTime %ld(s).%09lu(ns), offset %ld(s).%09lu(ns) \n",timeSystem.tv_sec,timeSystem.tv_nsec,
			timePCIE.tv_sec,timePCIE.tv_nsec, timePCIE.tv_sec - timeSystem.tv_sec, timePCIE.tv_nsec - timeSystem.tv_nsec );
		sleep(1);
	 }
		
}

