We want to make programs and kernel modules that will run on the board, but build them on our local computer. To achieve this, we need a **cross-compilation toolchain**, i.e. a suite of tools that build stuff meant for another platform.
In particular, we will need:
- A gcc cross-compilation toolchain
- The Linux kernel source code corresponding to the one running on the board
- To build said kernel using the toolchain

## Set up your local environment
Install the relevant development package for your local Linux distribution: `build-essential` for Debian and `base-devel` for Manjaro (Arch).

## Install the right cross-compilation toolchain
The BBB is rocking a an [AM3358 Sitara processor from Texas Instruments](https://www.ti.com/product/AM3358), based on an Arm Cortex-A8.
As such, we can fetch the GNU toolchain directly [from Arm](https://developer.arm.com/downloads/-/arm-gnu-toolchain-downloads).
In particular, we will use the [arm-none-linux-gnueabihf version 13.2.rel1](https://developer.arm.com/-/media/Files/downloads/gnu/13.2.rel1/binrel/arm-gnu-toolchain-13.2.rel1-x86_64-arm-none-linux-gnueabihf.tar.xz?rev=adb0c0238c934aeeaa12c09609c5e6fc&hash=68DA67DE12CBAD82A0FA4B75247E866155C93053).
The [naming convention](https://wiki-archive.linaro.org/WorkingGroups/ToolChain/FAQ#What_is_the_differences_between_.2BIBw-arm-none-eabi-.2BIB0_and_.2BIBw-arm-linux-gnueabihf.2BIB0.3F_Can_I_use_.2BIBw-arm-linux-gnueabihf.2BIB0_tool_chain_in_bare-metal_environment.3F_How_do_you_know_which_toolchain_binary_to_use_where.3F) for the toolchain is `<architecture>-<vendor>-<ABI>` with:
- `<architecture>` the target architecture, `arm` in our case.
- `<vendor>` a specific vendor release, `none` in our case.
- `<ABI>` to tell if the toolchain is meant to be used in a linux environment or bare-metal (i.e. directly on the hardware), so that the compiler knows if there will be libraries alongside the program to link them or not. In our case, the programs will run on a Linux OS, so our ABI is `linux-gnueabihf`. The `hf` part at the end means **f**loats are managed in **h**ardware.


> [!TIP]
> Extract the content of the archive in a new directory and add it to your `PATH` environment variable (check how to make it permanent for your distribution, otherwise you will need to export it in every new terminal).

```
$ wget https://developer.arm.com/-/media/Files/downloads/gnu/13.2.rel1/binrel/arm-gnu-toolchain-13.2.rel1-x86_64-arm-none-linux-gnueabihf.tar.xz
$ sudo mkdir -p /opt/arm-gnu-toolchain-13.2.rel1-x86_64-arm-none-linux-gnueabihf
$ sudo chown $LOGNAME:$LOGNAME /opt/arm-gnu-toolchain-13.2.rel1-x86_64-arm-none-linux-gnueabihf
$ tar -xf arm-gnu-toolchain-13.2.rel1-x86_64-arm-none-linux-gnueabihf.tar.xz -C /opt/arm-gnu-toolchain-13.2.rel1-x86_64-arm-none-linux-gnueabihf
$ export PATH=/opt/arm-gnu-toolchain-13.2.rel1-x86_64-arm-none-linux-gnueabihf/arm-gnu-toolchain-13.2.Rel1-x86_64-arm-none-linux-gnueabihf/bin/:$PATH
$ arm-none-linux-gnueabihf-gcc -v
```
The last command should help you make sure the process worked.


## Compile the kernel locally
To cross-build a kernel module meant for another platform, we need the Linux kernel corresponding to that very platform.

First, check the exact kernel version running on your board:
```
debian@bbb:~$ uname -r
4.19.94-ti-r42
```

If you installed the [pre-built image](https://github.com/parastuffs/linux-kernel-modules/wiki/Linux-installation#installing-a-pre-built-image-on-a-microsd-card), it should be version 4.19.x.
Download the full source tree at that specific version from [Beagleboard GitHub](https://github.com/beagleboard/linux/tree/4.19) or from [TI](https://git.ti.com/cgit/ti-linux-kernel/ti-linux-kernel/?h=linux-4.19.y). **Make sure you download the same kernel version**, e.g. `4.19.94` according to the output of `uname -r` earlier.

Now to compile a Linux kernel, we need a configuration. It tells us which kernel modules should be compiled, formats to use, platform to target, etc.
Luckily, it can be retrieved from a running system.
From the board:
```
debian@bbb:~$ mkdir ~/LKM
debian@bbb:~$ cd ~/LKM
debian@bbb:~$ sudo modprobe configs
debian@bbb:~$ zcat /proc/config.gz > .config
```

Then from your computer, navigate to the folder where you extracted the Linux source tree and fetch the file (see [how to do it](https://github.com/parastuffs/linux-kernel-modules/wiki/Board-communication#send-and-receive-files)):
```
$ scp debian@192.168.7.2:/home/debian/LKM/.config .
```

From the same folder, we are now ready to compile the kernel:
```
$ make CROSS_COMPILE=arm-none-linux-gnueabihf- ARCH=arm -j 8
```
The `-j 8` argument tells `gcc` how many CPU cores it can use in parallel. Don't set more than you actually have.
It might ask you for some extra module to compile, accept the default prompt (simply press Return).



