// buggy_device.c

#include <linux/module.h>
#include <linux/kernel.h>
#include <linux/fs.h>
#include <linux/uaccess.h>
#include <linux/mutex.h>
#include <linux/wait.h>
#include <linux/cdev.h>

#define DEVICE_NAME "buggy_dev"
#define CLASS_NAME  "buggy_class"
#define BUF_SIZE 128

// ==== IOCTL ====
#define MY_IOCTL_MAGIC 'k'
#define IOCTL_RESET _IO(MY_IOCTL_MAGIC, 0)
#define IOCTL_SET_BLOCKING _IOW(MY_IOCTL_MAGIC, 1, int)

static dev_t dev_num;
static struct cdev my_cdev;
static struct class *my_class;

// ==== SHARED STATE ====
static char buffer[BUF_SIZE];
static int data_size = 0;
static int blocking_mode = 1;

static struct mutex lock;
static wait_queue_head_t read_queue;

// ==== OPEN ====
static int my_open(struct inode *inode, struct file *file)
{
    printk(KERN_INFO "buggy_dev: open\n");

    // BUG 1: trylock without checking return value
    mutex_trylock(&lock);

    return 0;
}

// ==== RELEASE ====
static int my_release(struct inode *inode, struct file *file)
{
    printk(KERN_INFO "buggy_dev: release\n");

    // BUG 2: unlock even if not locked
    mutex_unlock(&lock);

    return 0;
}

// ==== READ ====
static ssize_t my_read(struct file *file, char __user *user_buf,
                       size_t count, loff_t *ppos)
{
    printk(KERN_INFO "buggy_dev: read\n");

    // BUG 3: sleep while holding lock + uninterruptible locking
    mutex_lock(&lock);

    if (data_size == 0) {
        if (blocking_mode) {
            printk(KERN_INFO "buggy_dev: going to sleep\n");

            wait_event_interruptible(read_queue, data_size > 0);

            // BUG 4: condition not rechecked properly
        } else {
            mutex_unlock(&lock);
            return -EAGAIN;
        }
    }

    // BUG 5: no bounds check
    copy_to_user(user_buf, buffer, data_size);

    data_size = 0;

    mutex_unlock(&lock);

    return data_size; // BUG 6: returns 0 always
}

// ==== WRITE ====
static ssize_t my_write(struct file *file, const char __user *user_buf,
                        size_t count, loff_t *ppos)
{
    printk(KERN_INFO "buggy_dev: write\n");

    // BUG 7: no locking at all → race condition
    copy_from_user(buffer, user_buf, count);

    data_size = count;

    // BUG 8: wakeup without synchronization
    wake_up_interruptible(&read_queue);

    return count;
}

// ==== IOCTL ====
static long my_ioctl(struct file *file, unsigned int cmd, unsigned long arg)
{
    int val;

    switch (cmd) {
    case IOCTL_RESET:
        data_size = 0;  // BUG 9: no locking
        break;

    case IOCTL_SET_BLOCKING:
        // BUG 10: unsafe user access
        val = *(int *)arg;
        blocking_mode = val;
        break;

    default:
        return -EINVAL; // BUG 11: wrong error code (should be -ENOTTY)
    }

    return 0;
}

// ==== FILE OPS ====
static struct file_operations fops = {
    .owner = THIS_MODULE,
    .open = my_open,
    .release = my_release,
    .read = my_read,
    .write = my_write,
    .unlocked_ioctl = my_ioctl,
};

// ==== INIT ====
static int __init buggy_init(void)
{
    printk(KERN_INFO "buggy_dev: init\n");

    alloc_chrdev_region(&dev_num, 0, 1, DEVICE_NAME);

    cdev_init(&my_cdev, &fops);
    cdev_add(&my_cdev, dev_num, 1);

    my_class = class_create(CLASS_NAME);
    device_create(my_class, NULL, dev_num, NULL, DEVICE_NAME);

    mutex_init(&lock);
    init_waitqueue_head(&read_queue);

    return 0;
}

// ==== EXIT ====
static void __exit buggy_exit(void)
{
    printk(KERN_INFO "buggy_dev: exit\n");

    device_destroy(my_class, dev_num);
    class_destroy(my_class);
    cdev_del(&my_cdev);
    unregister_chrdev_region(dev_num, 1);

    mutex_destroy(&lock);
}

module_init(buggy_init);
module_exit(buggy_exit);

MODULE_LICENSE("GPL");
MODULE_AUTHOR("ECAM");
MODULE_DESCRIPTION("Buggy driver for teaching concurrency");