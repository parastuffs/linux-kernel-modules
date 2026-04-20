obj-m += buggy_leaky_device.o

all:
	make -C /lib/modules/$(shell uname -r)/build M=$(PWD) modules
	$(CC) leak_demo.c -o leak_demo

clean:
	make -C /lib/modules/$(shell uname -r)/build M=$(PWD) clean
	rm leak_demo