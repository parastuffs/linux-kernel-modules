# Linux Kernel Modules Wiki

Through these pages, you should learn how to build your own linux kernel module (LKM) for an embedded platform such as the Beaglebone Black (BBB).

1. [Flash a Linux distribution on an SD card](https://github.com/parastuffs/linux-kernel-modules/wiki/Linux-installation).
2. [Establish a connection with the board](https://github.com/parastuffs/linux-kernel-modules/wiki/Board-communication).
3. [Prepare the cross-compilation toolchain](https://github.com/parastuffs/linux-kernel-modules/wiki/Cross-compilation-toolchain).
4. [Build your first LKM for the BBB](https://github.com/parastuffs/linux-kernel-modules/wiki/Your-first-embedded-LKM).

Once it's done, you should make your way towards controlling an LCD display from a kernel module by completing the following steps:
1. [Build a character device driver](https://github.com/parastuffs/linux-kernel-modules/wiki/Character-Device-Driver)
2. [Control GPIOs](https://github.com/parastuffs/linux-kernel-modules/wiki/Control-GPIOs)
3. [Use interrupts on GPIOs](https://github.com/parastuffs/linux-kernel-modules/wiki/GPIO-Interrupt)
4. [Write on an LCD display](https://github.com/parastuffs/linux-kernel-modules/wiki/LCD-Screen)

Here are some other modules and features you should explore:
- [Communicating with an I2C sensor](https://github.com/parastuffs/linux-kernel-modules/wiki/I2C-Temperature-Sensor)
- [Read and write data on an I2C EEPROM](https://github.com/parastuffs/linux-kernel-modules/wiki/I2C-EEPROM)
- [Timer](https://github.com/parastuffs/linux-kernel-modules/wiki/Timer-in-kernel-modules)
- ~Communicating with an SPI flash memory~ (Coming next year)
- Using `ioctl` commands instead or read/write syscalls
- Ensure concurrent access to your character device using locks
- Using a [cryptographic co-processor](https://github.com/parastuffs/linux-kernel-modules/wiki/I2C-Crypto-Core)

If you are stuck somewhere, check the [troubleshooting page](https://github.com/parastuffs/linux-kernel-modules/wiki/Troubleshooting).
You can find [more resources here](https://github.com/parastuffs/linux-kernel-modules/wiki/Resources).
You can **suggest updates and new topics** by submitting an [issue](https://github.com/parastuffs/linux-kernel-modules/issues).

## Open questions and paths to investigate
- Can we create a `screen` session that would directly connect to the LCD display and allow us to send it data?