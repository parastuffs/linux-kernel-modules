obj-m += crypto_challenge.o

KDIR := "/home/dlh/dev/lemon-image/build/6.12.y"

all:
	make -C $(KDIR) M=$(PWD) modules

clean:
	make -C $(KDIR) M=$(PWD) clean