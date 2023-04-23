#include <linux/module.h>
#include <linux/init.h>
#include <linux/fs.h>
#include <linux/cdev.h>
#include <linux/uaccess.h>
#include <linux/mutex.h>
#include <linux/errno.h>

#define DRIVER_NAME "MyChrDevice"
#define DRIVER_CLASS "MyModuleClass"

/* Static definition  and initialisation of a mutex. */
// static DEFINE_MUTEX(my_mutex_name);
/* Dynamic mutex definition. Requires a mutex_init() to be initialised. */
static struct mutex my_dyn_mutex;

/* Meta information */
MODULE_LICENSE("GPL");
MODULE_AUTHOR("DLH");
MODULE_DESCRIPTION("Read and write on a device.");

/* Buffer for data */
static char buffer[255];
static int buffer_pointer = 0;

/* Vars for device and device class */
static dev_t my_device_number;
static struct class *my_class;
static struct cdev my_device;

/* Read callback */
static ssize_t driver_read(struct file *File, char *user_buffer, size_t count, loff_t *offs) {
	int to_copy, not_copied, delta;

	/* Amount of data to copy */
	to_copy = min(count, buffer_pointer);

	/* Copy data to user, returns the amount of byte that weren't copied. */
	not_copied = copy_to_user(user_buffer, buffer, to_copy);

	/* Calculate delta */
	delta = to_copy - not_copied;

	return delta;
}

/* Write callback */
static ssize_t driver_write(struct file *File, const char *user_buffer, size_t count, loff_t *offs) {
	int to_copy, not_copied, delta;

	/* Amount of data to copy */
	to_copy = min(count, sizeof(buffer));

	/* Copy data to user, returns the amount of byte that weren't copied. */
	not_copied = copy_from_user(buffer, user_buffer, to_copy);
	buffer_pointer = to_copy - not_copied;

	/* Calculate delta */
	delta = to_copy - not_copied;

	return delta;
}

/* Open callback. */
static int driver_open(struct inode *device_file, struct file *instance){
	printk("read-write module - open was called!\n");
	/*if(!mutex_trylock(&my_dyn_mutex)) {
		printk("read-write module: resource busy, aborting!\n");
		return -EBUSY;
	}

	return 0;*/

	return mutex_lock_interruptible(&my_dyn_mutex);
	/*mutex_lock(&my_dyn_mutex);*/
	return 0;
}

static int driver_close(struct inode *device_file, struct file *instance){
	printk("read-write module - close was called!\n");
	mutex_unlock(&my_dyn_mutex);

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

	mutex_init(&my_dyn_mutex);

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
	mutex_destroy(&my_dyn_mutex);
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