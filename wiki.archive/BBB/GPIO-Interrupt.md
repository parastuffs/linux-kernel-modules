We can get information from a GPIO in two main ways: actively polling the pin for updated data or waiting for the pin to trigger an interrupt.
The latter is often the better alternative, allowing the code *not* to be actively waiting all the time.

## Wiring
In this example, we have a button with a pull-down resistor connected on pin P9_27 (`GPIO_115`) and an LED on pin P9_23 (`GPIO_49`).
We get the 5V for the button from `SYS_5V` on pin P9_7.

## Kernel Module
This time, we won't need a character device, hence the base module is the [Hello World](https://github.com/parastuffs/linux-kernel-modules/wiki/Your-first-embedded-LKM) example.

### Includes and global variables

We need a couple of new includes,
```c
#include <linux/gpio.h>
#include <linux/interrupt.h>
```

to define the GPIO pins,
```c
#define GPIO_BUTTON 115
#define GPIO_LED 49
```

and to create a global variable to hold the interrupt number.
```c
unsigned int irq_number;
```

### Initialisation

The initialisation of the module is vastly similar to the [GPIO module](https://github.com/parastuffs/linux-kernel-modules/wiki/Control-GPIOs), minus the character device creation.
The main steps are as follow:
- Request the GPIO using `gpio_request(unsigned gpio, const char *label)`
- Set the GPIO direction as an output (`gpio_direction_output(unsigned gpio, int value)`) or input (`gpio_direction_input(unsigned gpio)`)
- Export the GPIO using `gpio_export(unsigned gpio, bool direction_may_change)`

Don't forget to manage the possible failures for all these calls.

We can improve the behaviour of the button by setting a **software debounce** using `gpio_set_debounce(unsigned gpio, unsigned debounce);`, where `debounce` is a time in milli-seconds during which edge changes will be ignored after the first triggering, effectively ignoring bounces (but also ignoring anything else happening on the pin).

### Setting the interrupt
First, we need to tell the kernel that we want to map the GPIO to an interrupt. It will answer back with a unique interrupt number.
```c
irq_number = gpio_to_irq(GPIO_BUTTON);
```

Next we have to register that interrupt number with a function callback that will be executed when the interrupt is triggered.
The function return 0 upon success.
```c
request_irq(irq_number, (irq_handler_t) gpio_irq_handler, IRQF_TRIGGER_RISING, "custom_gpio_irq", NULL);
```
- `irq` is our interrupt number given by `gpio_to_irq()`
- `handler` is a function pointer to our callback
- `flags` are defined in `<linux/interrupt.h>`. For example, if we want the interrupt to trigger on a rising edge, we would set this field to `IRQF_TRIGGER_RISING`.
- `name` is a custom name for our interrupt registration.
- `dev` is used for debug purposes, we can set it to `NULL`.

### Interrupt callback
The interrupt handler given as a pointer in `request_irq()` needs to be defined.
Its signature is as follows:
```c
static irq_handler_t gpio_irq_handler(unsigned int irq, void *dev_id, struct pt_regs *regs);
```

Upon success, it returns `(irq_handler_t) IRQ_HANDLED;`

**As an exercise, write a couple of lines that reverse the LED status (on to off or off to on).** 

### Exit routine
When removing the module from the kernel, we have to free the IRQ and GPIOS.
```c
free_irq(irq_number, NULL);
gpio_unexport(GPIO_LED);
gpio_unexport(GPIO_BUTTON);
gpio_free(GPIO_LED);
gpio_free(GPIO_BUTTON);
```

## Witnessing the interrupt registration
If everything went well, we should see the interrupt in a couple of places.
```sh
debian@beaglebone:~/LKM/GPIO_irq$ cat /proc/interrupts
[...]
 70:          0  481ae000.gpio  19 Edge      custom_gpio_irq
[...]
```
where `custom_gpio_irq` is the `name` field given to `request_irq()`.

Also, we can check the two GPIOs we set, as well as the fact that there is an IRQ link to the button pin.
```sh
debian@beaglebone:~/LKM/GPIO_irq$ cat /sys/kernel/debug/gpio 
[...]
 gpio-49  (GPMC_A1             |P9_23               ) out lo
[...]
 gpio-115 (MCASP0_FSR          |P9_27               ) in  lo IRQ
[...]
```

Now, every time you press the button, the LED should turn on or off.


A full code example is available [in the repository](https://github.com/parastuffs/linux-kernel-modules/blob/main/LKM/GPIO_irq/LKM_gpio_irq.c).


## Going further and challenges
- Implement a [timer](https://github.com/parastuffs/linux-kernel-modules/wiki/Timer-in-kernel-modules) to control how long the LED will stay on after the button press.
- Wire up a second button to a GPIO. Press the first button in a known pattern (e.g. Morse code), then press the second button once. Upon pressing the second button, the LED should reproduce the same pattern as you input on the first button.


## Resources
- [linux/gpio.h on kernel 4.19.94](https://elixir.bootlin.com/linux/v4.19.94/source/include/linux/gpio.h)
- [linux/interrupt.h on kernel 4.19.94](https://elixir.bootlin.com/linux/v4.19.94/source/include/linux/interrupt.h)
- [Let's code a Linux Driver - 11: Using GPIO Interrupts in a Linux Kernel Module](Let's code a Linux Driver - 11: Using GPIO Interrupts in a Linux Kernel Module)
- [Writing a Linux Kernel Module — Part 3: Buttons and LEDs](http://derekmolloy.ie/kernel-gpio-programming-buttons-and-leds/)