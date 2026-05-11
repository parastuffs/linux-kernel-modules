obj-m += kbdmon.o
obj-m += kbdmon_chrdev.o
obj-m += kbdmon_sequence_detector.o
obj-m += kbdmon_timing.o
obj-m += kbdmon_keylogger.o

#KDIR := "/lib/modules/$(shell uname -r)/build"
KDIR := "/home/dlh/dev/lemon-image/build/6.12.y"

all:
	make -C $(KDIR) M=$(PWD) modules
	arm-none-linux-gnueabihf-gcc kbdmon_chrdev_demo.c -o kbdmon_chrdev_demo

clean:
	make -C $(KDIR) M=$(PWD) clean
	rm kbdmon_chrdev_demo