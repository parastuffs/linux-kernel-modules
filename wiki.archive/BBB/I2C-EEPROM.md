The 24LC32 and 24LC256 are EEPROM from Microchip that can communicate over I²C. The former has 32kb and the latter 256kb; we will focus on the latter on this page, but every step should be compatible with the 32kb chip (save for the space available and thus the maximum memory address).

A fun fact about those EEPROM is that there already are drivers available in the kernel, but *not* for "really large devices" according to the [official documentation](https://docs.kernel.org/misc-devices/eeprom.html). The invoked reason is that devices starting at 32kb need two-byte addresses, which is not supported.

## How it works
The device is chipped in an 8-pin package:
```
      ------
A0  --|*   |-- VCC
A1  --|    |-- WP
A2  --|    |-- SCL
VSS --|____|-- SDA
```

- `A0`, `A1` and `A2` are the device address bits, tied to VSS if left unconnected.
- `WP` is a 'write protect' input, tied to VSS if left unconnected, which is the normal operation mode.

If we can address up to 256kb of data, we need 32768 addresses, i.e. 15 bits. Those are spread over two bytes, one for the high order bits of the address (in which the MSB is a 'don't care') and one the low order bits.
Each data byte can be addresses individually, but we can also write full *pages* of up to 64 bytes.
When we start sending data, we simply keep going and the device will store the data in its page buffer, automatically incrementing the 6 lower address bits for each new incoming byte. This keeps on going until the control chip sends a stop bit.
Beware though that pages have boundaries! They start at integer multiples of \[page size - 1\]. If we cross a boundary, the address increment simply wraps around and comes back to the start of the current page. Hence we can only send up to one page of data at a time.

To do a **random read** (i.e. at an arbitrary address), we need to:
1. Send a 'write command' to change the address.
2. Send a read command to get the byte at the set address.

After a random read, the internal address counter is automatically incremented.


## Accessing the EEPROM from the CLI
As an I²C device, it can be used on the same bus as other devices, as long as they don't share the same address.
These chips have the vendor fixed address bits `1010xxx`, same as the [47L series](https://i2cdevices.org/devices/47l04-47c04-47l16-47c16).
With all three address pins unconnected, we thus get the address `0b01010000`, which translates to `Ox50`.

```sh
debian@beaglebone:~$ i2cdetect -y -r 2
     0  1  2  3  4  5  6  7  8  9  a  b  c  d  e  f
00:          -- -- -- -- -- -- -- -- -- -- -- -- -- 
10: -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- 
20: -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- 
30: -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- 
40: -- -- -- -- -- -- -- -- -- -- -- -- -- -- 4e -- 
50: 50 -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- 
60: -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- 
70: -- -- -- -- -- -- -- -- 
```
In this capture, we still have a temperature sensor connected on the same bus (i.e. their `SCL` and `SDA` pins are connected together).

We can send data to the EEPROM with `i2ctransfer`:
```sh
debian@beaglebone:~$ i2ctransfer 2 w4@0x50 0x00 0x00 0x85=
WARNING! This program can confuse your I2C bus, cause data loss and worse!
I will send the following messages to device file /dev/i2c-2:
msg 0: addr 0x50, write, len 4, buf 0x00 0x00 0x85 0x85
Continue? [y/N] y
```
It's wise to *not* set the `-y` flag for such a command, so that we have a summary of what is about to be sent.

The command arguments are as follow:
- `2`: I²C bus
- `w`: 'write'
- `4`: Total length of the payload in bytes
- `@0x50`: address of the device
- `0x00 0x00`: first two bytes for the address
- `0x85=`: Send byte `0x85` until the total length is reached

To check what we just wrote, we first reset the internal address counter which as just been incremented after te write operation:
```sh
debian@beaglebone:~$ i2ctransfer 2 w2@0x50 0=
WARNING! This program can confuse your I2C bus, cause data loss and worse!
I will send the following messages to device file /dev/i2c-2:
msg 0: addr 0x50, write, len 2, buf 0x00 0x00
Continue? [y/N] y
```

Then we can dump the memory of the EEPROM:
```sh
debian@beaglebone:~$ i2cdump -y 2 0x50 c
     0  1  2  3  4  5  6  7  8  9  a  b  c  d  e  f    0123456789abcdef
00: 85 85 ff ff ff ff ff ff ff ff ff ff ff ff ff ff    ??..............
10: ff ff ff ff ff ff ff ff ff ff ff ff ff ff ff ff    ................
20: ff ff ff ff ff ff ff ff ff ff ff ff ff ff ff ff    ................
30: ff ff ff ff ff ff ff ff ff ff ff ff ff ff ff ff    ................
40: ff ff 42 ff ff ff ff ff ff ff ff ff ff ff ff ff    ..B.............
50: ff ff ff ff ff ff ff ff ff ff ff ff ff ff ff ff    ................
60: ff ff ff ff ff ff ff ff ff ff ff ff ff ff ff ff    ................
70: ff ff ff ff ff ff ff ff ff ff ff ff ff ff ff ff    ................
80: ff ff ff ff ff ff ff ff ff ff ff ff ff ff ff ff    ................
90: ff ff ff ff ff ff ff ff ff ff ff ff ff ff ff ff    ................
a0: ff ff ff ff ff ff ff ff ff ff ff ff ff ff ff ff    ................
b0: ff ff ff ff ff ff ff ff ff ff ff ff ff ff ff ff    ................
c0: ff ff ff ff ff ff ff ff ff ff ff ff ff ff ff ff    ................
d0: ff ff ff ff ff ff ff ff ff ff ff ff ff ff ff ff    ................
e0: ff ff ff ff ff ff ff ff ff ff ff ff ff ff ff ff    ................
f0: ff ff ff ff ff ff ff ff ff ff ff ff ff ff ff ff    ................
```
We only get the memory up to address `0xff` as the program is not aware that addresses span two bytes.


## I²C EEPROM in an LKM
The base idea and initialisation is the same as for the [I²C temperature sensor](https://github.com/parastuffs/linux-kernel-modules/wiki/I2C-Temperature-Sensor).
Simply don't forget to change the device address and name.

Sending data is a bit more complicated.
Since the device expects to receive the address followed by the data without a stop bit in between, we cannot send them separately. Every time the client receives a write command, it begins by expecting an address.
Here is an example of what we can do:
```c
const char buf[4] = {0x00, 0x00, 0x50, 0x51};
i2c_master_send(eeprom_i2c_client, buf, 4);
```

If we want to read back what we just wrote, we again need to reset the address counter:
```c
char recv_buf[1] = {0x00};
const char base_address[2] = {0x00, 0x00};
msleep(1); /* requires <linux/delay.h> */
i2c_master_send(eeprom_i2c_client, base_address, 2);
printk("Reading from eeprom: %d\n",i2c_master_recv(eeprom_i2c_client, recv_buf, 1));
printk("Received the byte %x\n", recv_buf[0]);
```

The sleep is an ugly way to wait for the client to write its buffer to memory so that we can read the actual memory content back.
A proper way would be to wait the `ACK` signal from the client.



A crude example is [available in the repository](https://github.com/parastuffs/linux-kernel-modules/blob/main/LKM/i2c_24LC256/i2c_24LC256.c)

## Going further and challenges
- Use the `read` and `write` callbacks to properly use the EEPROM.
- Receive the `ACK` signal from the client to know it's safe to read data again.
- Make use of the `WP` pin to protect the memory.

## Resources
- [24LC256 datasheet](https://ww1.microchip.com/downloads/en/devicedoc/21203r.pdf)
- [24LC32 datasheet](https://ww1.microchip.com/downloads/aemDocuments/documents/MPD/ProductDocuments/DataSheets/24AA32A-24LC32A-32-Kbit-I2C-Serial-EEPROM-20001713N.pdf)
- [eeprom driver documentation in Linux kernel](https://docs.kernel.org/misc-devices/eeprom.html)
- [i2c driver API](https://docs.kernel.org/driver-api/i2c.html)