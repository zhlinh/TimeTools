/**
 * This program is to sync pcie_time and system_time by hypercall
 * 
 * USAGE:
 * gcc -o [filename] [filename].c -lrt
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

#define OFFSET_1980 315532800
#define OFFSET_1990 631152000
#define OFFSET_2000 946684800
#define OFFSET_2010 1262304000

#define IO_MAGIC '='
#define HOST_GET_PCIE_TIME _IOR (IO_MAGIC, 1, struct timespec)
#define HOST_GET_LOCAL_SYSTEM_TIME _IOR (IO_MAGIC, 2, struct timespec)
#define HOST_GET_OFFSET _IOR (IO_MAGIC, 4, long long )

#define	TMR_CNT_L (0x24e1c >> 2)
#define	TMR_CNT_H (0x24e18 >> 2)


#define LOCKFILE "/var/run/PcieSync.pid"

#define LOCKMODE (S_IRUSR | S_IWUSR | S_IRGRP | S_IROTH)

#define NSEC 1000000000

// CHANGED: add vmcall definition
#define KVM_HC_GET_PCIE_TIME 100
#define KVM_HYPERCALL ".byte 0x0f,0x01,0xc1"
#define TAI_OFFSET 35

static int fd;
int lockfile(int fd)
{
    struct flock fl;

    fl.l_type = F_WRLCK;
    fl.l_start = 0;
    fl.l_whence = SEEK_SET;
    fl.l_len = 0;
    return(fcntl(fd, F_SETLK, &fl));
}

int already_running(const char *filename)
{
    int fd;
    char buf[16];

    fd = open(filename, O_RDWR | O_CREAT, LOCKMODE);
    if (fd < 0) {
        syslog(LOG_ERR, "can't open %s: %m\n", filename);
        exit(1);
    }
    if (lockfile(fd) == -1) {
        if (errno == EACCES || errno == EAGAIN) {
            syslog(LOG_ERR, "file: %s already locked", filename);
            close(fd);
            return 1;
        }
        syslog(LOG_ERR, "can't lock %s: %m\n", filename);
        exit(1);
    }
    ftruncate(fd, 0);
    sprintf(buf, "%ld", (long)getpid());
    write(fd, buf, strlen(buf) + 1);
    return 0;
}


void get_time(struct timespec *time)
{
    //ioctl(fd, HOST_GET_LOCAL_SYSTEM_TIME, time);
    clock_gettime(CLOCK_REALTIME, time);
}

// CHANGED: add get_pcie_time
void get_pcie_time(struct timespec *time)
{
    //ioctl(fd, HOST_GET_PCIE_TIME, &time);
    unsigned long ret, rete;
    unsigned  nr = KVM_HC_GET_PCIE_TIME;

    asm volatile(KVM_HYPERCALL
            : "=a"(ret),"=b"(rete)
            : "a"(nr)
            :"memory");
    time->tv_sec = ret;
    time->tv_nsec = rete;
}

// CHANGED: add get_offset
void get_offset(long long int *offset) {
    unsigned long long tmr_cnt;
    unsigned long long cur_cnt;
    struct timespec tmr_time;
    struct timespec cur_time;

    get_time(&cur_time);
    get_pcie_time(&tmr_time);
    cur_cnt = (unsigned long long)cur_time.tv_sec * NSEC + cur_time.tv_nsec;
    tmr_cnt = (unsigned long long)tmr_time.tv_sec * NSEC + tmr_time.tv_nsec;
    // PtpCard design
    tmr_cnt -= TAI_OFFSET;
    if (tmr_cnt > cur_cnt) {
        *offset = (-(tmr_cnt - cur_cnt));
    } else {
        *offset = (cur_cnt - tmr_cnt);
    }
}

int set_time(struct timespec *time)
{
    return clock_settime(CLOCK_REALTIME,time);
}


static void normalize_time(struct timespec *result)
{
    result->tv_sec += result->tv_nsec / NSEC;
    result->tv_nsec -= result->tv_nsec / NSEC * NSEC;

    if (result->tv_sec > 0 && result->tv_nsec < 0) {
        result->tv_sec -= 1;
        result->tv_nsec += NSEC;
    } else if(result->tv_sec < 0 && result->tv_nsec > 0) {
        result->tv_sec += 1;
        result->tv_nsec -= NSEC;
    }
}

static void add_time(struct timespec *result, struct timespec *x, struct timespec *y)
{
    result->tv_sec = x->tv_sec + y->tv_sec;
    result->tv_nsec = x->tv_nsec + y->tv_nsec;

    normalize_time(result);
}

static void sub_time(struct timespec *result, struct timespec *x, struct timespec *y)
{
    result->tv_sec = x->tv_sec - y->tv_sec;
    result->tv_nsec = x->tv_nsec - y->tv_nsec;

    normalize_time(result);
}

static int adj_freq (int adj)
{
    struct timex t;

    //printf ("-----------------adj %d\n",adj);
    t.modes = MOD_FREQUENCY;
    t.freq = adj * ((1 << 16) / 1000);
    return adjtimex (&t);
}

void update_clock (struct timespec *offset)
{
    int adj = 0;
    static int ap = 2;
    static int ai = 10;
    static int observed_drift = 0;


#define	THRESHOLD	100000000   //100ms

    if(offset->tv_sec || abs (offset->tv_nsec) > THRESHOLD) {
        struct timespec timeTmpA;
        struct timespec timeTmpB;
        struct timespec timeTmpC;
        struct timespec timeTmpD;
        struct timespec timeTmpE;
        struct timespec timeTmpF;

        get_time (&timeTmpA);   // Get current time #1
        get_time (&timeTmpB);   // Get current time #2
        sub_time (&timeTmpC, &timeTmpB, &timeTmpA);
        get_time (&timeTmpD);   // Get current time #4
        sub_time (&timeTmpE, &timeTmpD, offset);
        add_time (&timeTmpF, &timeTmpE, &timeTmpC);
        set_time (&timeTmpF);
        //printf ("-------------timeTmpF %d.%d\n", \
        //        timeTmpF.tv_sec,timeTmpF.tv_nsec);
    } else {
        /*
         * Offset from master is less than one second.  Use the the PI controller
         * to adjust the time
         */

        /* no negative or zero attenuation */

        /* the accumulator for the I component */
        observed_drift += offset->tv_nsec / ai;

        /* clamp the accumulator to ADJ_FREQ_MAX for sanity */
        if (observed_drift > ADJ_FREQ_MAX) {
            observed_drift =  ADJ_FREQ_MAX;
        } else if (observed_drift < -ADJ_FREQ_MAX) {
            observed_drift = -ADJ_FREQ_MAX;
        }
        adj = offset->tv_nsec / ap + observed_drift;
        /* apply controller output as a clock tick rate adjustment */
        adj_freq (-adj);
    }
    //printf ("offset %d.%d observed_drift %d adj %d\n", \
    //        offset->tv_sec,offset->tv_nsec,observed_drift,adj);
}

