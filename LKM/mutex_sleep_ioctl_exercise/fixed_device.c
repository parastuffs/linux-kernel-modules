// fixed_device.c

#include <linux/module.h>
#include <linux/kernel.h>
#include <linux/fs.h>
#include <linux/uaccess.h>
#include <linux/mutex.h>
#include <linux/wait.h>
#include <linux/cdev.h>

#define DEVICE_NAME "fixed_dev"
#define CLASS_NAME  "fixed_class"
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
static size_t data_size = 0;
static int blocking_mode = 1;

static DEFINE_MUTEX(lock);
static wait_queue_head_t read_queue;

// ==== OPEN ====
static int my_open(struct inode *inode, struct file *file)
{
    printk(KERN_INFO "fixed_dev: open\n");
    return 0;
}

// ==== RELEASE ====
static int my_release(struct inode *inode, struct file *file)
{
    printk(KERN_INFO "fixed_dev: release\n");
    return 0;
}

// ==== READ ====
static ssize_t my_read(struct file *file, char __user *user_buf,
                       size_t count, loff_t *ppos)
{
    ssize_t ret;

    printk(KERN_INFO "fixed_dev: read\n");

    // Handle non-blocking mode
    if (!blocking_mode) {
        if (mutex_lock_interruptible(&lock))
            return -EINTR;

        if (data_size == 0) {
            mutex_unlock(&lock);
            return -EAGAIN;
        }
    } else {
        // Blocking mode: wait for data
        if (wait_event_interruptible(read_queue, data_size > 0))
            return -EINTR;

        if (mutex_lock_interruptible(&lock))
            return -EINTR;
    }

    // At this point: lock held + data_size > 0
    size_t to_copy = min(count, data_size);

    if (copy_to_user(user_buf, buffer, to_copy)) {
        mutex_unlock(&lock);
        return -EFAULT;
    }

    data_size = 0;

    mutex_unlock(&lock);

    ret = to_copy;
    return ret;
}

// ==== WRITE ====
static ssize_t my_write(struct file *file, const char __user *user_buf,
                        size_t count, loff_t *ppos)
{
    size_t to_copy;

    printk(KERN_INFO "fixed_dev: write\n");

    if (mutex_lock_interruptible(&lock))
        return -EINTR;

    to_copy = min(count, (size_t)BUF_SIZE);

    if (copy_from_user(buffer, user_buf, to_copy)) {
        mutex_unlock(&lock);
        return -EFAULT;
    }

    data_size = to_copy;

    mutex_unlock(&lock);

    // Wake readers AFTER updating state
    wake_up_interruptible(&read_queue);

    return to_copy;
}

// ==== IOCTL ====
static long my_ioctl(struct file *file, unsigned int cmd, unsigned long arg)
{
    int val;

    switch (cmd) {
    case IOCTL_RESET:
        if (mutex_lock_interruptible(&lock))
            return -EINTR;

        data_size = 0;

        mutex_unlock(&lock);
        break;

    case IOCTL_SET_BLOCKING:
        if (copy_from_user(&val, (int __user *)arg, sizeof(int)))
            return -EFAULT;

        if (val != 0 && val != 1)
            return -EINVAL;

        if (mutex_lock_interruptible(&lock))
            return -EINTR;

        blocking_mode = val;

        mutex_unlock(&lock);
        break;

    default:
        return -ENOTTY;
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
static int __init fixed_init(void)
{
    int ret;

    printk(KERN_INFO "fixed_dev: init\n");

    ret = alloc_chrdev_region(&dev_num, 0, 1, DEVICE_NAME);
    if (ret < 0)
        return ret;

    cdev_init(&my_cdev, &fops);

    ret = cdev_add(&my_cdev, dev_num, 1);
    if (ret < 0) {
        unregister_chrdev_region(dev_num, 1);
        return ret;
    }

    my_class = class_create(CLASS_NAME);
    if (IS_ERR(my_class)) {
        cdev_del(&my_cdev);
        unregister_chrdev_region(dev_num, 1);
        return PTR_ERR(my_class);
    }

    if (IS_ERR(device_create(my_class, NULL, dev_num, NULL, DEVICE_NAME))) {
        class_destroy(my_class);
        cdev_del(&my_cdev);
        unregister_chrdev_region(dev_num, 1);
        return -ENOMEM;
    }

    init_waitqueue_head(&read_queue);

    return 0;
}

// ==== EXIT ====
static void __exit fixed_exit(void)
{
    printk(KERN_INFO "fixed_dev: exit\n");

    device_destroy(my_class, dev_num);
    class_destroy(my_class);
    cdev_del(&my_cdev);
    unregister_chrdev_region(dev_num, 1);
}

module_init(fixed_init);
module_exit(fixed_exit);

MODULE_LICENSE("GPL");
MODULE_AUTHOR("ECAM");
MODULE_DESCRIPTION("Reference fixed driver");