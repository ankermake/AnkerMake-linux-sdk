#!/bin/sh

VID=0x1D6B
PID=0x0100

UDC=`ls /sys/class/udc/` # UDC=13500000.otg

prg_name=$0
status=$1

set_usb_descriptor() {
    # 配置设备描述符
    echo "Setting Device Descriptor..."
    echo $VID > idVendor
    echo $PID > idProduct

    # 配置字符串描述符
    echo "Setting English strings..."
    mkdir strings/0x409
    echo "01234567" > strings/0x409/serialnumber
    echo "Viveris Technologies" > strings/0x409/manufacturer
    echo "The Viveris Product !" > strings/0x409/product

    # 配置配置描述符
    echo "Creating Config..."
    mkdir configs/c.1/
    echo "120" > configs/c.1/MaxPower
    mkdir configs/c.1/strings/0x409/
    echo "Conf 1" > configs/c.1/strings/0x409/configuration
}

# 配置 mtp 设备函数
mtp_start() {
    if [ -d /sys/kernel/config/usb_gadget ]; then
        echo "Error: USB gadget already mount"
        exit 1
    fi

    echo "Creating the USB gadget"
    mount -t configfs none /sys/kernel/config

    echo "Creating gadget directory mtp_demo"
    mkdir /sys/kernel/config/usb_gadget/g1
    if [ ! -d /sys/kernel/config/usb_gadget/g1 ]; then
        echo "Error: usb_gadget busy, can't create gadget directory!"
        exit 1
    fi

    cd /sys/kernel/config/usb_gadget/g1
    set_usb_descriptor

    mkdir functions/ffs.mtp
    ln -s functions/ffs.mtp configs/c.1

    mkdir /dev/ffs-mtp
    mount -t functionfs mtp /dev/ffs-mtp

    umtprd &

    sleep 1

    echo "Binding USB Device Controller"
    echo $UDC > UDC
    echo "ok"
}

mtp_stop() {
    echo "stopping the USB gadget"

    if [ ! -d /sys/kernel/config/usb_gadget/g1 ]; then
        echo "Error: usb configfs mtp uninitialized"
        exit 1
    fi
    cd /sys/kernel/config/usb_gadget/g1

    echo "Unbinding USB Device controller..."
    echo "" > UDC
    echo "ok"

    echo "Removing mtp fs directory"
    killall umtprd
    sleep 1
    umount /dev/ffs-mtp
    rmdir /dev/ffs-mtp
    echo "ok"

    echo "Deleting mass storage gadget functionality : "
    rm configs/c.1/ffs.mtp
    rmdir functions/ffs.mtp
    echo "Delete mtp dev ok"

    echo "Cleaning up configuration..."
    rmdir configs/c.1/strings/0x409
    rmdir configs/c.1
    echo "ok"

    echo "cleaning English string..."
    rmdir strings/0x409
    echo "ok"

    echo "Removing gadget directory..."
    cd - &> /dev/null
    rmdir /sys/kernel/config/usb_gadget/g1/
    echo "ok"

    umount /sys/kernel/config
}

case "$status" in
  start)
    if [ "$#" -lt "1" ]; then
        echo "Usage1: $prg_name start"
        exit 1
    fi

    # 配置 mtp 设备函数
    mtp_start
    ;;

  stop)
    if [ "$#" -lt "1" ]; then
        echo "Usage: $prg_name stop"
        exit 1
    fi

    if [ ! -d /sys/kernel/config/usb_gadget/g1/ ]; then
        echo "Error: usb configfs mtp uninitialized"
        exit 1
    fi

    # 卸载 mtp 设备
    mtp_stop
    ;;

    *)
    echo "Usage1: $prg_name start"
    echo "Usage2: $prg_name stop"
    exit 1
esac