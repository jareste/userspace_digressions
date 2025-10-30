# userspace_digressions

For executing the project, you may be needing a linux kernel.
By default, Makefile targets your kernel. If you don't have it, try:
```bash
ls /boot/vmlinuz-$(uname -r)
```

In any case of error, would mean you don't have any kernel available.

Download it like this:
```bash
sudo apt update
sudo apt install -y linux-image-6.8.0-87-generic
```
And on Makefile change KERNEL_NAME to '6.8.0-87-generic'.

If you are seeing it long enough for it to be deprecated, google may assist you :).

After it, this should work flawless!
```bash
make && make run
```

User for the terminal is 'root'.
