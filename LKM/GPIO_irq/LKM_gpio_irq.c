#include <linux/module.h>
#include <linux/init.h>
#include <linux/fs.h>
#include <linux/cdev.h>
#include <linux/uaccess.h>
#include <linux/gpio.h>
#include <linux/interrupt.h>

/* P9_27 */
#define GPIO_BUTTON 115
/* P9_23 */
#define GPIO_LED 49

/* Meta information */
MODULE_LICENSE("GPL");
MODULE_AUTHOR("DLH");
MODULE_DESCRIPTION("Control and LED with a button using interrupts");

unsigned int irq_number;
static bool ledStatus = 0;

static irq_handler_t gpio_irq_handler(unsigned int irq, void *dev_id, struct pt_regs *regs) {
	printk("Triggered interrupt\n");
	ledStatus = !ledStatus;
	gpio_set_value(GPIO_LED, ledStatus);
	return (irq_handler_t) IRQ_HANDLED;
}

static int __init ModuleInit(void) {
	/* printk writes to dmesg */
	printk("Hi, I'm a new LKM for GPIOs!\n");

	/* LED on GPIO 49 (pin 23 on header P9) */
	if(gpio_request(GPIO_LED, "bbb-gpio-49")) {
		printk("Can not allocate GPIO %d\n", GPIO_LED);
	}
	/* Set GPIO direction */
	/* Second argument is the value at initiatlisation, 0 or 1. */
	if(gpio_direction_output(GPIO_LED, 0)) {
		printk("Can not set GPIO %d to output\n", GPIO_LED);
		goto GPIOLEDError;
	}

	/* Button on GPIO 117 (pin xx on header P9) */
	if(gpio_request(GPIO_BUTTON, "bbb-gpio-115")) {
		printk("Can not allocate GPIO %d\n", GPIO_BUTTON);
		goto GPIOLEDError;
	}
	/* Set GPIO direction */
	if(gpio_direction_input(GPIO_BUTTON)) {
		printk("Can not set GPIO %d to output\n", GPIO_BUTTON);
		goto GPIOButtonError;
	}
	/* Debounce the button */
	/* Second argument is the duration for which to ignore the bounces. */
	gpio_set_debounce(GPIO_BUTTON, 100);
	/* Export GPIO so that it appearts in SYSFS */
	gpio_export(GPIO_LED, false);
	gpio_export(GPIO_BUTTON, false);

	irq_number = gpio_to_irq(GPIO_BUTTON);

	/* Triggers are defined in linux/interrupt.h */
	/* https://elixir.bootlin.com/linux/v4.19.94/source/include/linux/interrupt.h */
	if(request_irq(irq_number, (irq_handler_t) gpio_irq_handler, IRQF_TRIGGER_RISING, "custom_gpio_irq", NULL) != 0) {
		printk("Failed to request IRQ %d\n", irq_number);
		goto GPIOButtonError;
	}


	return 0;

GPIOButtonError:
	gpio_free(GPIO_BUTTON);

GPIOLEDError:
	gpio_free(GPIO_LED);
	return -1;
}

static void __exit ModuleExit(void) {
	free_irq(irq_number, NULL);
	gpio_set_value(GPIO_LED, 0); /* Set the GPIO value to 0 before freeing it. */
	gpio_unexport(GPIO_LED);
	gpio_unexport(GPIO_BUTTON);
	gpio_free(GPIO_LED);
	gpio_free(GPIO_BUTTON);
	printk("Goodbye, Kernel :'(\n");
}

/* Define entry point */
module_init(ModuleInit);
/* Define exit point */
module_exit(ModuleExit);