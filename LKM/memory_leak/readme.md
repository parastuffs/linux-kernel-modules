# Purpose
This kernel module is buggy.
Its purpose is to expose a buffer to the userspace through `/dev/leaky_dev`, which accepts read and writes.
Whatever a user writes into the buffer, they can read back.
Problem is, we can abuse the behaviour of the module by sending a short message and then read the content of the buffer's max size. That way, we get not only our message, but whatever bytes were written previously.

Find the bug and fix it.

# Usage
```sh
sudo insmod buggy_leaky_device.ko
sudo chmod a+rw /dev/leaky_dev
./leak_demo
```

The expected output is
```
=== Memory leak demo ===
[+] wrote: "SECRET_PASSWORD_123456"
[+] wrote: "short"

[+] Process B reading leaked memory...
[+] read (255 bytes): "shortT_PASSWORD_123456"

[!] If implemented incorrectly, you may see:
    - 'short' + leftover bytes from previous allocation
    - or even unrelated kernel heap data
```