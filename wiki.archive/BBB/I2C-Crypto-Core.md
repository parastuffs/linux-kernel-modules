The [ATECC508A](https://docs.rs-online.com/eba3/0900766b8166f69b.pdf) is a cryptographic co-processor.
Its basic usage is to store cryptographic keys and to offer some security schemes using them.
In particular, the chip is able to use a SHA-256 hash and perform PKI algorithms (ECDSA and ECDH).

It communicates using the I2C protocol, also needing pull-up resistors on SDA and SCL. You should thus be able to use it with both the [temperature sensor](https://github.com/parastuffs/linux-kernel-modules/wiki/I2C-Temperature-Sensor) and [eeprom](https://github.com/parastuffs/linux-kernel-modules/wiki/I2C-EEPROM) connected on the bus.

The main device features are already implemented in a driver available in the kernel tree: [`atmel-ecc`](https://git.kernel.org/pub/scm/linux/kernel/git/stable/linux.git/tree/drivers/crypto/atmel-ecc.c?h=v4.19.94). Pay attention to the version of the kernel you're exploring, as the interfaces changed along the years.

Start by reading the documentation to understand how the data is sent to the device (sections 6.2 and 6.5), then read about the I/O groups (section 9.1).
Then explore the `atmel-ecc` driver to see how it has been implemented and use it to create a basic character device that can **return a (true) random number** from the co-processor.



## Resources
- Chip [documentation](https://docs.rs-online.com/eba3/0900766b8166f69b.pdf)
- I2C kernel [API](https://docs.kernel.org/driver-api/i2c.html)
- Atmel driver [for kernel 4.19.94](https://git.kernel.org/pub/scm/linux/kernel/git/stable/linux.git/tree/drivers/crypto/atmel-ecc.c?h=v4.19.94)
- Online [CRC calculator](https://crccalc.com/)