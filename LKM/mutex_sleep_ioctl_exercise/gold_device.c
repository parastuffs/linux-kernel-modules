// gold_device.c

#include <linux/module.h>
#include <linux/fs.h>
#include <linux/uaccess.h>
#include <linux/cdev.h>
#include <linux/mutex.h>
#include <linux/wait.h>
#include <linux/poll.h>

#define DEVICE_NAME "gold_dev"
#define CLASS_NAME  "gold_class"

#define BUF_SIZE 256

// ==== IOCTL ====
#define MY_IOCTL_MAGIC 'k'
#define IOCTL_RESET _IO(MY_IOCTL_MAGIC, 0)
#define IOCTL_GET_BUFSIZE _IOR(MY_IOCTL_MAGIC, 1, int)

// ==== DEVICE STRUCT ====
struct gold_dev {
    char buffer[BUF_SIZE];
    size_t head;
    size_t tail;
    size_t size;

    struct mutex lock;

    wait_queue_head_t read_queue;
    wait_queue_head_t write_queue;

    struct cdev cdev;
};

static dev_t dev_num;
static struct class *gold_class;
static struct gold_dev dev;

// ==== PER-FILE CONTEXT ====
struct file_ctx {
    int nonblocking;
};

// ==== HELPERS ====
static int buffer_empty(struct gold_dev *d)
{
    return d->size == 0;
}

static int buffer_full(struct gold_dev *d)
{
    return d->size == BUF_SIZE;
}

// ==== OPEN ====
static int gold_open(struct inode *inode, struct file *file)
{
    struct file_ctx *ctx;

    ctx = kmalloc(sizeof(*ctx), GFP_KERNEL);
    if (!ctx)
        return -ENOMEM;

    ctx->nonblocking = (file->f_flags & O_NONBLOCK) ? 1 : 0;
    file->private_data = ctx;

    return 0;
}

// ==== RELEASE ====
static int gold_release(struct inode *inode, struct file *file)
{
    kfree(file->private_data);
    return 0;
}

// ==== READ ====
static ssize_t gold_read(struct file *file, char __user *buf,
                         size_t count, loff_t *ppos)
{
    struct gold_dev *d = &dev;
    struct file_ctx *ctx = file->private_data;
    size_t copied = 0;

    if (ctx->nonblocking) {
        if (mutex_lock_interruptible(&d->lock))
            return -EINTR;

        if (buffer_empty(d)) {
            mutex_unlock(&d->lock);
            return -EAGAIN;
        }
    } else {
        if (wait_event_interruptible(d->read_queue,
            !buffer_empty(d)))
            return -EINTR;

        if (mutex_lock_interruptible(&d->lock))
            return -EINTR;
    }

    while (copied < count && !buffer_empty(d)) {
        if (put_user(d->buffer[d->tail], buf + copied)) {
            mutex_unlock(&d->lock);
            return -EFAULT;
        }

        d->tail = (d->tail + 1) % BUF_SIZE;
        d->size--;
        copied++;
    }

    mutex_unlock(&d->lock);

    wake_up_interruptible(&d->write_queue);

    return copied;
}

// ==== WRITE ====
static ssize_t gold_write(struct file *file, const char __user *buf,
                          size_t count, loff_t *ppos)
{
    struct gold_dev *d = &dev;
    struct file_ctx *ctx = file->private_data;
    size_t copied = 0;

    if (ctx->nonblocking) {
        if (mutex_lock_interruptible(&d->lock))
            return -EINTR;

        if (buffer_full(d)) {
            mutex_unlock(&d->lock);
            return -EAGAIN;
        }
    } else {
        if (wait_event_interruptible(d->write_queue,
            !buffer_full(d)))
            return -EINTR;

        if (mutex_lock_interruptible(&d->lock))
            return -EINTR;
    }

    while (copied < count && !buffer_full(d)) {
        char c;

        if (get_user(c, buf + copied)) {
            mutex_unlock(&d->lock);
            return -EFAULT;
        }

        d->buffer[d->head] = c;
        d->head = (d->head + 1) % BUF_SIZE;
        d->size++;
        copied++;
    }

    mutex_unlock(&d->lock);

    wake_up_interruptible(&d->read_queue);

    return copied;
}

// ==== POLL ====
static __poll_t gold_poll(struct file *file, poll_table *wait)
{
    struct gold_dev *d = &dev;
    __poll_t mask = 0;

    poll_wait(file, &d->read_queue, wait);
    poll_wait(file, &d->write_queue, wait);

    mutex_lock(&d->lock);

    if (!buffer_empty(d))
        mask |= POLLIN | POLLRDNORM;

    if (!buffer_full(d))
        mask |= POLLOUT | POLLWRNORM;

    mutex_unlock(&d->lock);

    return mask;
}

// ==== IOCTL ====
static long gold_ioctl(struct file *file,
                       unsigned int cmd, unsigned long arg)
{
    int val;

    switch (cmd) {
    case IOCTL_RESET:
        if (mutex_lock_interruptible(&dev.lock))
            return -EINTR;

        dev.head = dev.tail = dev.size = 0;

        mutex_unlock(&dev.lock);

        wake_up_interruptible(&dev.write_queue);
        break;

    case IOCTL_GET_BUFSIZE:
        val = BUF_SIZE;

        if (copy_to_user((int __user *)arg, &val, sizeof(val)))
            return -EFAULT;

        break;

    default:
        return -ENOTTY;
    }

    return 0;
}

// ==== FOPS ====
static struct file_operations fops = {
    .owner = THIS_MODULE,
    .open = gold_open,
    .release = gold_release,
    .read = gold_read,
    .write = gold_write,
    .poll = gold_poll,
    .unlocked_ioctl = gold_ioctl,
};

// ==== INIT ====
static int __init gold_init(void)
{
    int ret;

    ret = alloc_chrdev_region(&dev_num, 0, 1, DEVICE_NAME);
    if (ret)
        return ret;

    cdev_init(&dev.cdev, &fops);

    ret = cdev_add(&dev.cdev, dev_num, 1);
    if (ret)
        goto err_unregister;

    gold_class = class_create(CLASS_NAME);
    if (IS_ERR(gold_class)) {
        ret = PTR_ERR(gold_class);
        goto err_cdev;
    }

    if (IS_ERR(device_create(gold_class, NULL, dev_num, NULL, DEVICE_NAME))) {
        ret = -ENOMEM;
        goto err_class;
    }

    mutex_init(&dev.lock);
    init_waitqueue_head(&dev.read_queue);
    init_waitqueue_head(&dev.write_queue);

    dev.head = dev.tail = dev.size = 0;

    printk(KERN_INFO "gold_dev: loaded\n");
    return 0;

err_class:
    class_destroy(gold_class);
err_cdev:
    cdev_del(&dev.cdev);
err_unregister:
    unregister_chrdev_region(dev_num, 1);
    return ret;
}

// ==== EXIT ====
static void __exit gold_exit(void)
{
    device_destroy(gold_class, dev_num);
    class_destroy(gold_class);
    cdev_del(&dev.cdev);
    unregister_chrdev_region(dev_num, 1);

    printk(KERN_INFO "gold_dev: unloaded\n");
}

module_init(gold_init);
module_exit(gold_exit);

MODULE_LICENSE("GPL");
MODULE_AUTHOR("ECAM");
MODULE_DESCRIPTION("Gold standard driver");