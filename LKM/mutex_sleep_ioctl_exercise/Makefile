obj-m += buggy_device.o
obj-m += fixed_device.o
obj-m += gold_device.o
KDIR = "/lib/modules/$(shell uname -r)/build"

all:
	make -C $(KDIR) M=$(PWD) modules
	$(CC) -O2 -pthread stress_test.c -o stress_test
	rm *.mod *.o *.mod.c
clean:
	make -C $(KDIR) M=$(PWD) clean
	rm stress_test