This module will control a [1602 LCD screen](https://www.openhacks.com/uploadsproductos/eone-1602a1.pdf).
It uses the board's GPIOs to send data to the screen using individual wires for each bit.
If you did not implement your first [GPIO LKM](https://github.com/parastuffs/linux-kernel-modules/wiki/Control-GPIOs), now would be a good time as the present module will be based heavily on it.

## Pinout
Check the [documentation](https://www.openhacks.com/uploadsproductos/eone-1602a1.pdf) of your screen to determine the exact pinout.
It should be something like the following:

| Pin | Label | Role |
| ---- | ---- | ---- |
| 1 | VSS | GND |
| 2 | VDD | 5V |
| 3 | V0 | LCD Contrast: [0, VDD] using a potentiometer |
| 4 | RS | Register Select, H: data, L: instruction |
| 5 | RW | Read/Write, H: read, L: write |
| 6 | E | Enable |
| 7 | D0 | Data bit 0 |
| 8 | D1 | Data bit 1 |
| 9 | D2 | Data bit 2 |
| 10 | D3 | Data bit 3 |
| 11 | D4 | Data bit 4 |
| 12 | D5 | Data bit 5 |
| 13 | D6 | Data bit 6 |
| 14 | D7 | Data bit 7 |
| 15 | A | Backlight anode (VDD) |
| 16 | K | Backlight cathode (GND) |

**Check if your LCD board already has a resistor in series with the backlight anode. If not, add one yourself!**

On the BBB side, there *seems* to be a lot of possibilities, as illustrated on the [pinout of the board](https://github.com/parastuffs/linux-kernel-modules/wiki/Control-GPIOs).
However, some GPIOs are already in use by other parts of the kernel!
We can find out which are free by checking their listing:
```sh
$ cat /sys/kernel/debug/gpio
```
The output displays the available pins by showing their physical number, e.g. `P9_30`.

On `P8`, the following pins are available:

| GPIO|22|23|26|27|44|45|46|47|61|65|66|67|68|69|
| --  |--|--|--|--|--|--|--|--|--|--|--|--|--|--|
|`P8_`|19|13|14|17|12|11|16|15|26|18|07|08|10|09|



For the code examples of this page, we will use the following connections:

| LCD | BBB |
| --- | --- |
| 1 | P9_1 (DGND) |
| 2 | P9_7 (SYS_5V) |
| 3 | Potentiometer on a breadboard |
| 4 | P8_17 (GPIO_27) |
| 5 | P8_18 (GPIO_65) |
| 6 | P8_15 (GPIO_47) |
| 7 | P8_16 (GPIO_46) |
| 8 | P8_13 (GPIO_23) |
| 9 | P8_14 (GPIO_26) |
| 10 | P8_11 (GPIO_45) |
| 11 | P8_12 (GPIO_44) |
| 12 | P8_09 (GPIO_69) |
| 13 | P8_10 (GPIO_68) |
| 14 | P8_07 (GPIO_66) |
| 15 | P9_7 (SYS_5V) |
| 16 | P9_1 (DGND) |


## Write the LKM
Starting from the [GPIO LKM](https://github.com/parastuffs/linux-kernel-modules/wiki/Control-GPIOs), we first need to define a bunch of GPIOs for the LCD.
```C
unsigned char gpios[] = {
	27, /* Register Select */
	65, /* R/W */
	47, /* Enable */
	46, /* D0 */
	23, /* D1 */
	26, /* D2 */
	45, /* D3 */
	44, /* D4 */
	69, /* D5 */
	68, /* D6 */
	66 /* D7 */
};
```

Investigating the documentation, we can notice that there are two modes for the screen: instruction and data. The mode is selected using the `RS` pin. We can then send a full byte of data (an instruction or a character to display) by setting each of the 8 data pins.
```C
#include <linux/delay.h>

/* Latch the screen by sending a pulse on the Enable pin.
   This function should be called when we want the screen
   to acknowledge new data on its inputs.
*/
void lcd_enable(void) {
	gpio_set_value(gpios[2], 1);
	msleep(5);
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
```


To write data on the LCD from the user space, we will use the same `driver_write()` callback as usual.
```C
/* Buffer for LCD, 16 characters per line */
static char lcd_buffer[17];
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
```


The `__init` and `__exit` functions are business as usual, except that we send some data to the screen to set it up.
```C
static int __init ModuleInit(void) {
	int i;
	/* printk writes to dmesg */
	char *names[] = {"REGISTER_SELECT", "READ_WRITE_PIN", "ENABLE_PIN", "DATA_PIN0", "DATA_PIN1", "DATA_PIN2", "DATA_PIN3", "DATA_PIN4", "DATA_PIN5", "DATA_PIN6", "DATA_PIN7"};
	
	/* DEVICE INITIALISATION CODE GOES HERE! */

	for(i=0; i<11; i++) {
		if(gpio_request(gpios[i], names[i]))
			goto GPIORequestError;
	}

	for(i=0; i<11; i++) {
		if(gpio_direction_output(gpios[i], 0))
			goto GPIODirectionError;
	}

	for(i=0; i<11; i++)
		gpio_export(gpios[i], false);

	/* Init screen */
	lcd_instruction(0x30); /* 8 bits data */
	lcd_instruction(0xF); /* Turn on, cursor on, cursor blinking */
	lcd_instruction(0x1); /* Clear display */

	return 0;

GPIODirectionError:
	i=10;

GPIORequestError:
	cdev_del(&my_device);
	for(; i>=0; i--)
		gpio_free(gpios[i]);

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
}
```


## Write on the display
Once the module has been loaded and the user can actually write on the device `/dev/LKM_lcd`, a simple `echo` command is enough:
```sh
debian@beaglebone:~$ echo "Hello :)" > /dev/LKM_lcd
```





## Going further and challenges
Explore the documentation to use the other modes and setting of the screen. For example, try to
- Choose the line on which to write
- Read the data displayed on the screen
- Append a character to the string already displayed
- Add buttons to move the cursor on the screen


## Resource
- The full code is available [in the repository](https://github.com/parastuffs/linux-kernel-modules/blob/main/LKM/GPIO_LCD/LKM_lcd.c).
- [Let's code a Linux Driver - 5: LCD text display driver (HD44780)](https://www.youtube.com/watch?v=HH3OOtJwBz4)