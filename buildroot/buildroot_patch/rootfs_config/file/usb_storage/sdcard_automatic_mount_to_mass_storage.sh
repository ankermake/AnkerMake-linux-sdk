#!/bin/sh

case "${ACTION}" in

        add|"")
                echo "sd card mount to mass_storage! " > /dev/console
                usb_dev_mass_storage.sh start /dev/$MDEV
                ;;

        remove)
                echo "sd card umount from mass_storage! " > /dev/console
                sync
                usb_dev_mass_storage.sh stop /dev/$MDEV
                ;;
esac
