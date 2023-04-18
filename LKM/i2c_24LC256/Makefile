obj-m += i2c_24LC256.o
KDIR = "/home/dlh/ECAM/Driver_C/Labs/Debian/linux-4.19"

all:
	make -C $(KDIR) M=$(PWD) modules
clean:
	make -C $(KDIR) M=$(PWD) clean
