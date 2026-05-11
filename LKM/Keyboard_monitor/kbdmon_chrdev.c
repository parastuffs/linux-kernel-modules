#include <linux/module.h>
#include <linux/init.h>
#include <linux/input.h>
#include <linux/slab.h>
#include <linux/miscdevice.h>
#include <linux/fs.h>
#include <linux/uaccess.h>
#include <linux/wait.h>
#include <linux/spinlock.h>

MODULE_LICENSE("GPL");
MODULE_DESCRIPTION("Simple keyboard monitor using input_handler");
MODULE_VERSION("1.0");

#define BUF_SIZE 128

struct key_event {
    u16 code;
    u8 value;
};

static struct key_event ring[BUF_SIZE];
static int head, tail;

static DEFINE_SPINLOCK(ring_lock);
static DECLARE_WAIT_QUEUE_HEAD(readq);

struct kbdmon {
    struct input_handle handle;
};

static void push_event(u16 code, u8 value)
{
    unsigned long flags;

    spin_lock_irqsave(&ring_lock, flags);

    ring[head].code = code;
    ring[head].value = value;
    head = (head + 1) % BUF_SIZE;

    if (head == tail)
        tail = (tail + 1) % BUF_SIZE;

    spin_unlock_irqrestore(&ring_lock, flags);

    wake_up_interruptible(&readq);
}

static ssize_t kbdmon_read(struct file *f,
                           char __user *buf,
                           size_t len,
                           loff_t *off)
{
    struct key_event ev;
    unsigned long flags;

    if (len < sizeof(ev))
        return -EINVAL;

    wait_event_interruptible(readq, head != tail);

    spin_lock_irqsave(&ring_lock, flags);

    ev = ring[tail];
    tail = (tail + 1) % BUF_SIZE;

    spin_unlock_irqrestore(&ring_lock, flags);

    if (copy_to_user(buf, &ev, sizeof(ev)))
        return -EFAULT;

    return sizeof(ev);
}

static const struct file_operations fops = {
    .owner = THIS_MODULE,
    .read = kbdmon_read,
};

static struct miscdevice kbdmon_dev = {
    .minor = MISC_DYNAMIC_MINOR,
    .name = "kbdmon",
    .fops = &fops,
};

/* -------------------------------------------------------------------------- */
/* Event callback                                                             */
/* -------------------------------------------------------------------------- */

static void kbdmon_event(struct input_handle *handle,
                         unsigned int type,
                         unsigned int code,
                         int value)
{
    if (type != EV_KEY)
        return;

    push_event(code, value);
}

/* -------------------------------------------------------------------------- */
/* Device matching                                                            */
/* -------------------------------------------------------------------------- */

static int kbdmon_connect(struct input_handler *handler,
                          struct input_dev *dev,
                          const struct input_device_id *id)
{
    struct kbdmon *kbd;
    int ret;

    /*
     * Allocate connection object.
     */
    kbd = kzalloc(sizeof(*kbd), GFP_KERNEL);
    if (!kbd)
        return -ENOMEM;

    kbd->handle.dev = dev;
    kbd->handle.handler = handler;
    kbd->handle.name = "kbdmon_handle";

    /*
     * Register input handle.
     */
    ret = input_register_handle(&kbd->handle);
    if (ret) {
        pr_err("[kbdmon] input_register_handle failed\n");
        goto err_free;
    }

    /*
     * Open device so we receive events.
     */
    ret = input_open_device(&kbd->handle);
    if (ret) {
        pr_err("[kbdmon] input_open_device failed\n");
        goto err_unregister;
    }

    pr_info("[kbdmon] Connected to device: %s\n", dev->name);

    return 0;

err_unregister:
    input_unregister_handle(&kbd->handle);

err_free:
    kfree(kbd);
    return ret;
}

static void kbdmon_disconnect(struct input_handle *handle)
{
    struct kbdmon *kbd =
        container_of(handle, struct kbdmon, handle);

    pr_info("[kbdmon] Disconnected device\n");

    input_close_device(handle);
    input_unregister_handle(handle);

    kfree(kbd);
}

/* -------------------------------------------------------------------------- */
/* Device ID table                                                            */
/* -------------------------------------------------------------------------- */

static const struct input_device_id kbdmon_ids[] = {
    {
        .flags = INPUT_DEVICE_ID_MATCH_EVBIT |
                 INPUT_DEVICE_ID_MATCH_KEYBIT,

        /*
         * Match devices supporting EV_KEY events.
         */
        .evbit = { BIT_MASK(EV_KEY) },
        .keybit = { [BIT_WORD(KEY_A)] = BIT_MASK(KEY_A) },
    },
    { },
};

MODULE_DEVICE_TABLE(input, kbdmon_ids);

/* -------------------------------------------------------------------------- */
/* Input handler definition                                                   */
/* -------------------------------------------------------------------------- */

static struct input_handler kbdmon_handler = {
    .event      = kbdmon_event,
    .connect    = kbdmon_connect,
    .disconnect = kbdmon_disconnect,
    .name       = "kbdmon",
    .id_table   = kbdmon_ids,
};

/* -------------------------------------------------------------------------- */
/* Module init/exit                                                           */
/* -------------------------------------------------------------------------- */

static int __init kbdmon_init(void)
{
    int ret;

    pr_info("[kbdmon] Initializing\n");

    misc_register(&kbdmon_dev);

    ret = input_register_handler(&kbdmon_handler);
    if (ret) {
        pr_err("[kbdmon] Failed to register handler\n");
        return ret;
    }

    pr_info("[kbdmon] Handler registered\n");

    return 0;
}

static void __exit kbdmon_exit(void)
{
    pr_info("[kbdmon] Exiting\n");

    input_unregister_handler(&kbdmon_handler);
    misc_deregister(&kbdmon_dev);
}

module_init(kbdmon_init);
module_exit(kbdmon_exit);