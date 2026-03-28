One of the fundamental devices we can control using a driver is a **character device**.
By registering a device as such, we can communicate with it using basic `read` and `write` system calls.

In this first introduction, we will create a dummy device that simply stores what we send it (`write`) and can spit it back upon request (`read`).

## Major and minor numbers
Devices have a major and a minor number. The former is unique to a category of devices, the latter is unique within a device group and is handled at the discretion of the group.

`/proc/devices` holds the major numbers currently in use by the OS, whilst `/usr/include/linux/major.h` lists reserved major numbers.

When creating your own device, you could simply pick an available major number, but this is **ad practice**. Your code becomes less portable (nothing guarantees your that this specific value is not reserved no another platform) and harder to maintain (what if the value you picked becomes reserved upstream in the future?).
Instead we will see how to gently ask the kernel for an available value and work with what we get.


## Device driver

First of all, we need a **file operations structure** that defines the function callbacks for various actions.
The structure is defined in `<linux/fs.h>`.
```C
static struct file_operations fops = {
	.owner = THIS_MODULE,
	.open = driver_open,
	.release = driver_close,
	.read = driver_read,
	.write = driver_write
};
```

We need two functions to **open and close** the device. In this instance, they don't do anything particular.
```C
static int driver_open(struct inode *device_file, struct file *instance){
	printk("read-write module - open was called!\n");
	return 0;
}

static int driver_close(struct inode *device_file, struct file *instance){
	printk("read-write module - close was called!\n");
	return 0;
}
```

Next we need to complete the **initialisation** function with the device proper creation. This happens in several steps.
Bear in mind that each step can go wrong. When a step fails, anything that may have been initialised before must be undone.

1. Allocate a device number with `alloc_chrdev_region()`. This is a 32-bit value where the 12 high order bits are the **major number** and the 20 low order bits are the **minor number**.
This is undone with `unregister_chrdev()`.
2. Create a class for the device with `class_create()`. Undone with `class_destroy()`.
3. Create the device file itself with `device_create()`. Undone with `device_destroy()`.
4. Initialise the device file with `cdev_init()`. the function does not return anything, so no verification needs to be done for this step.
5. Register the device to the kernel with `cdev_add()`. Undone with `cdev_del()`.

```C
#define DRIVER_NAME "dummydriver"
#define DRIVER_CLASS "MyModuleClass"

static dev_t my_device_number;
static struct class *my_class;
static struct cdev my_device;

static int __init ModuleInit(void) {
	/* Allocate device number */
	if( alloc_chrdev_region(&my_device_number, 0, 1, DRIVER_NAME) < 0) {
		printk("Device number could not be allocated\n");
		return -1;
	}
	printk("read-write: Device number major: %d, minor: %d registered\n", MAJOR(my_device_number), MINOR(my_device_number));

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

	return 0;

AddError:
	device_destroy(my_class, my_device_number);

FileError:
	class_destroy(my_class);

ClassError:
	unregister_chrdev(my_device_number, DRIVER_NAME);
	return -1;
}
```

When removing the LKM from the kernel, we need to undo everything that has been created and registered in the function above.
```C
static void __exit ModuleExit(void) {
	cdev_del(&my_device);
	device_destroy(my_class, my_device_number);
	class_destroy(my_class);
	unregister_chrdev(my_device_number, DRIVER_NAME);
}
```

