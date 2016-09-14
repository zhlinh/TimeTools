/**
 * This program is to read time(SystemTime, IOTime, HyperCallTime).
 * SystemTime: system time
 * IOTime: pcie ptp-card time from driver directly(ioctl)
 * HyperCallTime: pcie ptp-card time from hypercall
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
#include <signal.h>   // for signal()
#include <sys/time.h> // struct itimeral. setitimer()
#include <getopt.h>
#include <string.h>

// define ns
#define NSEC 1000000000

// default interval 5s
#define DE_IV 5



typedef void (*sighandler_t) (int);
#define ADJ_FREQ_MAX  512000

#define OFFSET_1980 315532800
#define OFFSET_1990 631152000
#define OFFSET_2000 946684800
#define OFFSET_2010 1262304000

#define IO_MAGIC '='
//IOCTL
//It may get the pcie time from pcie_host_driver
//timespec struct define
//tv_sec: seconds
//tv_nsec:nanoseconds
#define HOST_GET_PCIE_TIME _IOR(IO_MAGIC,1,struct timespec)
#define HOST_GET_LOCAL_SYSTEM_TIME _IOR(IO_MAGIC,2,struct timespec)

#define KVM_HC_GET_PCIE_TIME 100
#define KVM_HYPERCALL ".byte 0x0f,0x01,0xc1"
#define PCIE_DEV "/dev/pcietime0"

#define LOG_FILE "posix-timer.log"

// get_time methods
void get_system_time (struct timespec *time);//Get current system time
void get_io_pcie_time (struct timespec *time); // Get the pcie time from driver directly
void get_hc_pcie_time (struct timespec *time); // Get the pcie time from hypercall

static void normalize_time(struct timespec *result);
static void sub_ns(struct timespec *result, struct timespec *x, long y);

void printMsg(int signo);

static int rd_num = 0;

int fd = -1; // file descriptor
int mode = 0; // define getting what time
char *mode_name = "system_time"; // mode name
timer_t  my_posix_timer;
timer_t  mPrepareAsyncTimer;


/**
 *  get system time
 */
void get_system_time(struct timespec *time)
{
    //ioctl(fd, HOST_GET_LOCAL_SYSTEM_TIME, time);
    clock_gettime(CLOCK_REALTIME, time);
}

/**
 *  get pcie time from driver directly
 */
void get_io_pcie_time(struct timespec *time)
{
    ioctl(fd, HOST_GET_PCIE_TIME, time);
}

/**
 *  get pcie time from hypercall
 */
