#include <linux/module.h>
#include <linux/init.h>
#include <linux/fs.h>
#include <linux/cdev.h>
#include <linux/uaccess.h>

#define DEVICE_NAME "petite_mouette"
#define DRIVER_CLASS "Classe_mouette"

/* Meta information */
MODULE_LICENSE("GPL");
MODULE_AUTHOR("DLH");
MODULE_DESCRIPTION("Create and destroy a simple character device.");

/* Vars for device and device class */
static dev_t my_device_number;
static struct class *my_class;
static struct cdev my_device;

static struct file_operations fops = {
	.owner = THIS_MODULE
};


static int __init ModuleInit(void) {
	/* printk writes to dmesg */
	printk("Hi, I'm a new LKM! (๏̯͡๏) O'RLY?\n");

	/* Allocate device number */
	if( alloc_chrdev_region(&my_device_number, 0, 1, DEVICE_NAME) < 0) {
		printk("Device number could not be allocated\n");
		return -1;
	}
	printk("Petite mouette: Device number major: %d, minor: %d registered\n", MAJOR(my_device_number), MINOR(my_device_number));

	/* Create device class */
	if ( (my_class = class_create(DRIVER_CLASS)) == NULL) {
		printk("Device class can't be created\n");
		goto ClassError;
	}

	/* Create device file */
	if (device_create(my_class, NULL, my_device_number, NULL, DEVICE_NAME) == NULL) {
		printk("Can't create device file\n");
		goto FileError;
	}

	/* Init device file */
	cdev_init(&my_device, &fops);

	/* Registering device to kernel */
	if (cdev_add(&my_device, my_device_number, 1) == -1) {
		printk("Can't register device to kernel\n");
		goto AddError;
	}

	return 0;

AddError:
	device_destroy(my_class, my_device_number);

FileError:
	class_destroy(my_class);

ClassError:
	unregister_chrdev(my_device_number, DEVICE_NAME);
	return -1;
}

static void __exit ModuleExit(void) {
	cdev_del(&my_device);
	device_destroy(my_class, my_device_number);
	class_destroy(my_class);
	unregister_chrdev(my_device_number, DEVICE_NAME);
	printk("Goodbye, Kernel :'(\n");
}

/* Defin zero e entry point */
module_init(ModuleInit);
/* Define exit point */
module_exit(ModuleExit);