Now we can do something with the device, i.e. **read and write** data. The function names have already been set in the file operations structure.
Check the documentation for [`copy_to_user`](https://archive.kernel.org/oldlinux/htmldocs/kernel-api/API---copy-to-user.html) and [`copy_from_user()`](https://manpages.org/copy_to_user/9) to understand how it works and why there are so many variables in the following snippet.
```C
#include <linux/uaccess.h>

static char buffer[255];
static int buffer_pointer;

/* Read callback */
static ssize_t driver_read(struct file *File, char *user_buffer, size_t count, loff_t *offs) {
	int to_copy, not_copied, delta;

	/* Amount of data to copy */
	to_copy = min(count, buffer_pointer);

	/* Copy data to user, returns the amount of byte that weren't copied. */
	not_copied = copy_to_user(user_buffer, buffer, to_copy);

	/* Calculate delta */
	delta = to_copy - not_copied;

	return delta;
}

/* Write callback */
static ssize_t driver_write(struct file *File, const char *user_buffer, size_t count, loff_t *offs) {
	int to_copy, not_copied, delta;

	/* Amount of data to copy */
	to_copy = min(count, sizeof(buffer));

	/* Copy data to user, returns the amount of byte that weren't copied. */
	not_copied = copy_from_user(buffer, user_buffer, to_copy);
	buffer_pointer = to_copy;

	/* Calculate delta */
	delta = to_copy - not_copied;

	return delta;
}
```

Create a Makefile like for any LKM and don't forget to export the `CROSS_COMPILE` and `ARCH` variables.
```Makefile
obj-m += read_write.o
KDIR = "/home/dlh/ECAM/Driver_C/Labs/Debian/linux-4.19"

all:
	make -C $(KDIR) M=$(PWD) modules
clean:
	make -C $(KDIR) M=$(PWD) clean
```

You can now build and insert your module into the kernel. As soon as you do, you should see a new device in `/dev`.

A full example is available [in the repository](https://github.com/parastuffs/linux-kernel-modules/blob/main/LKM/chrdev_read_write/read_write.c)



## Make the device user-accessible
As the device is created from within the kernel module that requires superuser privileges to be inserted into the kernel, its access privileges are set for the `root` user:
```sh
debian@beaglebone:~/$ ls -l /dev/dummydriver 
crw------- 1 root root 239, 0 Apr  7 10:17 /dev/dummydriver
```

The not-so-clean-but-easy way is to manually change the privileges:
```
$ sudo chmod 666 /dev/dummydriver
```

The clean way to do it is actually as easy but requires a few extra steps to add a **udev** rule.
Those rules tell the kernel what privileges to set for new devices.
They are stored in `/dev/udev/rules.d/` and their filename starts with a priority (the higher the value, the lower the priority).
Take a look at `/etc/udev/rules.d/85-gpio-noroot.rules` for example.

The fields of the rule file is set according to what the kernel knows about the device. We can find the information about our `dummydriver` device using `udevadm`:
```sh
debian@beaglebone:~/LKM$ udevadm info -a -p /sys/class/MyModuleClass/dummydriver 

Udevadm info starts with the device specified by the devpath and then
walks up the chain of parent devices. It prints for every device
found, all possible attributes in the udev rules key format.
A rule to match, can be composed by the attributes of the device
and the attributes from one single parent device.

  looking at device '/devices/virtual/MyModuleClass/dummydriver':
    KERNEL=="dummydriver"
    SUBSYSTEM=="MyModuleClass"
    DRIVER==""
```

Now to create our own rules, we create a file `99-my-rw-lkm.rules`. Appart from the priority value, the rest of the name does not matter.
Its content is must match the attributes highlighted by `udevadm` and the mode is set so that everyone can read and write on the device.
```
KERNEL=="dummydriver", SUBSYSTEM=="MyModuleClass", MODE="0666"
```

Add the file in `/etc/udev/rules.d/` and *voilà*. If we reload the kernel module, the permissions should now be set correctly for the user to have access to the device:
```sh
debian@beaglebone:~/LKM/read_write$ ls -l /dev/dummydriver 
crw-rw-rw- 1 root root 238, 0 Apr  7 10:27 /dev/dummydriver
```



## Play with the device from CLI
You can write and read data from the device using the `echo` command.
```sh
debian@beaglebone:~/LKM/read_write$ echo "Hello" > /dev/dummydriver 
debian@beaglebone:~/LKM/read_write$ head -n 1 /dev/dummydriver 
Hello
```


## Play with the device from a C program
We can access the device from a C program using the following functions.
```C
/* Open the device and store it into a file descriptor (int) */
fd = open("/dev/dummydriver", O_RDWR);
/* Write a string of characters to the device */
write(fd, stringToSend, strlen(stringToSend));
/* Read a string of characters from the device */
read(fd, receive, BUFFER_LENGTH);
/* Close the device */
close(fd);
```

Don't forget that to avoid missing libraries on the embedded platform, you can build the program in a static fashion using this kind of Makefile:
```Makefile
CC-arm = $(CROSS_COMPILE)gcc

all:
	$(CC-arm) -static rwtodev.c -o rwtodev
clean:
	rm rwtodev
```

A full example is available [in the repository](https://github.com/parastuffs/linux-kernel-modules/blob/main/LKM/chrdev_read_write/rwtodev.c)