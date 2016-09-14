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

#define ADJ_FREQ_MAX  512000
#define MASTER_TIMETRACEABLE_STATUS	0x08000000   //GPIO4代表timeTraceable标志位
#define GPIO_DATA_ADDRESS_OFFSET 0xc08

#define OFFSET_1980     315532800
#define OFFSET_1990 	631152000
#define OFFSET_2000  	946684800
#define OFFSET_2010	1262304000

#define IO_MAGIC '='
#define HOST_GET_PCIE_TIME _IOR(IO_MAGIC,1, struct timespec)
#define HOST_GET_LOCAL_SYSTEM_TIME _IOR(IO_MAGIC,2,struct timespec)
#define HOST_GET_OFFSET _IOR(IO_MAGIC,4, long long )

static int fd;

long main (int argc,char *argv[])
{

    long time_s, time_ns;
    struct timespec timePCIE;
    int ret;
    /* change the device name if PTP-Card device node name changes */
    fd = open("/dev/pcietime0", O_RDWR);
    if (fd == -1) {
        printf ("Please check the PCIE card and try again.\n");
        return -1;
    }
    ioctl(fd, HOST_GET_PCIE_TIME, &timePCIE);
    time_s = timePCIE.tv_sec;
    time_ns = timePCIE.tv_nsec;
    printf("%ld(s), %09lu(ns)\n", time_s, time_ns);
    return 0;
}

