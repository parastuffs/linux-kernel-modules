In this folder, you will find:
- `buggy_device.c`, a buggy implementation of a driver described bellow.
- `fixed_device.c`, a corrected version of the previous driver, correcting most of the bugs.
- `gold_device.c`, a gold standard implementation of the driver using ring buffers, structures, producer/consummer pattern and better error checking. Its downside is the relative loss in readability of the code. It can serve as a good example of a correct implementation.
- `stress_test.c`, a userspace program testing the driver, trigger bugs in the buggy implementation.

# What the driver does
The driver creates a virtual character device in `/dev`, called `buggy_dev`, `fixed_dev` or `gold_dev` depending on the version used.
It holds a buffer that can be read and written into from userspace, using a mutex to manage concurrency and making calling processes sleep when the device is busy.
`IOCTL` commands are available on magic number `k`, IDs `0` and `1` for reseting the buffer and switching between blocking or non-blocking access.

# What the stress test does
The userspace stress tests spwans threads to read and write on the device concurrently, as well as an `ioctl` thread that sends commands at random to the device.

The writer threads send a message to the device and check the return value for error, then sleep for a random amount of time between 0 and 1ms.

The reader threads try to read the content of the device buffer, detecting two kinds of errors: partial reads (corruption) and errneous returns (`EAGAIN` and `EINTR`). They then sleep as well for a random amount of time.

While using the stress test, check the terminal for messages from the userspace program, as well as the kernel log for messages from the driver.

If the driver crashes inside the kernel at some point, you can try to forcefully remove it with `sudo rmmod -f buggy_device`.


# Checklist of bugs to fix in `buggy_device.c`
- [ ] In `my_open()`, we don't check the return value of `mutex_trylock()`.
- [ ] In `my_release()`, we release the mutex using `mutex_unlock()`, without checking if the lock was even held.
- [ ] In `my_read()`, we go to sleep using `wait_event_interruptible()`, but we are holding a lock!
- [ ] We acquire the lock with an uninterruptible function, which can lead to hanging.
- [ ] We don't check the return value of the interruptible sleep `wait_event_interruptible()`. Since it's interruptible, we could get out of sleep through an interruption, which is not the expected wake up condition. A good practice is to check the return value in an `if` statement, and we exit witht the return `-EINTR` if we detected an interruption.
- [ ] In `my_read()`, we don't check the boundaries before reading the buffer, risk of overflow.
- [ ] In `my_read()`, we always return `0` instead of the amount of bytes copied to the user buffer.
- [ ] In `my_write()`, we don't use locking at all.
- [ ] In `my_write()`, we wake sleeping processes without synchronisation. Since the condition change (`data_size` update) is not protected by a lock *and* the `wait_event_interruptible()` is behind a condition check, we might miss the wake up signal, sleeping forever.
- [ ] In `my_ioctl()`, there is no protection on `data_size` update.
- [ ] In `my_ioctl()`, there is an unsafe access to userspace memory: `val = *(int *)arg`. We should instead use `copy_from_user(&val, (int __user *)arg, sizeof(int))`.
- [ ] In `my_ioctl()`, the mode change is not protected by a lock.
- [ ] In `my_ioctl()`, we return the wrong error code for the default case.