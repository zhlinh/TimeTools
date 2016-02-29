GUEST_HC_SYNC = guest_sync
HOST_IO_SYNC = host_sync

all:
	gcc $(GUEST_HC_SYNC).c -o  $(GUEST_HC_SYNC) -lrt
	gcc $(HOST_IO_SYNC).c -o  $(HOST_IO_SYNC) -lrt

clean:
	rm -rf $(GUEST_HC_SYNC) $(HOST_IO_SYNC)

.PHONY: all clean

