GUEST_HC_SYNC = guest_sync
HOST_IO_SYNC = host_sync
CC = gcc
RM = rm

all:
	@echo $(MAKE)...
	$(CC) $(GUEST_HC_SYNC).c -o $(GUEST_HC_SYNC) -lrt
	$(CC) $(HOST_IO_SYNC).c -o $(HOST_IO_SYNC) -lrt

clean:
	@echo $(MAKE)...
	$(RM) -rf $(GUEST_HC_SYNC) $(HOST_IO_SYNC)

.PHONY: all clean

