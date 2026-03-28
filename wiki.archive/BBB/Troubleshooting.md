## Manjaro is freezing at startup
If Manjaro is freezing on the splash screen when booting up, try to boot without splash.

## I have problems with my virtual machine
- Don't work on the live image, install it first then get to work.

## My interrupt is registered, but it does not trigger.
Try changing the GPIO used for the interrupt.

## The compilation fails
- Check the path to your compiler.
- Check if the following programs are installed: `bison`, `flex`, `libssl-dev`.
- Check that `arm-none-linux-gnueabihf-gcc` is still in your path. Remember that the `export` command is forgotten when you close or change terminal.

## Compilation tells me about `implicit declaration`
You did not include some libraries.
- `<linux/module.h>` always
- `<linux/uaccess.h>` for `copy_from_user` and `copy_to_user`
- `<linux/fs.h>` for file_operations
- `<linux/cdev.h>` for character device handling

## I'm pretty sure my LKM is fine, but I can't insert it into the kernel
- Try rebooting the board.
- Check that you compiled it for the correct kernel version. `$ uname -r` and `$ modinfo myLKM.ko` should return the same kernel version.

## Putting my process to sleep with `wait_interruptible()` crashes my kernel
- Check that the process is not holding a lock when put to sleep. You can't do that and the kernel thus crashes.

## My BBB seems to not boot on the Linux I flashed on my SD card
The BBB has internal memory on which you might already have another Linux installed.
You can force your board to boot on the SD card by pressing the `S2` button and resetting the board while keeping the button pressed.

## When using `cat` to read my eeprom device, the output never stops
`cat` is issuing `read()` calls to your device until it reads an end-of-file (EOF). This happens when the `read()` function returns `0`. While we return a value larger than zero, `cat` keeps on calling `read()`, hence the infinite output.

Instead of using cat, use `head`: `$ head -n 1 /dev/mydevice`.

## My virtual machine cannot ssh to the BBB
If scanning with `nmap` does not return any result on the `192.168.7.0/24` sub-network, make sure that you did **not** select the BeagleBoneBlack in your Virtualbox settings, under `Devices > USB`. This would make the interface unavailable from your host, hence removing the network interface that the guest is not able to manage correctly.

If that does not work, try to add the network interface in the Virtualbox settings.
1. Shut down your VM
2. Go to the settings of your VM
3. Go to `Network`
4. Go to the `Adapter 2` tab
5. Enable it with the following settings:
  - `Attached to: Bridged Adapter`
  - `Name: ` <-- select the name of the USB network interface created by the BBB

`ip a` might still not see the proper interface, but you should be able to establish an SSH connection on the address `192.168.7.2`.

## When compiling the kernel, I get `error junk at end of line first unrecognized character is #`
In the folder containing the Linux 4.19 kernel source, open the file `arch/arm/mm/proc-v7.S`.
On line 640, change the line `.section ".proc.info.init", #alloc` to `.section ".proc.info.init", "a"`.

## The kernel version on my BBB is not correct
If you see another version than expected (such as 3.8) on the BBB, you are probably booting from the emmc rather than the SD card.
To make sure you are booting from the SD card, remove the power supply (USB cable), press *and hold* the button next to the SD card slot, plug the power supply in. When the blue LEDs start flashing, release the button.
[Guide here](https://subscription.packtpub.com/book/iot-and-hardware/9781785285059/1/ch01lvl1sec14/booting-your-beaglebone-board-from-a-sd-card)

## Kernel compilation tells me `openssl/bio.h not found`
Install the package `libssl-dev`.

## I forgot the password of my virtual machine account 🙃
- Restart you VM and hold `shift` in order to display the GRUB menu. It should show you a list of OS names with an asterisk in front of the default option.
- Using the arrow keys, highlight the default option and press `e`. It should open an editor.
- Locate the line starting with `linux`. On that line, replace `ro` by `rw` and add `init=/bin/bash` (on the same line).
- Press `F10` to restart the VM and you should start on a root prompt, i.e. starting with `#`.
- Change your password using `passwd <username>` (e.g. `passwd dlh`).
- Restart the VM again and *voilà*.