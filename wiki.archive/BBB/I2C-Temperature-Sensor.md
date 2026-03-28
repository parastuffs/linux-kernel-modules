This module is for a `MCP9801` temperature sensor from Microchip. Keep the [documentation](https://ww1.microchip.com/downloads/en/devicedoc/21909d.pdf) at hand, you will need it to understand how it works.

## Understanding the `MCP9801`
The device is chipped in an 8-pin package:
```
        ------
SDA   --|*   |-- VDD
SCLK  --|    |-- A0
ALERT --|    |-- A1
GND   --|____|-- A2
```

The sensor can give you the temperature on the I²C bus and trigger an alert (on the eponymous pin) if a particular temperature is reached.

The device has several registers that we can access using an 8-bit register pointer.

- `0x00` is a read-only register `TA` and holds the current temperature value
- `0x01` is the configuration register `CONFIG`
- `0x02` is the temperature hysteresis configuration
- `0x03` is the temperature limit setting register

`TA` is a two-bytes register with the high order bits holding the positive value of the temperature, the low order 8 bits its decimals, depending on the ADC resolution.

- MSB: `<Sign 2⁶°C 2⁵ 2⁴ 2³ 2² 2¹ 2⁰>`
- LSB: `<2⁻¹ 2⁻² 2⁻³ 2⁻⁴ 0 0 0 0>`

`CONFIG` is an 8-bit register with each field reserved for a specific purpose:

- [7] (MSB) One shot, 0 (default) or 1: do a single conversion while in shutdown mode.
- [6:5] Resolution, 00 for 9-bit (default, 0.5°C), 01 for 10-bit (0.25°C), 10 for 11-bit (0.125°C) and 11 for 12-bit (0.0625°C).
- [4:3] Fault queue size for ALERT.
- [2] ALERT polarity.
- [1] Comparison (0, default) or interrupt mode (1).
- [0] Shutdown mode.

The device address is defined on 8 bits: `y1001xxx`.

- `y` is `0` for a write operation and `1` for a read.
- `1001` is a constant that cannot be changed.
- `xxx` is the device bits set by `A2 A1 A0` by tying them either high (VDD) or low (GND).



## Wiring up the sensor
**Check in the documentation if `SDA` and `SCLK` need pull-up resistors.**
1. Choose an address for your sensor by tying `A*` to VDD or GND. If you want to use several sensors on the same bus, those addresses need to be different.
2. Check the board [documentation](https://cdn.sparkfun.com/datasheets/Dev/Beagle/BBB_SRM_C.pdf) to know which GPIOs are the I²C bus. For example, the second bus has `SCLK` on `P9_19` and `SDA` on `P9_20`.
3. Connect VDD to the BBB 5V power supply `SYS_5V` and GND.
4. Connect the ALERT pin on a GPIO if you intend to use it.



## Talking with the sensor from CLI
First check the i2c buses availabe:
```sh
debian@beaglebone:~$ dmesg | grep i2c
[    0.276502] omap_i2c 4802a000.i2c: bus 1 rev0.11 at 100 kHz
[    0.278104] omap_i2c 4819c000.i2c: bus 2 rev0.11 at 100 kHz
[    1.261980] i2c /dev entries driver
[    1.567326] input: tps65217_pwr_but as /devices/platform/ocp/44e0b000.i2c/i2c-0/0-0024/tps65217-pwrbutton/input/input0
[    1.569461] omap_i2c 44e0b000.i2c: bus 0 rev0.11 at 400 kHz
debian@beaglebone:~$ i2cdetect -l
i2c-1   i2c             OMAP I2C adapter                        I2C adapter
i2c-2   i2c             OMAP I2C adapter                        I2C adapter
i2c-0   i2c             OMAP I2C adapter                        I2C adapter
```

If you followed the guide up to this point, you should be using `bus 2`. Check if the device if correctly wired up:
```sh
debian@beaglebone:~$ i2cdetect -y -r 2
     0  1  2  3  4  5  6  7  8  9  a  b  c  d  e  f
00:          -- -- -- -- -- -- -- -- -- -- -- -- -- 
10: -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- 
20: -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- 
30: -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- 
40: -- -- -- -- -- -- -- -- -- -- -- -- -- -- 4e -- 
50: -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- 
60: -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- 
70: -- -- -- -- -- -- -- -- 
```
My device has the `A*` pins configured as `110`, hence the `0b01001110 = 0x4e`.

We can use `i2cget` to read bytes from the device and get its temperature.
Since the temperature is stored on two bytes, the following call only returns the first one, which is storing the integer part:
```sh
debian@beaglebone:~$ i2cget -y 2 0x4e
0x17
```
`0x17` is 23 in hexadecimal.

After heating the sensor up a little bit:
```
debian@beaglebone:~$ i2cget -y 2 0x4e
0x1b
```
`0x1b` is 27 in hexadecimal.

You can get both bytes at once using the appropriate arguments:
```sh
debian@beaglebone:~/LKM/i2c_MCP9801$ i2cget -y 2 0x4e 0x00 w
0x8019
```
Which sends the result in little-endian and should be read as 25.5°C



## Writing an LKM for the sensor
Our driver module will make use of a character device, so you can start from the [basic LKM on this wiki](https://github.com/parastuffs/linux-kernel-modules/wiki/Character-Device-Driver).

First of all, we need to include `<linux/i2c>` to have access to te relevant structures and functions.

We then need a bunch of definitions for the bus and device:
```c
#define I2C_BUS_AVAILABLE 2         /* i2c bus used */
#define SLAVE_DEVICE_NAME "MCP9801" /* Name of the device, arbitrary */
#define MCP9801_SLAVE_ADDRESS 0x4E  /* Address of the device */
```

An i2c device requires a few global structures:
```c
static struct i2c_adapter * mcp9801_i2c_adapter = NULL;
static struct i2c_client * mcp9801_i2c_client = NULL;

static const struct i2c_device_id mcp_id[] = {
	{ SLAVE_DEVICE_NAME, 0 }, 
	{ }
};

static struct i2c_driver mcp_driver = {
	.driver = {
		.name = SLAVE_DEVICE_NAME,
		.owner = THIS_MODULE
	}
};

static struct i2c_board_info mcp_i2c_board_info = {
	I2C_BOARD_INFO(SLAVE_DEVICE_NAME, MCP9801_SLAVE_ADDRESS)
};
```

In the `__init` function, after the creation of the character device, we need to
1. Create an adapter with `i2c_get_adapter()`
2. Create a new device with `i2c_new_device()`
3. Add a driver for the device with `i2c_add_driver()`
4. Make the adapter available with `i2c_put_adapter()`

Written properly, it would look like this:
```c
mcp9801_i2c_adapter = i2c_get_adapter(I2C_BUS_AVAILABLE);
if(mcp9801_i2c_adapter != NULL) {
	mcp9801_i2c_client = i2c_new_device(mcp9801_i2c_adapter, &mcp_i2c_board_info);
	if(mcp9801_i2c_client != NULL) {
		if(i2c_add_driver(&mcp_driver) == -1) {
			printk("Can't add MCP driver\n");
			goto i2cError;
		}
	}
	i2c_put_adapter(mcp9801_i2c_adapter);
}
printk("MCP9801 driver added!\n");
```

As usual, when removing the module from the kernel, we need to clean up:
```c
i2c_unregister_device(mcp9801_i2c_client);
i2c_del_driver(&mcp_driver);
```


The [kernel documentation](https://docs.kernel.org/driver-api/i2c.html) describes a bunch of functions that will help you read and write data on the device. You don't actually need to send the start/stop signals by hand if you can avoid to do so.

Let us first update the configuration to change the resolution to 12 bits. Check the [documentatio](https://docs.kernel.org/driver-api/i2c.html#c.i2c_smbus_write_byte_data) to better understand the functions parameters.
```c
if(i2c_smbus_write_byte_data(mcp9801_i2c_client, 0x1, 0x60) < 0) {
	printk("Failed to change config\n");
}
```

To actually get data, we can use the following functions:
```c
/* Get 8 bits of data */
printk("Temp (byte_data): %d\n", i2c_smbus_read_byte_data(mcp9801_i2c_client, 0x0));
/* Get 16 bits of data at once */
printk("Temp (word_data): %d\n", i2c_smbus_read_word_data(mcp9801_i2c_client, 0x0));
/* Get two bytes of data and put them in buffer */
u8 buffer[2] = {42,42};
i2c_smbus_read_i2c_block_data_or_emulated(mcp9801_i2c_client, 0x0, 2, buffer);
```







A code example can be found [in the repository](https://github.com/parastuffs/linux-kernel-modules/blob/main/LKM/i2c_MCP9801/i2c_MCP9801.c).


## Going further and challenges
- Add a callback to write data on the device. For example, allow the user to change the configuration of the sensor.
- Use the ALERT pin to trigger an interruption on a GPIO and light an LED on another GPIO.
- Create a dump of the sensor behaviour and create a dummy device for offline testing: [https://hackerbikepacker.com/i2c-on-linux](https://hackerbikepacker.com/i2c-on-linux)


## Resources
- [Kernel I2C API](https://docs.kernel.org/driver-api/i2c.html)
- [BBB SRM](https://cdn.sparkfun.com/datasheets/Dev/Beagle/BBB_SRM_C.pdf)
- [MCP9801 datasheet](https://ww1.microchip.com/downloads/en/devicedoc/21909d.pdf)
- [Embedded Linux Wiki](https://elinux.org/EBC_I2C)
- [List of I2C device addresses](https://i2cdevices.org/addresses)
- [Let's code a Linux Driver - 7: BMP280 Driver (I2C Temperature Sensor)](https://www.youtube.com/watch?v=j-zo1QOBUZg)
