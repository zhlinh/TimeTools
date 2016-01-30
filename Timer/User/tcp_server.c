#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <unistd.h>
#define PORT 8888			
#define BACKLOG 10

int bytes_to_int(unsigned char *);
void process_conn_server(int);

int bytes_to_int(unsigned char *bytes)
{
    int num = bytes[0] & 0xff;
    num |= ((bytes[1] << 8) & 0xff00);
    num |= ((bytes[2] << 16) & 0xff0000);
    num |= ((bytes[3] << 24) & 0xff000000);
    return num;
}

void process_conn_server(int s)
{
	ssize_t size = 0;
	unsigned char buffer[50];
  int hour, min, sec;
  int ts_sec, ts_nsec;
  int count = 0;						
	for(;;){
    bzero(buffer, sizeof(buffer));								
		size = read(s, buffer, 100);	
		if(size == 0){
			return;	
		}
		
		//printf("%d bytes altogether\n", (int)size);
		hour = bytes_to_int(buffer);
		min = bytes_to_int(&buffer[4]);		
    sec = bytes_to_int(&buffer[8]);

    ts_sec = bytes_to_int(&buffer[12]);              
		ts_nsec = bytes_to_int(&buffer[16]);
                 
    printf("%d:%d:%02d offset is %d(s).%09u(ns), (%d)\n", \
           hour, min, sec, ts_sec, ts_nsec, ++count);    
	}	
}


int main(int argc, char *argv[])
{
	int ss,sc;
	struct sockaddr_in server_addr;
	struct sockaddr_in client_addr;
	int err;							
	pid_t pid;							

	ss = socket(AF_INET, SOCK_STREAM, 0);
	if(ss < 0){
		printf("socket error\n");
		return -1;	
	}
	
	
	bzero(&server_addr, sizeof(server_addr));	
	server_addr.sin_family = AF_INET;
	server_addr.sin_addr.s_addr = htonl(INADDR_ANY);
	server_addr.sin_port = htons(PORT);
	
	err = bind(ss, (struct sockaddr*)&server_addr, sizeof(server_addr));
	if(err < 0){
		printf("bind error\n");
		return -1;	
	}
	
	err = listen(ss, BACKLOG);
	if(err < 0){
		printf("listen error\n");
		return -1;	
	}
	
	for(;;)	{
		socklen_t addrlen = sizeof(struct sockaddr);
		
		sc = accept(ss, (struct sockaddr*)&client_addr, &addrlen); 
		
		if(sc < 0){						
			continue;						
		}	
		
		pid = fork();					
		if( pid == 0 ){			
			process_conn_server(sc);
			close(ss);
		}else{
			close(sc);
		}
	}
}



