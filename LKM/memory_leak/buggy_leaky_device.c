#define pr_fmt(fmt) "leaky_dev: " fmt

#include <linux/module.h>
#include <linux/fs.h>
#include <linux/slab.h>
#include <linux/uaccess.h>
#include <linux/cdev.h>

#define DEVICE_NAME "leaky_dev"
#define CLASS_NAME  "leaky"

static char *buffer;
static size_t buffer_size = 1024;

/* Vars for device and device class */
static dev_t my_device_number;
static struct class *my_class;
static struct cdev my_device;

module_param(buffer_size, ulong, 0644);
MODULE_PARM_DESC(buffer_size, "Size of the internal buffer");

static ssize_t dev_read(struct file *file, char __user *user_buf,
                        size_t len, loff_t *offset)
{
    size_t to_copy;

    if (*offset >= buffer_size)
        return 0;

    to_copy = min(len, buffer_size - *offset);

    print_hex_dump(KERN_INFO, "leak: ", DUMP_PREFIX_OFFSET, 16, 1, buffer, buffer_size, true);
    if (copy_to_user(user_buf, buffer + *offset, to_copy))
        return -EFAULT;

    *offset += to_copy;
    return to_copy;
}

static ssize_t dev_write(struct file *file, const char __user *user_buf,
                         size_t len, loff_t *offset)
{
    size_t to_copy = min(len, buffer_size);

    if (!buffer)
        buffer = kmalloc(buffer_size, GFP_KERNEL);

    if (!buffer)
        return -ENOMEM;

    if (copy_from_user(buffer, user_buf, to_copy))
        return -EFAULT;

    return to_copy;
}

static int dev_open(struct inode *inodep, struct file *filep)
{
    return 0;
}

static int dev_release(struct inode *inodep, struct file *filep)
{
    return 0;
}

static struct file_operations fops = {
    .owner = THIS_MODULE,
    .read = dev_read,
    .write = dev_write,
    .open = dev_open,
    .release = dev_release,
};

static int __init leaky_init(void)
{
    if( alloc_chrdev_region(&my_device_number, 0, 1, DEVICE_NAME) < 0) {
        pr_err("Device number could not be allocated\n");
        return -1;
    }

    if ( (my_class = class_create(CLASS_NAME)) == NULL) {
        pr_err("Device class can't be created\n");
        goto ClassError;
    }

    if (device_create(my_class, NULL, my_device_number, NULL, DEVICE_NAME) == NULL) {
        pr_err("Can't create device file\n");
        goto FileError;
    }

    cdev_init(&my_device, &fops);

    if (cdev_add(&my_device, my_device_number, 1) == -1) {
        pr_err("Can't register device to kernel\n");
        goto AddError;
    }

    buffer = kmalloc(buffer_size, GFP_KERNEL);

    pr_info("Leaky device is ready to go.");

    return 0;

   AddError:
    device_destroy(my_class, my_device_number);

   FileError:
    class_destroy(my_class);

   ClassError:
    unregister_chrdev(my_device_number, DEVICE_NAME);
    return -1;
}

static void __exit leaky_exit(void)
{
    kfree(buffer);
    cdev_del(&my_device);
	device_destroy(my_class, my_device_number);
	class_destroy(my_class);
	unregister_chrdev(my_device_number, DEVICE_NAME);
    pr_info("unloaded\n");
}

MODULE_LICENSE("GPL");
MODULE_AUTHOR("ECAM");
MODULE_DESCRIPTION("Leaky device demonstrating lack of memory zeroing");

module_init(leaky_init);
module_exit(leaky_exit);