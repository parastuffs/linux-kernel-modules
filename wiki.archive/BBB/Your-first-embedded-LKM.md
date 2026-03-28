On your local computer, create a new directory from your project, then create a file with a simple 'Hello world' code.
```c
/* hello.c */
#include <linux/module.h>
/* Located in /usr/lib/modules/5.15.102-1-MANJARO/build/include/linux/init.h */
#include <linux/init.h>

/* Meta information */
MODULE_LICENSE("GPL");
MODULE_AUTHOR("DLH");
MODULE_DESCRIPTION("Hello world kernel module");

/* __init tells the kernel this function is executed only once at the initialisation of the module. */
static int __init ModuleInit(void) {
	 /* printk writes to dmesg */
	printk(KERN_NOTICE "Hello kernel!\n");
	return 0;
}

/* __exit is the same as __init but for unloading the module.*/
static void __exit ModuleExit(void) {
	printk(KERN_NOTICE "Goodbye, Kernel :'(\n");
}

/* Define entry point */
module_init(ModuleInit);
/* Define exit point */
module_exit(ModuleExit);
```

Create a Makefile in the same directory:
```Makefile
obj-m += hello.o
KDIR = "/path/to/your/compiled/linux/kernel"

all:
	make -C $(KDIR) M=$(PWD) modules
clean:
	make -C $(KDIR) M=$(PWD) clean
```
`KDIR` is the path to the linux kernel [you should have compiled](https://github.com/parastuffs/linux-kernel-modules/wiki/Cross-compilation-toolchain#compile-the-kernel-locally). It contains the folders `arch`, `block`, `certs`, etc.
If you don't set this variable, `make` will assume you want to build a module for your local computer, which will not work on the BBB.


Compile the module, then send it to the BBB with `scp`:
```
$ make CROSS_COMPILE=arm-none-linux-gnueabihf- ARCH=arm
$ scp hello.ko debian@192.168.7.2:~/LKM/
```

On the BBB, test the module:
```
debian@beaglebone:~/LKM$ modinfo hello.ko
filename:       /home/debian/LKM/hello.ko
description:    Hello world kernel module
author:         DLH
license:        GPL
depends:        
name:           hello
vermagic:       4.19.94 SMP preempt mod_unload modversions ARMv7 p2v8
debian@beaglebone:~/LKM$ sudo insmod hello.ko
debian@beaglebone:~/LKM$ lsmod | grep hello
hello                  16384  0
debian@beaglebone:~/LKM$ sudo rmmod hello
debian@beaglebone:~/LKM$ sudo dmesg | tail
[   54.532712] wkup_m3_ipc 44e11324.wkup_m3_ipc: CM3 Firmware Version = 0x193
[   54.605281] PM: bootloader does not support rtc-only!
[   56.876103] remoteproc remoteproc1: 4a334000.pru is available
[   56.876491] pru-rproc 4a334000.pru: PRU rproc node pru@4a334000 probed successfully
[   56.888700] remoteproc remoteproc2: 4a338000.pru is available
[   56.888880] pru-rproc 4a338000.pru: PRU rproc node pru@4a338000 probed successfully
[17617.733701] hello: no symbol version for module_layout
[17617.733725] hello: loading out-of-tree module taints kernel.
[17617.744371] Hello kernel!
[17646.617032] Goodbye, Kernel :'(
```




