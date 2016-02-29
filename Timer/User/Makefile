MAIN_TIMER = guest_user_timer
POSIX_TIMER = posix_timer
LOAD = load
RDTIME_COST = rdtime_cost
TCP_CLIENT = tcp_client
TCP_SERVER = tcp_server

all:
	gcc $(MAIN_TIMER).c -o  $(MAIN_TIMER) -lrt
	gcc $(POSIX_TIMER).c -o  $(POSIX_TIMER) -lrt
	gcc $(LOAD).c -o  $(LOAD) -lrt
	gcc $(RDTIME_COST).c -o  $(RDTIME_COST) -lrt
	gcc $(TCP_CLIENT).c -o  $(TCP_CLIENT) -lrt
	gcc $(TCP_SERVER).c -o  $(TCP_SERVER) -lrt

clean:
	rm -rf $(MAIN_TIMER) $(POSIX_TIMER) $(LOAD) $(RDTIME_COST) $(TCP_CLIENT) $(TCP_SERVER)

.PHONY: all clean

