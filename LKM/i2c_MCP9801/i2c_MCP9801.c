#include <linux/module.h>
#include <linux/init.h>
#include <linux/fs.h>
#include <linux/cdev.h>
#include <linux/uaccess.h>
#include <linux/slab.h>
#include <linux/i2c.h>
#include <linux/kernel.h>

#define DRIVER_NAME "mcp9801"
#define DRIVER_CLASS "MCP9801Class"

/* Meta information */
MODULE_LICENSE("GPL");
MODULE_AUTHOR("DLH");
MODULE_DESCRIPTION("Control a MCP9801 temperature sensor.");

/* Vars for device and device class */
static dev_t my_device_number;
static struct class *my_class;
static struct cdev my_device;

u8 buffer[2] = {42,42};

/* i2c */
static struct i2c_adapter * mcp9801_i2c_adapter = NULL;
static struct i2c_client * mcp9801_i2c_client = NULL;

/* I2C bus on the BBB */
#define I2C_BUS_AVAILABLE 2
#define SLAVE_DEVICE_NAME "MCP9801"
#define MCP9801_SLAVE_ADDRESS 0x4E

#define RESOLUTION_12B 0x60

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

uint16_t read_temperature(void) {
	uint16_t temp;
	temp = i2c_smbus_read_byte_data(mcp9801_i2c_client, 0x0);

	return 25;
}

/* Read callback */
static ssize_t driver_read(struct file *File, char *user_buffer, size_t count, loff_t *offs) {
	int to_copy, not_copied, delta;
	char out_string[20];
	uint16_t temperature;

	/* Amount of data to copy */
	to_copy = min(count, sizeof(out_string));

	temperature = read_temperature();
	snprintf(out_string, sizeof(out_string), "%d\n", temperature);

	/* Copy data to user, returns the amount of byte that weren't copied. */
	not_copied = copy_to_user(user_buffer, out_string, to_copy);

	/* Calculate delta */
	delta = to_copy - not_copied;

	return delta;
}

/* Open callback. */
static int driver_open(struct inode *device_file, struct file *instance){
	printk("MCP9801 module - open was called!\n");

	return 0;
}

static int driver_close(struct inode *device_file, struct file *instance){
	printk("MCP9801 module - close was called!\n");

	return 0;
}

static struct file_operations fops = {
	.owner = THIS_MODULE,
	.open = driver_open,
	.release = driver_close,
	.read = driver_read
};

static int __init ModuleInit(void) {
	/* printk writes to dmesg */
	printk("Hi, I'm a new LKM!\n");

	/* Allocate device number */
	if( alloc_chrdev_region(&my_device_number, 0, 1, DRIVER_NAME) < 0) {
		printk("Device number could not be allocated\n");
		return -1;
	}
	printk("MCP9801: Device number major: %d, minor: %d registered\n", MAJOR(my_device_number), MINOR(my_device_number));

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
	if(i2c_smbus_write_byte_data(mcp9801_i2c_client, 0x1, RESOLUTION_12B) < 0) {
		printk("Failed to change config\n");
	}
	printk("Temp (byte_data): %d\n", i2c_smbus_read_byte_data(mcp9801_i2c_client, 0x0));
	printk("Temp (word_data): %d\n", i2c_smbus_read_word_data(mcp9801_i2c_client, 0x0));
	printk("Temp (byte): %d\n", i2c_smbus_read_byte(mcp9801_i2c_client));
	printk("Buffer: %d %d\n", buffer[0], buffer[1]);
	i2c_smbus_read_i2c_block_data_or_emulated(mcp9801_i2c_client, 0x0, 2, buffer);
	printk("Buffer: %d %d\n", buffer[0], buffer[1]);


	return 0;

i2cError:
	cdev_del(&my_device);

AddError:
	device_destroy(my_class, my_device_number);

FileError:
	class_destroy(my_class);

ClassError:
	unregister_chrdev(my_device_number, DRIVER_NAME);
	return -1;
}

static void __exit ModuleExit(void) {
	i2c_unregister_device(mcp9801_i2c_client);
	i2c_del_driver(&mcp_driver);
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