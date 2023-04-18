#include <linux/module.h>
#include <linux/init.h>
#include <linux/fs.h>
#include <linux/cdev.h>
#include <linux/uaccess.h>
#include <linux/slab.h>
#include <linux/i2c.h>
#include <linux/kernel.h>
#include <linux/delay.h>

#define DRIVER_NAME "eeprom_24LC256"
#define DRIVER_CLASS "EEPROMClass"

/* Meta information */
MODULE_LICENSE("GPL");
MODULE_AUTHOR("DLH");
MODULE_DESCRIPTION("Control an EEPROM 24LC256.");

/* Vars for device and device class */
static dev_t my_device_number;
static struct class *my_class;
static struct cdev my_device;


/* i2c */
static struct i2c_adapter * eeprom_i2c_adapter = NULL;
static struct i2c_client * eeprom_i2c_client = NULL;

/* I2C bus on the BBB */
#define I2C_BUS_AVAILABLE 2
#define SLAVE_DEVICE_NAME "24LC256"
#define EEPROM_SLAVE_ADDRESS 0x50

/* EEPROM commands */
#define EEPROM_READ 0xA1
#define EEPROM_WRITE 0xA0

const char base_address[2] = {0x00, 0x00};
const char buf[4] = {0x00, 0x00, 0x50, 0x51};
char recv_buf[1] = {0x00};

static const struct i2c_device_id eeprom_id[] = {
	{ SLAVE_DEVICE_NAME, 0 }, 
	{ }
};

static struct i2c_driver eeprom_driver = {
	.driver = {
		.name = SLAVE_DEVICE_NAME,
		.owner = THIS_MODULE
	}
};

static struct i2c_board_info eeprom_i2c_board_info = {
	I2C_BOARD_INFO(SLAVE_DEVICE_NAME, EEPROM_SLAVE_ADDRESS)
};

uint16_t read_temperature(void) {
	uint16_t temp;
	temp = i2c_smbus_read_byte_data(eeprom_i2c_client, 0x0);

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
	printk("EEPROM 24LC256: Device number major: %d, minor: %d registered\n", MAJOR(my_device_number), MINOR(my_device_number));

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

	eeprom_i2c_adapter = i2c_get_adapter(I2C_BUS_AVAILABLE);
	if(eeprom_i2c_adapter != NULL) {
		eeprom_i2c_client = i2c_new_device(eeprom_i2c_adapter, &eeprom_i2c_board_info);
		if(eeprom_i2c_client != NULL) {
			if(i2c_add_driver(&eeprom_driver) == -1) {
				printk("Can't add EEPROM 24LC256 driver\n");
				goto i2cError;
			}
		}
		i2c_put_adapter(eeprom_i2c_adapter);
	}
	printk("24LC256 driver added!\n");
	
	/* It does not work to send the address, then send the data. A STOP signal is being sent by the master in between, so the next time the client is reveiving a WRITE command, it's expecting an adress again.
	This means the first two bytes of the message need to be the memory address. */
	i2c_master_send(eeprom_i2c_client, buf, 4);
	/* Doing a sleep here is ugly AF, but it's necessary otherwise the master tries to read before the slave got enough time to write its buffer to memory. I should find a way to wait for the slave ACK before accessing it again. */
	msleep(5);
	i2c_master_send(eeprom_i2c_client, base_address, 2);
	printk("Reading from eeprom: %d\n",i2c_master_recv(eeprom_i2c_client, recv_buf, 1));
	printk("Received the byte %x\n", recv_buf[0]);



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
	i2c_unregister_device(eeprom_i2c_client);
	i2c_del_driver(&eeprom_driver);
	cdev_del(&my_device);
	device_destroy(my_class, my_device_number);
	class_destroy(my_class);
	unregister_chrdev(my_device_number, DRIVER_NAME);
	printk("24LC256: module unloaded\n");
}

/* Define entry point */
module_init(ModuleInit);
/* Define exit point */
module_exit(ModuleExit);