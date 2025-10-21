#!/bin/sh
rm -rf /tmp/busybox
git clone https://git.busybox.net/busybox /tmp/busybox
cd /tmp/busybox
make defconfig
sed -i 's/# CONFIG_STATIC is not set/CONFIG_STATIC=y/' .config
make -j$(nproc)
make install
