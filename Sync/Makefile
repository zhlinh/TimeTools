HC_SYNC = guest_time_sync
IO_SYNC = time_sync

all:
	gcc $(HC_SYNC).c -o  $(HC_SYNC) -lrt
	gcc $(IO_SYNC).c -o  $(IO_SYNC) -lrt

clean:
	rm -rf $(HC_SYNC) $(IO_SYNC)

.PHONY: all clean

