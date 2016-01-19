TARGET = ttest

obj-m := $(TARGET).o

KDIR := /lib/modules/$(shell uname -r)/build

PWD := $(shell pwd)

all:
	make -C $(KDIR) M=$(PWD) modules

install:
	insmod $(TARGET).ko

uninstall:
	rmmod $(TARGET)
clean:
	rm *.o *.ko *.mod.c Modules.symvers Module.symvers modules.order -f
