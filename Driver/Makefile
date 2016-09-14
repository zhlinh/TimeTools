KERNELDIR = /lib/modules/$(shell uname -r)/build/

# The current directory is passed to sub-makes as argument
#KERNELDIR =/home/lemon/bsp/kernel/linux-2.6.29.6_1588_patched
#KERNELDIR = /home/chengyi/kernel/linux_2.x/linux-2.6.18
#KERNELDIR = /usr/src/linux-2.6.32-220.el6

PWD := $(shell pwd)
#$(shell grep " do_adjtimex" /boot/System.map-`uname -r` | awk -F ' ' '{print  "#define DO_ADJTIMEX_ADDRESS 0x"$1}'>system.h)
#INSTALLDIR = /home/ben/boardnet

#CC=/home/lemon/freescale-2010.09/bin/powerpc-linux-gnu-gcc
CC = gcc
RM = rm
SYNC_DAEMON = time_sync

obj-m := pcie_host_device.o

all:
	@echo $(MAKE)...
	$(MAKE) modules
	$(MAKE) sync

modules:
	@echo $(MAKE)...
	$(MAKE) -C $(KERNELDIR) M=$(PWD) modules

#modules_install:
#cp pcie_host_device.ko $(INSTALLDIR)

sync:
	@echo $(MAKE)...
	$(CC) time_sync.c -o $(SYNC_DAEMON) -lrt

clean:
	@echo $(MAKE)...
	$(RM) -rf *.o Module.* modules.* *~ core .depend .*.cmd *.ko *.mod.c .tmp_versions $(SYNC_DAEMON)

.PHONY: all modules modules_install sync clean
