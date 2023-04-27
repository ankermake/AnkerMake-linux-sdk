#!/bin/sh

source /etc/usb_storage_conf

case "$1" in
  start)

    block_size=$((1024*1024))
    bclok_count=$(($image_size*1024*1024/$block_size))

    if [ ! -f $image_file_name ];then

        dd if=/dev/zero of=$image_file_name bs=$block_size count=$bclok_count

        mkfs.fat $image_file_name
        if [ $? != 0 ];then
            echo "mkfs.fat ${image_file_name} failed ,please sure tool exists"
            return 1
        fi
    else
        echo "The same name image exists..."
    fi

    usb_dev_mass_storage.sh start $image_file_name
    if [ $? != 0 ];then
        return 1
    fi

    ;;
  stop)
    ;;
  restart|reload)
    ;;
  *)
    echo  "Usage: $0 {start|stop|restart|reload}"
    exit 1
esac

exit $?