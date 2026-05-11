#include <linux/module.h>
#include <linux/init.h>
#include <linux/input.h>
#include <linux/slab.h>

MODULE_LICENSE("GPL");
MODULE_DESCRIPTION("Simple keyboard monitor using input_handler // Sequence detector");
MODULE_VERSION("1.0");

/* -------------------------------------------------------------------------- */
/* Per-device connection structure                                            */
/* -------------------------------------------------------------------------- */

struct kbdmon {
    struct input_handle handle;
};

/* -------------------------------------------------------------------------- */
/* Optional keycode -> name helper                                            */
/* -------------------------------------------------------------------------- */

static const char *key_name(unsigned int code)
{
    switch (code) {
    case KEY_A: return "A";
    case KEY_B: return "B";
    case KEY_C: return "C";
    case KEY_D: return "D";
    case KEY_E: return "E";
    case KEY_F: return "F";
    case KEY_G: return "G";
    case KEY_H: return "H";
    case KEY_I: return "I";
    case KEY_J: return "J";
    case KEY_K: return "K";
    case KEY_L: return "L";
    case KEY_M: return "M";
    case KEY_N: return "N";
    case KEY_O: return "O";
    case KEY_P: return "P";
    case KEY_Q: return "Q";
    case KEY_R: return "R";
    case KEY_S: return "S";
    case KEY_T: return "T";
    case KEY_U: return "U";
    case KEY_V: return "V";
    case KEY_W: return "W";
    case KEY_X: return "X";
    case KEY_Y: return "Y";
    case KEY_Z: return "Z";

    case KEY_1: return "1";
    case KEY_2: return "2";
    case KEY_3: return "3";
    case KEY_4: return "4";
    case KEY_5: return "5";
    case KEY_6: return "6";
    case KEY_7: return "7";
    case KEY_8: return "8";
    case KEY_9: return "9";
    case KEY_0: return "0";

    case KEY_ENTER: return "ENTER";
    case KEY_SPACE: return "SPACE";
    case KEY_ESC: return "ESC";
    case KEY_BACKSPACE: return "BACKSPACE";
    case KEY_TAB: return "TAB";

    case KEY_LEFTSHIFT: return "LSHIFT";
    case KEY_RIGHTSHIFT: return "RSHIFT";
    case KEY_LEFTCTRL: return "LCTRL";
    case KEY_RIGHTCTRL: return "RCTRL";
    case KEY_LEFTALT: return "LALT";
    case KEY_RIGHTALT: return "RALT";

    default:
        return "UNKNOWN";
    }
}

/* -------------------------------------------------------------------------- */
/* Event callback                                                             */
/* -------------------------------------------------------------------------- */

struct sequence_buffer{
    int ctrl_down;
    int alt_down;
};

static struct sequence_buffer seq_buf;

static void kbdmon_event(struct input_handle *handle,
                         unsigned int type,
                         unsigned int code,
                         int value)
{
    /*
     * value meanings:
     *   0 = release
     *   1 = press
     *   2 = autorepeatq
     */

    if (type != EV_KEY)
            return;

    if (code == KEY_LEFTCTRL)
        seq_buf.ctrl_down = value;

    if (code == KEY_LEFTALT)
        seq_buf.alt_down = value;

    if (seq_buf.ctrl_down && seq_buf.alt_down &&
        code == KEY_K && value == 1) {

        pr_info("SECRET HOTKEY DETECTED\n");
    }
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
        .flags = INPUT_DEVICE_ID_MATCH_EVBIT,

        /*
         * Match devices supporting EV_KEY events.
         */
        .evbit = { BIT_MASK(EV_KEY) },
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
}

module_init(kbdmon_init);
module_exit(kbdmon_exit);