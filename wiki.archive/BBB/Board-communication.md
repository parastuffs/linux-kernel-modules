We could use the HDMI port to connect a screen to the board and manage it like a regular computer.
However this method would defeat the purpose of having an *embedded* platform that could end up in an environment that does not require or even allow access to a screen.

This page will demonstrate a few methods to access and manage the board without the need to connect a screen or any control peripherals.

## UART debug connection
This method does not require to have a working OS on the board, it's thus useful to debug things that may prevent one to correctly boot.

The 6 pins next to one of the GPIO header, with `J1` indicated on the silkscreen, is a UART debug port. The dot on the silkscreen indicates the first pin and three pins actually interest us: Ground, RX and TX.
Connect them correctly to a USB-to-UART converter:

| Pin      | On BBB | On converter | Colour (if using a [FTDI TTL-232R](https://www.ftdichip.com/Support/Documents/DataSheets/Cables/DS_TTL-232R_CABLES.pdf) cable |
| -------| -----------  | -----------  | ----------- |
| 1      | Ground       | Ground       | Black       |
| 4      | RX           | TX           | Orange      |
| 5      | TX           | RX           | Yellow      |

Once it's done, power the board.
You should see a new device appear in your `/dev` directory, such as `/dev/ttyUSB0`.
Note that if you're using a virtual machine, you might need to activate the USB to UART converter (see [this screenshot](https://raw.githubusercontent.com/wiki/parastuffs/linux-kernel-modules/images/VM-FTDI.png) if using an FTDI cable, if you are using a CH340 converter check `QinHeng Electronics USB Serial [0264]`).

We can now connect to that device, for example using `minicom` (you may need to install it first).
According to [eLinux wiki](https://elinux.org/Beagleboard:BeagleBone_Black_Serial), the baudrate is 115200 and all other communication parameters are the defaults.
We can establish a serial connection with the board like so:
```
$ minicom --device /dev/ttyUSB0
```
Press the reset button next to the USB port on the board to witness the full booting procedure.
You should then be welcomed with a login prompt with  default login:password `debian:temppwd` if you installed the [pre-built image](https://raw.githubusercontent.com/wiki/parastuffs/linux-kernel-modules/images/VM-FTDI.png).

Note: The UART logic level of the board is 5V. If you are using a CH340G USB-to-UART controller, make sure you set the logic level jumper correctly.


## SSH connection
If you installed a pre-built Linux image or configured one correctly, the OS should create a network interface using the USB connection between the board and your computer. If you are working within VirtualBox, don't forget to activate the USB interface like in [this screenshot](https://raw.githubusercontent.com/wiki/parastuffs/linux-kernel-modules/images/VM-FTDI.png).

There are a few methods to discover the IP of the board.

1. From a debug console running through UART, we can see the IP assigned to the USB network connection. In a terminal:
```
$ ip a
[...]
5: usb0: <BROADCAST,MULTICAST,UP,LOWER_UP> mtu 1500 qdisc pfifo_fast state UP group default qlen 1000
    link/ether 84:eb:18:97:ef:16 brd ff:ff:ff:ff:ff:ff
    inet 192.168.7.2/24 brd 192.168.7.255 scope global usb0
       valid_lft forever preferred_lft forever
```

2. From your local computer, check the sub-network of the newly created interface and scan it using `nmap`.
```
$ ip a
[...]
5: enp5s0f3u2u3u3: <BROADCAST,MULTICAST,UP,LOWER_UP> mtu 1500 qdisc fq_codel state UNKNOWN group default qlen 1000
    link/ether 84:eb:18:97:ef:17 brd ff:ff:ff:ff:ff:ff
    inet 192.168.7.1/24 brd 192.168.7.255 scope global dynamic noprefixroute enp5s0f3u2u3u3
       valid_lft 106sec preferred_lft 106sec

$ nmap -sn 192.168.7.0/24
Starting Nmap 7.93 ( https://nmap.org ) at 2023-04-04 12:34 CEST
Nmap scan report for 192.168.7.1
Host is up (0.00012s latency).
Nmap scan report for 192.168.7.2
Host is up (0.00064s latency).

$ nmap -sV -p 22 192.168.7.2
Starting Nmap 7.93 ( https://nmap.org ) at 2023-04-04 12:36 CEST
Nmap scan report for 192.168.7.2
Host is up (0.00046s latency).

PORT   STATE SERVICE VERSION
22/tcp open  ssh     OpenSSH 7.9p1 Debian 10+deb10u2 (protocol 2.0)
Service Info: OS: Linux; CPE: cpe:/o:linux:linux_kernel
```

We know the IP of the board is `192.168.7.2` in this instance, and we confirmed that port 22 is open and running an SSH service.
We can thus connect to the board like so:
```
$ ssh debian@192.168.7.2
```

We can leave the session by typing `exit` in the terminal.




## Send and receive files
To exchange file with the board, we can use an SSH connection through `scp`.

On your local computer, create a test file and then send it to the board:
```
$ echo "Hello world!" > hello.txt
$ scp hello.txt debian@192.168.7.2:/home/debian/
```

Check on the board that the file was correctly received!