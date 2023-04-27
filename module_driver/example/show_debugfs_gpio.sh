#!/bin/sh

function filter_gpio()
{
	line=$1
	for str in $line; do
		length=`expr length "$str"`
		line_length=`expr length "$line"`

		i=`expr match $str gpio-`
		i=`expr $i + 1`
		if [ $i = 1 ]; then
			echo  "-----------------------------------"
			echo  "$line"
			echo  "-----------------------------------"
			return
		fi

		num=`expr substr $str $i $length`
		expr $num + 100 2> /dev/null > /dev/null
		isdigit=$?

		if [ $isdigit != 0 ]; then
			echo  "-----------------------------------"
			echo  "$line"
			echo  "-----------------------------------"
		else
			i=`expr $num / 32 + 1`
			num=`expr $num % 32`
			if [ $num -lt 10 ]; then
				num=0$num
			fi
			A=ABCDEFGH
			x=`expr substr $A $i 1`
			echo -n GPIO_P$x\($num\)
			i=`expr $length + 1`
			b=`expr substr "$line" $i ${line_length}`
			echo "$b"
		fi
		return
	done
}

if [ "$1" == "" ]; then
	if [ ! -f /sys/kernel/debug/gpio ]; then
		mount -t debugfs none /sys/kernel/debug/
		if [ $? != 0 ]; then
			echo "failed to mount debugfs" 1>&2
			exit 1
		fi
	fi
	while read line; do
		filter_gpio "$line"
	done </sys/kernel/debug/gpio
	exit
fi

if [ "$1" == "-i" ]; then
	while read line; do
		filter_gpio "$line"
	done
	exit
fi

echo "unkown param $1 " 1>&2

exit 1
