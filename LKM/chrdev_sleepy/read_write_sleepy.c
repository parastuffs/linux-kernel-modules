#include <linux/module.h>
#include <linux/init.h>
#include <linux/fs.h>
#include <linux/cdev.h>
#include <linux/uaccess.h>
#include <linux/errno.h>
#include <linux/wait.h>

#define DRIVER_NAME "SleepyDevice"
#define DRIVER_CLASS "MyModuleClass"

/* Meta information */
MODULE_LICENSE("GPL");
MODULE_AUTHOR("DLH");
MODULE_DESCRIPTION("Sleep on read, wake up on write.");

/* Vars for device and device class */
static dev_t my_device_number;
static struct class *my_class;
static struct cdev my_device;

/* Wait queue */
static wait_queue_head_t my_queue;
static int flag = 0;

/* Read callback */
static ssize_t driver_read(struct file *File, char *user_buffer, size_t count, loff_t *offs) {
	printk(KERN_DEBUG "process %i (%s) going to sleep\n", current->pid, current->comm);
	wait_event_interruptible(my_queue, flag != 0);
	flag = 0;
	printk(KERN_DEBUG "awoken %i (%s)\n", current->pid, current->comm);
	return 0;
}

/* Write callback */
static ssize_t driver_write(struct file *File, const char *user_buffer, size_t count, loff_t *offs) {
	printk(KERN_DEBUG "process %i (%s) awakening the readers...\n", current->pid, current->comm);
	flag = 1;
	wake_up_interruptible(&my_queue);
	return count; /* succeed, to avoid retrial */
}

/* Open callback. */
static int driver_open(struct inode *device_file, struct file *instance){
	printk("read-write module - open was called!\n");
	return 0;
}

static int driver_close(struct inode *device_file, struct file *instance){
	printk("read-write module - close was called!\n");
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
	printk("Hi, I'm a new LKM!\n");

	/* Allocate device number */
	if( alloc_chrdev_region(&my_device_number, 0, 1, DRIVER_NAME) < 0) {
		printk("Device number could not be allocated\n");
		return -1;
	}
	printk("read-write: Device number major: %d, minor: %d registered\n", MAJOR(my_device_number), MINOR(my_device_number));

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

	init_waitqueue_head(&my_queue);

	return 0;

AddError:
	device_destroy(my_class, my_device_number);

FileError:
	class_destroy(my_class);

ClassError:
	unregister_chrdev(my_device_number, DRIVER_NAME);
	return -1;
}

static void __exit ModuleExit(void) {
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