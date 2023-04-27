#!/bin/sh
echo "sd card remove!" > /dev/console

sync
umount -l /tmp/sdcard/$MDEV*
rm -rf /tmp/sdcard/$MDEV*
