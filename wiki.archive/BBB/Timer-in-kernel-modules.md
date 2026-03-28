Timers are quite straightforward to implement.

They need a couple of **libraries** to include,
```c
#include <linux/jiffies.h>
#include <linux/timer.h>
```

a **global `timer_list` structure** that holds the timer,
```c
static struct timer_list my_timer;
```

and a timer **callback**.
```c
void timer_callback(struct timer_list * data);
```

The timer should be **initialised** in the `__init` function of the module through this function:
```c
timer_setup(timer, callback, flags);
```
In our example, if we don't need any particular flag, it would look like `timer_setup(&my_timer, timer_callback, 0);`.

When removing the module from the kernel, we also need to **delete** the created timer:
```c
del_timer(&my_timer);
```


The duration of the timer proper is set through the function `mod_timer(struct timer_list *timer, unsigned long expires)`.
The `expires` argument is in *jiffies*, i.e. the number of ticks since the system booted. `<linux/jiffies.h>` defines a bunch of handy functions to convert usual time scales into jiffies:
- `msecs_to_jiffies(const unsigned int m)`
- `usecs_to_jiffies(const unsigned int u)`
- `nsecs_to_jiffies(u64 n)` Note that the [documentation](https://elixir.bootlin.com/linux/v4.19.94/source/kernel/time/time.c#L816) states that this function "*is designed for scheduler, not for use in device drivers to calculate timeout value.*"

`jiffies` itself is a global variable that holds the current tick count.
For example, `jiffies+usecs_to_jiffies(1)` would be 1 µs from now.

Once the `mod_timer()` function has been called, the timer starts and calls its callback function at the end of its period.


## Resources
- [`<linux/timer.h`](https://elixir.bootlin.com/linux/v4.19.94/source/include/linux/timer.h)
- [`<linux/jiffies.h`](https://elixir.bootlin.com/linux/v4.19.94/source/include/linux/jiffies.h)
- [Let's code a Linux Driver - 8: Timer in a Linux Kernel Module](https://www.youtube.com/watch?v=3JTfweWGjBg)