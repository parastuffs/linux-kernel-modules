obj-m += read_write_mutex.o

all:
	make -C /lib/modules/$(shell uname -r)/build M=$(PWD)
	$(CC) rwtodev.c -o rwtodev
clean:
	make -C /lib/modules/$(shell uname -r)/build M=$(PWD) clean
	rm rwtodev