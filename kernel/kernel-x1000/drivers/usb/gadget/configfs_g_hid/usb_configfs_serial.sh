 mount -t configfs none /sys/kernel/config
 cd /sys/kernel/config/usb_gadget/
 mkdir g1
 cd g1/
 ls
 echo 0x18d1 > idVendor
 echo 0xd002 > idProduct
 mkdir strings/0x409
 ls strings/0x409/
 echo "0123456789" >strings/0x409/serialnumber
 echo "Foo Inc." >strings/0x409/manufacturer
 echo "Bar Gadget" >strings/0x409/product
 mkdir functions/acm.GS0
 mkdir functions/acm.GS1
 mkdir -p configs/c.1/strings/0x409
 ls configs/c.1/strings/0x409/
 echo "CDC 2*ACM" >configs/c.1/strings/0x409/configuration
 ln -s functions/acm.GS0 configs/c.1
 ln -s functions/acm.GS1 configs/c.1
 ls /sys/class/udc/
#echo "13500000.otg" >UDC
echo "dwc2" >UDC