/**
 * This program is to read time(SystemTime, IOTime, HyperCallTime)
 * and transmit it through TCP. 
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

#include <sys/types.h>
#include <sys/socket.h>
#include <arpa/inet.h>

// default interval 5s
#define DE_IV 5000000

// define ns
#define NSEC 1000000000

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

#define LOG_FILE "tcp_timer.log"
#define PORT 8888

typedef unsigned char byte;

// get_time methods
void get_system_time (struct timespec *time);//Get current system time
void get_io_pcie_time (struct timespec *time); // Get the pcie time from driver directly
void get_hc_pcie_time (struct timespec *time); // Get the pcie time from hypercall

static void normalize_time(struct timespec *result);
static void sub_ns(struct timespec *result, struct timespec *x, long y);

void int_to_byte(int, unsigned char *);
void print_msg(int);  

static int rd_num = 0;
int s;  // socket descriptor
int fd = -1;  // file descriptor
int mode = 0; // define getting what time
char *mode_name = "system_time"; // mode name

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

void int32_to_byte(unsigned int i, unsigned char *bytes)
{
    int size = 4;
    memset(bytes, 0, sizeof(unsigned char) * size);
    bytes[0] = (unsigned char)(0xff & i);
    bytes[1] = (unsigned char)((0xff00 & i) >> 8);
    bytes[2] = (unsigned char)((0xff0000 & i) >> 16);
    bytes[3] = (unsigned char)((0xff000000 & i) >> 24);
}

void int64_to_byte(long long i, unsigned char *bytes)
{
    int size = 8;
    memset(bytes, 0, sizeof(unsigned char) * size);
    bytes[0] = (unsigned char)(0xff & i);
    bytes[1] = (unsigned char)((0xff00 & i) >> 8);
    bytes[2] = (unsigned char)((0xff0000 & i) >> 16);
    bytes[3] = (unsigned char)((0xff000000 & i) >> 24);
    bytes[4] = (unsigned char)((0xff00000000 & i) >> 32);
    bytes[5] = (unsigned char)((0xff0000000000 & i) >> 40);
    bytes[6] = (unsigned char)((0xff000000000000 & i) >> 48);
    bytes[7] = (unsigned char)((0xff00000000000000 & i) >> 56);
}

void print_msg(int num) {
    struct timespec systime;
    struct timespec time;
    struct timespec fixedtime;
    FILE *fp;
    unsigned char buf[50];  // data buffer
    //time_t timep;
    struct tm *tmp;

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
    bzero(buf, sizeof(buf));
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
        // buf[12]-buf[19] is tv_sec and buf[20] - buf[23] is tc_sec, LE
        int64_to_byte((long long)fixedtime.tv_sec, &buf[12]);
        int32_to_byte((unsigned int)fixedtime.tv_nsec, &buf[20]);   
    } else {
        //system time doesn't need fix
        printf("called %s %ld(s).%09lu(ns), (%d).\n", \
		        mode_name, time.tv_sec, time.tv_nsec, rd_num);
        fp = fopen(LOG_FILE, "a");
        fprintf(fp, "%ld, %09lu, %d\n", \
		        time.tv_sec, time.tv_nsec, rd_num);
        fclose(fp);
        // buf[12]-buf[19] is tv_sec and buf[20] - buf[23] is tc_sec, LE
        int64_to_byte((long long)time.tv_sec, &buf[12]);
        int32_to_byte((unsigned int)time.tv_nsec, &buf[20]);   
    }

    tmp = localtime(&systime.tv_sec);
    // buf[0]-buf[11] is hour, min, sec, LE
    int32_to_byte((unsigned int)tmp->tm_hour, buf);
    int32_to_byte((unsigned int)tmp->tm_min, &buf[4]);
    int32_to_byte((unsigned int)tmp->tm_sec, &buf[8]);
   
    buf[24] = '\0';
    write(s, buf, sizeof(buf));
}

void usage(char *file) {
    printf(
           "Usage: [sudo] %s [OPTION]\n\n"
           "-h \t\t show help information.\n"
           "-t \t\t set the timer start point.(default next secs(secs%%5==0))\n"
           "-i \t\t set the timer interval.(default 5s)\n"
           "-a \t\t set the tcp server address.\n"
           "-p \t\t set the tcp server port.(default 8888)\n"
           "-m \t\t set the mode to get time(default 0) :\n"
           "\t\t\t m : 0 means system time.\n"
           "\t\t\t m : 1 means pcie time from driver directly.\n"
           "\t\t\t m : 2 means pcie time from hypercall.\n"
           "\nExample: sudo %s -t 8:30:0 -i 5.3 -m 1\n", \
           file, file);
}

int main (int argc,char *argv[])
{
    // Get system call result to determine successful or failed   
    int res = 0;  // return result
    int ch;  // get char from command line
    unsigned long long utime;
    long ivtime = DE_IV;
    struct timeval tv;
    struct timezone tz;
    struct itimerval tick;    
    // Initialize struct  	  
    memset(&tick, 0, sizeof(tick));
    time_t timep;
    struct tm *tmp;
    char *substr;
    int flag = 1;
    char *tcpaddr;
    int tcpport = PORT;
    FILE *fp;
    
    struct sockaddr_in server_addr;	 // server address struct
    char buffer[1024];
    // Register print_msg to SIGALRM    
    signal(SIGALRM, print_msg); 
    time(&timep);
    //printf("time() : %d \n",timep);
    tmp = localtime(&timep);

    // read the command line
    if (argc >= 2) {
        while((ch = getopt(argc, argv, "t:i:a:p:m:h")) != -1) {
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
                    ivtime = (long)(atof(optarg) * 1000000);   
                    break;
                case 'a':
                    tcpaddr = optarg;
                    break;
                case 'p':
                    tcpport = atoi(optarg);
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
                    break;
                default:
                    break;
             }
         }
    } else {
        usage(argv[0]);
        return 0;
    }
    // Interval time to run function  	  
    tick.it_interval.tv_sec = ivtime / 1000000;   // sec. 	  
    tick.it_interval.tv_usec = ivtime % 1000000;  // usec. 
    
    if (flag) {
        // next secs(secs % 5 == 0)
        printf("it will start in 5 secs......\n");
        tmp->tm_sec = tmp->tm_sec - (tmp->tm_sec) % 5 + 5;
    }
    // Timeout to run function first time 
    timep = mktime(tmp); 
    gettimeofday(&tv, &tz);
    utime = timep * 1000000 - (tv.tv_sec * 1000000 + tv.tv_usec);	   	  
    tick.it_value.tv_sec = utime / 1000000;  // sec.  	  
    tick.it_value.tv_usec = utime % 1000000; // usec.
   	  
    // Set timer, ITIMER_REAL : real-time to decrease timer,  	  
    //                          send SIGALRM when timeout  	  
    res = setitimer(ITIMER_REAL, &tick, NULL);  	  
    if (res) {  	  
        printf("Set timer failed!!\n");  	  
    }

    // print the timer info. 
    printf("\tset timer to get %s at %02d-%02d-%02d %02d:%02d:%02d\n", \
        mode_name, tmp->tm_year+1900, tmp->tm_mon+1, tmp->tm_mday, \
        tmp->tm_hour, tmp->tm_min, tmp->tm_sec);
    fp = fopen(LOG_FILE, "a");
	  fprintf(fp, "\n============================================================\n"
            "\tset timer to get %s at %02d-%02d-%02d %02d:%02d:%02d\n", \
            mode_name, tmp->tm_year+1900, tmp->tm_mon+1, tmp->tm_mday, \
            tmp->tm_hour, tmp->tm_min, tmp->tm_sec);
	  fclose(fp);

    printf("\twith interval %ld(s).%ld(us)\n", \
    tick.it_interval.tv_sec, tick.it_interval.tv_usec);   
    
    fp = fopen(LOG_FILE, "a");
    fprintf(fp, "\twith interval %ld(s).%ld(us)\n"
        "============================================================\n",
        tick.it_interval.tv_sec, tick.it_interval.tv_usec);
    fclose(fp);  

    if (mode == 1) {
        fd = open(PCIE_DEV, O_RDWR);
        if (fd == -1) {
            printf ("Can not open PCIE card, check and try again!\n");
            return -1;
        }
    }
    
    // build tcp socket
    s = socket(AF_INET, SOCK_STREAM, 0);
    if(s < 0){
		    printf("socket error\n");
		    return -1;
	  }	
    bzero(&server_addr, sizeof(server_addr));  // set zero
	  server_addr.sin_family = AF_INET;  // protocal family
	  server_addr.sin_addr.s_addr = htonl(INADDR_ANY);  // local address
	  server_addr.sin_port = htons(tcpport);  // server port

    // turn the address string input from user into integer 
    inet_pton(AF_INET, tcpaddr, &server_addr.sin_addr);

    // connect to server
    connect(s, (struct sockaddr*)&server_addr, sizeof(struct sockaddr));

    // Always sleep to catch SIGALRM signal   
    while(1) {  
        pause();  
    }
    close(s);
    if (mode == 1) {
        close(fd);
    }  	 
    return 0;
		
}