void get_hc_pcie_time(struct timespec *time)
{
    unsigned long ret, rete;
    unsigned  nr = KVM_HC_GET_PCIE_TIME;
    asm volatile(KVM_HYPERCALL
                 : "=a"(ret),"=b"(rete)
                 : "a"(nr)
                 :"memory");
    time->tv_sec = ret;
    time->tv_nsec = rete;
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

static void sub_ns(struct timespec *result, struct timespec *x, long y)
{
    result->tv_sec = x->tv_sec;
    result->tv_nsec = x->tv_nsec - y;
    normalize_time(result);
}

void printMsg(int signo)
{
    struct timespec systime;
    struct timespec time;
    struct timespec fixedtime;
    FILE *fp;

    // Here is what kind of time you want to get
    if (mode == 1) {
        // FIXED: systime.tv_nsec means the delay time from the appointment time.
        get_system_time(&systime);
        // pcie time from driver directly
        get_io_pcie_time(&time);
    } else if (mode == 2) {
        // FIXED: systime.tv_nsec means the delay time from the appointment time.
        get_system_time(&systime);
        // pcie time from hypercall
        get_hc_pcie_time(&time);
    } else {
        // system time, system time doesn't need fix
        get_system_time(&time);
        systime = time;
    }

    rd_num += 1;
    if (mode == 1 || mode == 2) {
        // sub delay time
        sub_ns(&fixedtime, &time, systime.tv_nsec);
        printf("\n====SYSTEM TIME: %ld(s).%09lu(ns|delay from appointment), (%d).\n", \
		        systime.tv_sec, systime.tv_nsec, rd_num);
        printf("called %s %ld(s).%09lu(ns), (%d).\n", \
		        mode_name, time.tv_sec, time.tv_nsec, rd_num);
        printf("called fixed %s %ld(s).%09lu(ns), (%d).\n", \
		        mode_name, fixedtime.tv_sec, fixedtime.tv_nsec, rd_num);
        fp = fopen(LOG_FILE, "a");
        fprintf(fp, "%ld, %09lu, %d\n", \
		        fixedtime.tv_sec, fixedtime.tv_nsec, rd_num);
        fclose(fp);
    } else {
        //system time doesn't need fix
        printf("called %s %ld(s).%09lu(ns), (%d).\n", \
		        mode_name, time.tv_sec, time.tv_nsec, rd_num);
        fp = fopen(LOG_FILE, "a");
        fprintf(fp, "%ld, %09lu, %d\n", \
		        time.tv_sec, time.tv_nsec, rd_num);
        fclose(fp);
    }
}

void usage(char *file)
{
    printf(
           "\nUsage: [sudo] %s -m <mode number> [-t <HH:mm:ss> -i <interval in seconds>]\n\n"
           "-h\t\t\t\tshow help information.\n"
           "-t <HH:mm:ss>\t\t\tset the timer start point.(default next secs(secs%%5==0))\n"
           "-i <interval in seconds>\tset the timer interval.(default 5s)\n"
           "-m <mode number>\t\tset the mode to get time(default 0) :\n"
           "\t\t\t\t\t-m 0 means system time.\n"
           "\t\t\t\t\t-m 1 means pcie time from driver directly.\n"
           "\t\t\t\t\t-m 2 means pcie time from hypercall.\n"
           "\nExample: sudo %s -t 8:30:0 -i 5.3 -m 2\n", \
           file, file);
}

int main(int argc,char *argv[])
{
    // Get system call result to determine successful or failed
    int res = 0;
    int ch;
    unsigned long long starttime;
    unsigned long long ivtime = (unsigned long long)DE_IV * NSEC;
    // Register printMsg to SIGALRM
    struct itimerval tick;
    // Initialize struct
    memset(&tick, 0, sizeof(tick));
    time_t timep;
    struct tm *tmp;
    char *substr;
    int flag = 1;
    FILE *fp;
    struct sigevent sev;
    struct itimerspec its;
    struct timespec curtime;

    time(&timep);
    //printf("time() : %d \n",timep);
    tmp = localtime(&timep);

    // read the command line
    if (argc >= 2) {
        while((ch = getopt(argc, argv, "t:i:m:h")) != -1) {
            switch(ch) {
                case 'h':
                    usage(argv[0]);
                    return 0;
                case 't':
                    substr = strtok(optarg, ":");
                    tmp->tm_hour = atoi(substr);
                    substr = strtok(NULL, ":");
                    tmp->tm_min = atoi(substr);
                    substr = strtok(NULL, ":");
                    tmp->tm_sec = atoi(substr);
                    // when as -t 12:30:05:04...
                    if (strtok(NULL, ":") != NULL) {
                        usage(argv[0]);
                        return -1;
                    }
                    flag = 0;
                    break;
                case 'i':
                    ivtime = (long)(atof(optarg) * NSEC);
                    break;
                case 'm':
                    mode = atoi(optarg);
                    if (mode != 0 && mode != 1 && mode != 2) {
                        usage(argv[0]);
                        return -1;
                    }
                    // define the mode_name
                    if (mode == 1) {
                        // pcie time from driver directly
                        mode_name = "io_pcie_time";
                    } else if (mode == 2) {
                        // pcie time from hypercall
                        mode_name = "hc_pcie_time";
                    } else {
                        // system time
                        mode_name = "system_time";
                    }
                default:
                    break;
             }
         }
    }

    // init timer
    //sev.sigev_notify = SIGEV_THREAD; // open new thread to handle timers expire
    //sev.sigev_notify_function = printMsg; // new thread entry
    sev.sigev_notify = SIGEV_SIGNAL; // signal way to recall handler
    sev.sigev_signo = SIGRTMIN;  // RT signal, really cool.
    sev.sigev_value.sival_ptr = &my_posix_timer;  // when there is many timer signal
    sev.sigev_notify_attributes = NULL;

    signal(SIGRTMIN, printMsg);

    /* create timer */
    if (timer_create (CLOCK_REALTIME, &sev, &my_posix_timer) == -1)
    {
        printf("timer_create, error");
        return -1;
    }

    /* Start the timer */
    // Interval time to run function
    its.it_interval.tv_sec = ivtime / NSEC;   // sec.
    its.it_interval.tv_nsec = ivtime % NSEC;  // nsec.


    if (flag) {
        // next secs(secs % 5 == 0)
        printf("it will start in 5 secs......\n");
        tmp->tm_sec = tmp->tm_sec - (tmp->tm_sec) % 5 + 5;
    }

    // Timeout to run function first time
    timep = mktime(tmp);
    clock_gettime(CLOCK_REALTIME, &curtime);
    starttime = timep * NSEC - (curtime.tv_sec * NSEC + curtime.tv_nsec);
    its.it_value.tv_sec = starttime / NSEC;  // sec.
    its.it_value.tv_nsec = starttime % NSEC; // nsec.

    if (timer_settime(my_posix_timer, 0, &its, NULL) == -1)
    {
        printf("timer_settime error");
        timer_delete(my_posix_timer);
        return -1;
    }

    // print the timer info.
    printf("\tset timer to get %s at %02d-%02d-%02d %02d:%02d:%02d\n", \
        mode_name, tmp->tm_year+1900, tmp->tm_mon+1, tmp->tm_mday, \
        tmp->tm_hour, tmp->tm_min, tmp->tm_sec);
    fp = fopen(LOG_FILE, "a");
    fprintf(fp, "\n====================================================================\n"
            "\tset timer to get %s at %02d-%02d-%02d %02d:%02d:%02d\n", \
            mode_name, tmp->tm_year+1900, tmp->tm_mon+1, tmp->tm_mday, \
            tmp->tm_hour, tmp->tm_min, tmp->tm_sec);
	  fclose(fp);

    printf("\twith interval %ld(s).%ld(ns)\n", \
        its.it_interval.tv_sec, its.it_interval.tv_nsec);

    fp = fopen(LOG_FILE, "a");
    fprintf(fp, "\twith interval %ld(s).%ld(ns)\n"
	"\tTIME FORMAT: seconds(from 1970-01-01), nanoseconds, ordinal\n"
        "====================================================================\n",
        its.it_interval.tv_sec, its.it_interval.tv_nsec);
    fclose(fp);

    if (mode == 1) {
        fd = open(PCIE_DEV, O_RDWR);
        if (fd == -1) {
            printf ("Can not open PCIE card, check and try again!\n");
            return -1;
        }
    }

    // Always sleep to catch SIGALRM signal
    while(1) {
        pause();
    }
    if (mode == 1) {
       close(fd);
    }
    return 0;
}

