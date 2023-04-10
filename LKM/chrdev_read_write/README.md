To test the device and LKM:
```
$ sudo chmod 666 /dev/dummydriver
$ echo "Hello world" > /dev/dummydriver
$ head -n 1 /dev/dummydriver
Hello world
```

And we can also check the behaviour in `dmesg` when calling the read and write functions.

## Resources
- [Let's code a Linux Driver - 3: Auto Device File creation & Read- Write-Callbacks](https://www.youtube.com/watch?v=tNnH-YiY_1k)