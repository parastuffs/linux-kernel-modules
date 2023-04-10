This module interacts with GPIOs on the board.

## Connect an LED to a GPIO
Connect an LED to the pin 23 on header `P9`, which corresponds to `GPIO_49`.


## Control an LED from the CLI
GPIOs are readily available from the CLI.
See what happens with the following commands:

```
debian@beaglebone:~$ cd /sys/class/gpio/
debian@beaglebone:/sys/class/gpio$ sudo su
root@beaglebone:/sys/class/gpio# echo 49 > export
root@beaglebone:/sys/class/gpio# cd gpio49
root@beaglebone:/sys/class/gpio/gpio49# ls
active_low  device  direction  edge  label  power  subsystem  uevent  value
root@beaglebone:/sys/class/gpio/gpio49# echo out > direction 
root@beaglebone:/sys/class/gpio/gpio49# echo 1 > value 
root@beaglebone:/sys/class/gpio/gpio49# echo 0 > value
root@beaglebone:/sys/class/gpio/gpio49# cd ..
root@beaglebone:/sys/class/gpio# echo 49 > unexport
```

We just controlled an LED by writing some values in registers.

## Control the GPIO using a character driver




If the insertion went well, we should see our GPIO registration in `/sys/kernel/debug/gpio`:
```
debian@beaglebone:~/LKM/GPIO$ cat /sys/kernel/debug/gpio
[...]
 gpio-49  (GPMC_A1             |bbb-gpio-49         ) out lo
[...]
```

If you don't see it marked with the correct direction and state, something went wrong.

We can now control it from CLI again:
```
debian@beaglebone:~/LKM/GPIO$ sudo insmod LKM_gpio.ko 
debian@beaglebone:~/LKM/GPIO$ sudo chmod 666 /dev/LKM_gpio 
debian@beaglebone:~/LKM/GPIO$ echo 1 > /dev/LKM_gpio
debian@beaglebone:~/LKM/GPIO$ echo 0 > /dev/LKM_gpio
```