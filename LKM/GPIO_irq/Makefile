obj-m += LKM_gpio_irq.o
KDIR = "/home/dlh/ECAM/Driver_C/Labs/Debian/linux-4.19"
CC-arm = $(CROSS_COMPILE)gcc

all:
	make -C $(KDIR) M=$(PWD) modules
clean:
	make -C $(KDIR) M=$(PWD) clean