int main (int argc,char *argv[])
{

    long long offset;
    struct timespec timeSystem;
    struct timespec timeTemp;
    struct timespec timePCIE;
    struct timespec offset_from_PCIE;
    int ret;
    int displayResult = 0;
    int displayPCIETime = 0;
    int sync_period = 10000;
    if (argc >= 2) {
        int c;
        while((c = getopt(argc, argv, "dvts?h")) != -1) {
            switch(c) {
                case 'h':case '?':
                    printf(
                            "\nUsage:  %s [OPTION]\n\n"
                            "-h \t\t\t\t show the command line help information\n"
                            "\n"
                            "-d	\t\t\t run the pcie sync in the non-daemon mode\n"
                            "\n"
                            "-t	\t\t\t display current pcie time\n"
                            "\n"
                            "-v	\t\t\t display sync result\n"
                            "\n"
                            "-s	\t\t\t set sync period (us)\n"
                            , argv[0]);
                    return 0;
                case 'd':
                    if(already_running(LOCKFILE)) {
                        printf ("Sync Process has already running.\n"
                                "Use: %s -t to display\n", argv[0]);
                        return;
                    }
                    if(sync_period >= 10000) {
                        printf ("----------Sync process Successfully,\n"
                                "sync period is %d ms\n", sync_period / 1000);
                    } else {
                        printf ("----------Sync process failed,\n"
                                "sync period %d is too short,\n"
                                "please try again!!\n", sync_period);
                    }
                    if (daemon (0,0) < 0) {
                        perror ("Failed to be a daemon.");
                        return -1;
                    }
                    break;
                case 't':
                    displayPCIETime = 1;
                    break;
                case 'v':
                    displayResult = 1;
                    break;
                case 's':
                    if(argv[2]!= NULL) {
                        sync_period = atoi(argv[2]);
                        if(sync_period >= 10000) {
                            printf ("Set period ok,\n"
                                    "sync period is %d ms\n",sync_period / 1000);
                        } else {
                            printf ("Set sync period failed,\n"
                                    "Please check sync period[need > 10ms]! \n");
                            return -1;
                        }
                    } else {
                        printf ("Set sync period failed,\n"
                                "please check sync period[need > 10ms]! \n");
                        return -1;
                    }
                    break;
                default:
                    break;
            }
        }
    } else {
        printf(
                "\nUsage:  %s [OPTION]\n\n"
                "-h \t\t\t\t show the command line help information\n"
                "\n"
                "-d	\t\t\t run the pcie sync in the non-daemon mode\n"
                "\n"
                "-t	\t\t\t display current pcie time\n"
                "\n"
                "-v	\t\t\t display sync result\n"
                "\n"
                "-s	\t\t\t set sync period\n"
                "\n"
                , argv[0]);
        return -1;
    }

    //fd = open (PCIE_DEV, O_RDWR);
    if (fd == -1) {
        printf ("Please check the PCIE card and try again.\n" \
                "For Help input: sync -h \n");
        return -1;
    }

    if (!displayPCIETime && already_running(LOCKFILE)) {
        printf ("Sync Process has already been started.\n"
                "Use: sync -h  to display \n");
        return 0;
    }

    while (1) {
        get_time (&timeSystem);
        get_pcie_time(&timePCIE);
        get_offset(&offset);
        if(displayPCIETime && already_running(LOCKFILE)) {
            offset_from_PCIE.tv_sec = offset / NSEC;
            offset_from_PCIE.tv_nsec = offset % NSEC;
            printf ("----------Syncing process is running, "
                    "offset is  %ld (ns) per %d (ms)\n",
                    offset_from_PCIE.tv_nsec,sync_period /1000);
        } else {
            offset_from_PCIE.tv_sec = offset / NSEC;
            offset_from_PCIE.tv_nsec = offset % NSEC;
            //if(timePCIE.tv_sec < OFFSET_1990)
            //{
            //	if(displayResult)
            //		printf ("----------The Master clock has not been ready! Please wait or Use sync -h to try again. \n");
            //}else
            {
                update_clock (&offset_from_PCIE);
                if(displayResult) {
                    printf ("----------Sync Success, Current Offset "
                            "is %ld (ns)\n", offset_from_PCIE.tv_nsec);
                }
            }
        }
        //It need set the sync period > 10 ms
        if(sync_period >=10000) {
            usleep (sync_period);
        } else {
            break;
        }
    }
}
