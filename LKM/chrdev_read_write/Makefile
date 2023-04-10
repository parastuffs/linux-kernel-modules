obj-m += read_write.o
KDIR = "/home/dlh/ECAM/Driver_C/Labs/Debian/linux-4.19"
CC-arm = $(CROSS_COMPILE)gcc

all:
	make -C $(KDIR) M=$(PWD) modules
	$(CC-arm) -static rwtodev.c -o rwtodev
clean:
	make -C $(KDIR) M=$(PWD) clean
	rm rwtodev