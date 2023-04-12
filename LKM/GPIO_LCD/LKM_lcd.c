#include <linux/module.h>
#include <linux/init.h>
#include <linux/fs.h>
#include <linux/cdev.h>
#include <linux/uaccess.h>
#include <linux/gpio.h>
#include <linux/delay.h>

/* Meta information */
MODULE_LICENSE("GPL");
MODULE_AUTHOR("DLH");
MODULE_DESCRIPTION("Write stuff on a 1602C LCD screen.");

/* Major device number, chosen among free values in /proc/devices */
#define DRIVER_NAME "LKM_lcd"
#define DRIVER_CLASS "MyModuleClass"

/* Vars for device and device class */
static dev_t my_device_number;
static struct class *my_class;
static struct cdev my_device;

/* Pinout for the LCD.
   gpio_request() requires an unsigned int,
   but unsigned char uses less space.
*/
unsigned char gpios[] = {
	33, /* Register Select */
	62, /* R/W */
	63, /* Enable */
	27, /* D0 */
	46, /* D1 */
	23, /* D2 */
	44, /* D3 */
	69, /* D4 */
	67, /* D5 */
	34, /* D6 */
	39 /* D7 */
};

/* Buffer for LCD, 16 characters per line */
static char lcd_buffer[17];

/* Latch the screen by sending a pulse on the Enable pin.
   This function should be called when we want the screen
   to acknowledge new data on its inputs.
*/
void lcd_enable(void) {
	gpio_set_value(gpios[2], 1);
	msleep(5); /* 1ms should be enough */
	gpio_set_value(gpios[2], 0);
}

/* Set each of the 8 data pins of the LCD with the data
   provided. This function is mode-agnostic, i.e. it works
   for both instructions and characters to display.
*/
void lcd_send_byte(uint8_t data) {
	int i;
	for(i=0; i<8; i++) {
		/* Data pins start at index 3 of gpios[] */
		/* We send one bit of the byte to each pin. */
		gpio_set_value(gpios[3+i], ( data & (1<<i) ) >> i);
	}
	/* Latch the screen */
	lcd_enable();
	msleep(5);
}

/* Send an instruction to the LCD */
void lcd_instruction(uint8_t data) {
	/* First set it to 'instruction' mode */
	gpio_set_value(gpios[0], 0);
	lcd_send_byte(data);
}

/* Send data to the LCD */
void lcd_data(uint8_t data) {
	/* Set to 'data' mode */
	gpio_set_value(gpios[0], 1);
	lcd_send_byte(data);
}

/* Write callback */
static ssize_t driver_write(struct file *File, const char *user_buffer, size_t count, loff_t *offs) {
	int to_copy, not_copied, i;

	/* Amount of data to copy */
	to_copy = min(count, sizeof(lcd_buffer));

	/* Copy data to user, returns the amount of byte that weren't copied. */
	not_copied = copy_from_user(&lcd_buffer, user_buffer, to_copy);

	/* Wipe the screen */
	lcd_instruction(0x1);

	for(i=0; i<to_copy; i++) {
		lcd_data(lcd_buffer[i]);
	}

	return to_copy - not_copied;
}

/* Open callback. */
static int driver_open(struct inode *device_file, struct file *instance){
	printk("LKM_lcd - open was called!\n");

	return 0;
}

static int driver_close(struct inode *device_file, struct file *instance){
	printk("LKM_lcd - close was called!\n");

	return 0;
}

static struct file_operations fops = {
	.owner = THIS_MODULE,
	.open = driver_open,
	.release = driver_close,
	.write = driver_write
};

static int __init ModuleInit(void) {
	int i;
	/* printk writes to dmesg */
	char *names[] = {"REGISTER_SELECT", "READ_WRITE_PIN", "ENABLE_PIN", "DATA_PIN0", "DATA_PIN1", "DATA_PIN2", "DATA_PIN3", "DATA_PIN4", "DATA_PIN5", "DATA_PIN6", "DATA_PIN7"};
	printk("Hi, I'm a new LKM for a 1602C LCD screen!\n");

	/* Allocate device number */
	if( alloc_chrdev_region(&my_device_number, 0, 1, DRIVER_NAME) < 0) {
		printk("Device number could not be allocated\n");
		return -1;
	}
	printk("LKM_lcd: Device number major: %d, minor: %d registered\n", my_device_number>>20, my_device_number&0xfffff);

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

	/* GPIOs init */
	for(i=0; i<11; i++) {
		if(gpio_request(gpios[i], names[i])) {
			printk("Can not allocate GPIO %d\n", gpios[i]);
			goto GPIORequestError;
		}
	}

	/* Set GPIO direction */
	/* Second argument is the value at initiatlisation, 0 or 1. */
	for(i=0; i<11; i++) {
		if(gpio_direction_output(gpios[i], 0)) {
			printk("Can not set GPIO %d to output\n", gpios[i]);
			goto GPIODirectionError;
		}
	}
	/* Note that we don't need to do anything more with the R/W pin
	   for now if we only want to write on the display. If the R/W
	   pin is set to low (as it is from our gpio_direction_output
	   call above), the display is set to 'write'.
	*/

	/* Export GPIO so that it appearts in SYSFS */
	for(i=0; i<11; i++) {
		gpio_export(gpios[i], false);
	}

	/* Init screen */
	/* 8 bits data */
	lcd_instruction(0x30);
	/* Turn on, cursor on, cursor blinking */
	lcd_instruction(0xF);
	/* Clear display */
	lcd_instruction(0x1);


	return 0;

GPIODirectionError:
	/* Set i to the amount of GPIOs so that they can be freed in
	   the following label GPIORequestError
	*/
	i=10;

GPIORequestError:
	cdev_del(&my_device);
	for(; i>=0; i--) {
		gpio_free(gpios[i]);
	}

AddError:
	device_destroy(my_class, my_device_number);

FileError:
	class_destroy(my_class);

ClassError:
	unregister_chrdev(my_device_number, DRIVER_NAME);
	return -1;
}

static void __exit ModuleExit(void) {
	int i;
	/* Clear display */
	lcd_instruction(0x1);
	for(i=0; i<11; i++) {

		gpio_set_value(gpios[i], 0); /* Set the GPIO value to 0 before freeing it. */
		gpio_unexport(gpios[i]);
		gpio_free(gpios[i]);
	}
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