MAIN_TIMER = main_user_timer
POSIX_TIMER = posix_timer
LOAD = load
RDTIME_COST = rdtime_cost
TCP_CLIENT = tcp_client
TCP_SERVER = tcp_server
CC = gcc
RM = rm

all:
	@echo $(MAKE)...
	$(CC) $(MAIN_TIMER).c -o $(MAIN_TIMER) -lrt
	$(CC) $(POSIX_TIMER).c -o $(POSIX_TIMER) -lrt
	$(CC) $(LOAD).c -o $(LOAD) -lrt
	$(CC) $(RDTIME_COST).c -o $(RDTIME_COST) -lrt
	$(CC) $(TCP_CLIENT).c -o $(TCP_CLIENT) -lrt
	$(CC) $(TCP_SERVER).c -o $(TCP_SERVER) -lrt

clean:
	@echo $(MAKE)...
	$(RM) -rf $(MAIN_TIMER) $(POSIX_TIMER) $(LOAD) $(RDTIME_COST) $(TCP_CLIENT) $(TCP_SERVER)

.PHONY: all clean

