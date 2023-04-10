/* hello.c */
#include <linux/module.h>
/* Located in /usr/lib/modules/5.15.102-1-MANJARO/build/include/linux/init.h */
#include <linux/init.h>

/* Meta information */
MODULE_LICENSE("GPL");
MODULE_AUTHOR("DLH");
MODULE_DESCRIPTION("Hello world kernel module");

/* __init tells the kernel this function
is executed only once at the initialisation
of the module. */
static int __init ModuleInit(void) {
	 /* printk writes to dmesg */
	printk(KERN_NOTICE "Hello kernel!\n");
	return 0;
}

/* __exit is the same as __init but for unloading
the module.*/
static void __exit ModuleExit(void) {
	printk(KERN_NOTICE "Goodbye, Kernel :'(\n");
}

/* Define entry point */
module_init(ModuleInit);
/* Define exit point */
module_exit(ModuleExit);