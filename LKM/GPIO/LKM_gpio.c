#include <linux/module.h>
#include <linux/init.h>
#include <linux/fs.h>
#include <linux/cdev.h>
#include <linux/uaccess.h>
/* gives 'current' */
#include <asm/current.h>
/* 'current' has a pointer to 'struct task_struct'*/
#include <linux/sched.h>
#include <linux/gpio.h>

/* Major device number, chosen among free values in /proc/devices */
#define DRIVER_NAME "LKM_gpio"
#define DRIVER_CLASS "MyModuleClass"

#define GPIO_BUTTON 117
#define GPIO_LED 49

/* Meta information */
MODULE_LICENSE("GPL");
MODULE_AUTHOR("DLH");
MODULE_DESCRIPTION("Interact with GPIOs.");

/* Vars for device and device class */
static dev_t my_device_number;
static struct class *my_class;
static struct cdev my_device;

/* Read callback */
static ssize_t driver_read(struct file *File, char *user_buffer, size_t count, loff_t *offs) {
	int to_copy, not_copied, delta;
	/* Array for three characters: 
	[0] a dummy space (that will hold our value
	[1] New line for nice displaying in terminal. Remove if not necessary.
	[2] \0 end of string. (implicit) */
	char tmp[3] = " \n";

	/* Amount of data to copy */
	to_copy = min(count, sizeof(tmp));

	/* Read value of button */
	/* gpio_get_value() return 0 or 1, but we would like to store the 
	ASCII character '0' or '1' so that we can display it.
	To that end, we offset the value with the ASCII character '0'. */
	printk("Value of button: %d\n", gpio_get_value(GPIO_BUTTON));
	tmp[0] = gpio_get_value(GPIO_BUTTON) + '0';

	/* Copy data to user, returns the amount of byte that weren't copied. */
	not_copied = copy_to_user(user_buffer, &tmp, to_copy);

	/* Calculate delta */
	delta = to_copy - not_copied;

	return delta;
}

/* Write callback */
static ssize_t driver_write(struct file *File, const char *user_buffer, size_t count, loff_t *offs) {
	int to_copy, not_copied, delta;
	char value;

	/* Amount of data to copy */
	to_copy = min(count, sizeof(value));

	/* Copy data to user, returns the amount of byte that weren't copied. */
	not_copied = copy_from_user(&value, user_buffer, to_copy);

	switch(value) {
	case '0':
		printk("Setting a '0' to GPIO %d\n", GPIO_LED);
		gpio_set_value(GPIO_LED, 0);
		break;
	case '1':
		printk("Setting a '1' to GPIO %d\n", GPIO_LED);
		gpio_set_value(GPIO_LED, 1);
		break;
	default:
		printk("Invalid input: %d\n",value);
	}

	/* Calculate delta */
	delta = to_copy - not_copied;

	return delta;
}

/* Open callback. */
static int driver_open(struct inode *device_file, struct file *instance){
	printk("LKM_gpio - open was called!\n");

	return 0;
}

static int driver_close(struct inode *device_file, struct file *instance){
	printk("LKM_gpio - close was called!\n");

	return 0;
}

static struct file_operations fops = {
	.owner = THIS_MODULE,
	.open = driver_open,
	.release = driver_close,
	.read = driver_read,
	.write = driver_write
};

static int __init ModuleInit(void) {
	/* printk writes to dmesg */
	printk("Hi, I'm a new LKM for GPIOs!\n");

	/* Allocate device number */
	if( alloc_chrdev_region(&my_device_number, 0, 1, DRIVER_NAME) < 0) {
		printk("Device number could not be allocated\n");
		return -1;
	}
	printk("LKM_gpio: Device number major: %d, minor: %d registered\n", MAJOR(my_device_number), MINOR(my_device_number));

	/* Create device class */
	if ( (my_class = class_create(THIS_MODULE, DRIVER_CLASS)) == NULL) {
		printk("Device class can't be created\n");
		goto ClassError;
	}

	/* Create device file */
	if (device_create(my_class, NULL, my_device_number, NULL, DRIVER_NAME) == NULL) {
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

	/* LED on GPIO 49 (pin 23 on header P9) */
	if(gpio_request(GPIO_LED, "bbb-gpio-49")) {
		printk("Can not allocate GPIO %d\n", GPIO_LED);
		goto GPIORequestError;
	}
	/* Set GPIO direction */
	/* Second argument is the value at initiatlisation, 0 or 1. */
	if(gpio_direction_output(GPIO_LED, 1)) {
		printk("Can not set GPIO %d to output\n", GPIO_LED);
		goto GPIOLEDError;
	}

	/* Export GPIO so that it appearts in SYSFS */
	gpio_export(GPIO_LED, false);


	return 0;

GPIOLEDError:
	gpio_free(GPIO_LED);

GPIORequestError:
	cdev_del(&my_device);

AddError:
	device_destroy(my_class, my_device_number);

FileError:
	class_destroy(my_class);

ClassError:
	unregister_chrdev(my_device_number, DRIVER_NAME);
	return -1;
}

static void __exit ModuleExit(void) {
	gpio_set_value(GPIO_LED, 0); /* Set the GPIO value to 0 before freeing it. */
	gpio_unexport(GPIO_LED);
	gpio_free(GPIO_LED);
	cdev_del(&my_device);
	device_destroy(my_class, my_device_number);
	class_destroy(my_class);
	unregister_chrdev(my_device_number, DRIVER_NAME);
	printk("Goodbye, Kernel :'(\n");
}

/* Define entry point */
module_init(ModuleInit);
/* Define exit point */
module_exit(ModuleExit);