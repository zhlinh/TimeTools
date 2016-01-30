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

#define DE_HOUR 12
#define DE_MIN 30
#define DE_SEC 0

// default interval 5s
#define DE_IV 5000000

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


void get_system_time (struct timespec *time);//Get current system time
void get_pcie_time (struct timespec *time); // Get the pcie rtc time 
void int_to_byte(int, unsigned char *);
void print_msg(int);  

static int rd_num = 0;
int s;  // socket descriptor
int fd;  // file descriptor

void get_system_time (struct timespec *time)
{
    //ioctl(fd, HOST_GET_LOCAL_SYSTEM_TIME, time);
    clock_gettime(CLOCK_REALTIME, time); 
}

void get_pcie_time (struct timespec *time) 
{
    ioctl(fd, HOST_GET_PCIE_TIME, time);
    /**
    unsigned long ret, rete;
    unsigned  nr = KVM_HC_GET_PCIE_TIME;
    asm volatile(KVM_HYPERCALL
                 : "=a"(ret),"=b"(rete)
                 : "a"(nr)
                 :"memory");
    time->tv_sec = ret;
    time->tv_nsec = rete;
    */
}

void int_to_byte(int i, unsigned char *bytes)
{
    int size = 4;
    memset(bytes, 0, sizeof(unsigned char) * size);
    bytes[0] = (unsigned char)(0xff & i);
    bytes[1] = (unsigned char)((0xff00 & i) >> 8);
    bytes[2] = (unsigned char)((0xff0000 & i) >> 16);
    bytes[3] = (unsigned char)((0xff000000 & i) >> 24);
}

void print_msg(int num) {
    struct timespec timePCIE;
    FILE *fp;
    unsigned char buf[50];  // data buffer
    time_t timep;
    struct tm *tmp;

    //get_pcie_time(&timePCIE);
    get_system_time(&timePCIE);
    time(&timep);
    tmp = localtime(&timep);
    rd_num += 1;
    printf("called pcie_time %09lu(s).%09lu(ns), (%d).\n", \
				timePCIE.tv_sec, timePCIE.tv_nsec, rd_num);
    fp = fopen(LOG_FILE, "a");
    fprintf(fp, "%lu, %09lu, %d\n", \
				timePCIE.tv_sec, timePCIE.tv_nsec, rd_num);
    fclose(fp);
    bzero(buf, sizeof(buf));
    // buf[0]-buf[11] is hour, min, sec, LE
    int_to_byte((int)tmp->tm_hour, buf);
    int_to_byte((int)tmp->tm_min, &buf[4]);
    int_to_byte((int)tmp->tm_sec, &buf[8]);

    // buf[12]-buf[19] is tv_sec and tc_sec, LE
    int_to_byte((int)timePCIE.tv_sec, &buf[12]);
    int_to_byte((int)timePCIE.tv_nsec, &buf[16]);   
   
    buf[20] = '\0';
    write(s, buf, sizeof(buf));
}

void usage(char *file) {
    printf(
           "Usage: [sudo] %s [OPTION]\n\n"
           "-h \t\t show help information.\n"
           "-t \t\t set the timer start point.\n"
           "-i \t\t set the timer interval (default 5s)\n"
           "\nExample: sudo %s -t 8:30:0 -i 5.3\n"
           , file, file);
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
    tmp->tm_hour = DE_HOUR; 
    tmp->tm_min = DE_MIN;
    tmp->tm_sec = DE_SEC;

    // read the command line
    if (argc >= 2) {
        while((ch = getopt(argc, argv, "t:i:a:p:hf")) != -1) {
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
                case 'f':
                    flag = 0;
                    break; 
                default:
                    break;
             }
         }
    } else {
        flag = 0;
    }
    // Interval time to run function  	  
    tick.it_interval.tv_sec = ivtime / 1000000;   // sec. 	  
    tick.it_interval.tv_usec = ivtime % 1000000;  // usec. 
    
    if (flag) {
        // Timeout to run function first time 
        timep = mktime(tmp); 
        gettimeofday (&tv , &tz);
        utime = timep * 1000000 - (tv.tv_sec * 1000000 + tv.tv_usec);	   	  
        tick.it_value.tv_sec = utime / 1000000;  // sec.  	  
        tick.it_value.tv_usec = utime % 1000000; // usec.
    } else {
        // excute immediately, set timer 1ms later
        tick.it_value.tv_sec = 0;  // sec.  	  
        tick.it_value.tv_usec = 1000; // usec.
    }  
   	  
    // Set timer, ITIMER_REAL : real-time to decrease timer,  	  
    //                          send SIGALRM when timeout  	  
    res = setitimer(ITIMER_REAL, &tick, NULL);  	  
    if (res) {  	  
        printf("Set timer failed!!\n");  	  
    }

    // print the timer info. 
    if (flag) {
        printf("my_timer set timer at %02d-%02d-%02d %02d:%02d:%02d\n", \
            tmp->tm_year+1900, tmp->tm_mon+1, tmp->tm_mday, \
            tmp->tm_hour, tmp->tm_min, tmp->tm_sec);
        fp = fopen(LOG_FILE, "a");
    	  fprintf(fp, "my_timer set timer at %02d-%02d-%02d %02d:%02d:%02d\n", \
            tmp->tm_year+1900, tmp->tm_mon+1, tmp->tm_mday, \
            tmp->tm_hour, tmp->tm_min, tmp->tm_sec);
    	  fclose(fp);
    } else {
        printf("my_timer will excute immediately(1ms later).\n");
    }
    printf("and the interval is %ld(s).%ld(us)\n", \
        tick.it_interval.tv_sec, tick.it_interval.tv_usec);   
    
    fp = fopen(LOG_FILE, "a");
    fprintf(fp, "my_timer set timer at %02d-%02d-%02d %02d:%02d:%02d\n", \
        tmp->tm_year+1900, tmp->tm_mon+1, tmp->tm_mday, \
        tmp->tm_hour, tmp->tm_min, tmp->tm_sec);
    fclose(fp);  

    fd = open(PCIE_DEV, O_RDWR);
    if (fd == -1) {
        printf ("Please check the PCIE card and try again!\n");
    return -1;
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
    close(fd);  	 
    return 0;
		
}

