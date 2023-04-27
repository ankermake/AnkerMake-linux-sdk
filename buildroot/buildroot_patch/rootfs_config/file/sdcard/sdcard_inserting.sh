#!/bin/sh
echo "sd card insert!" > /dev/console

if [ -e "/dev/$MDEV" ]; then
    mkdir -p /tmp/sdcard/$MDEV
    mount -rw /dev/$MDEV /tmp/sdcard/$MDEV
fi
