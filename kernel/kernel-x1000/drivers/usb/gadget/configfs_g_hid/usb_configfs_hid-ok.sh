#!/bin/sh
mount -t configfs none /sys/kernel/config
cd /sys/kernel/config
ls
cd usb_gadget
mkdir g1
cd g1
echo "*******cd g1"
mkdir configs/c.1
mkdir functions/hid.usb0
echo 1 >functions/hid.usb0/protocol
echo 1 >functions/hid.usb0/subclass
echo 8 >functions/hid.usb0/report_length
cat /my_report_desc >functions/hid.usb0/report_desc
#echo -ne \\x05\\x01\\x09\\x06\\xa1\\x01\\x05\\x07\\x19\\xe0\\x29\\xe7\\x15\\x0
#0\\x25\\x01\\x75\\x01\\x95\\x08\\x81\\x02\\x95\\x01\\x75\\x08\\x81\\x03\\x95\\x0
#5\\x75\\x01\\x05\\x08\\x19\\x01\\x29\\x05\\x91\\x02\\x95\\x01\\x75\\x03\\x91\\x0
#3\\x95\\x06\\x75\\x08\\x15\\x00\\x25\\x65\\x05\\x07\\x19\\x00\\x29\\x65\\x81\\x0
#0\\xc0  >/my_report_desc
mkdir strings/0x409
mkdir configs/c.1/strings/0x409
echo 0xd002 > idProduct
echo 0x18d1 > idVendor
echo 0x0200 >bcdUSB #usb 2.0
echo serial > strings/0x409/serialnumber
echo manufacturer > strings/0x409/manufacturer
echo HID Gadget > strings/0x409/product
echo "Conf 1" > configs/c.1/strings/0x409/configuration
echo 120 > configs/c.1/MaxPower
ln -s functions/hid.usb0 configs/c.1
 echo dwc2 >UDC
echo jz-dwc2 >UDC
#kernel 3.10
#[  459.978717] configfs-gadget gadget: high-speed config #1: c
#[  459.984825] control error -122 req21.0a v0000 i0000 l0
#[  459.990149] dwc2 dwc2: dwc2_ep0_stall_and_restart

#ls /dev/hidg0

#kernel 4.4
# echo 13500000.otg >UDC
#[  443.408489] dwc2 13500000.otg: bound driver configfs-gadget
# [  443.607501] dwc2 13500000.otg: new device is high-speed
#[  443.719897] dwc2 13500000.otg: new device is high-speed
#[  443.776528] dwc2 13500000.otg: new address 26
#[  443.797545] configfs-gadget gadget: high-speed config #1: c

#ls /dev/hidg0


