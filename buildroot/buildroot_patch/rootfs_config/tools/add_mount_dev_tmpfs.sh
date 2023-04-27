#!/bin/sh

inittab=$1

result=`grep "::sysinit:/bin/mount -t tmpfs tmpfs /dev" $inittab`
if [ "$result" = "" ]; then
    n=`sed -n -e '/Startup the system/=' $inittab`
    sed -i "${n}a\::sysinit:/bin/mknod /dev/null c 1 3" $inittab
    sed -i "${n}a\::sysinit:/bin/mount -t tmpfs tmpfs /dev" $inittab
fi
