#!/bin/sh

prg_name=$0
partition_name=$1
dev_path=$1
mount_path=$2

# 显示错误和使用方法并退出
usage_exit()
{
	echo $1
	echo "usage: $prg_name partition_name/dev_path mount_path" 1>&2
	exit 1
}

# 显示错误并退出
error_exit()
{
	echo $1 1>&2
	exit 1
}

mtd_name_to_num()
{
	local partition_name=$1
	local mtd_info=`cat /proc/mtd | grep \""$partition_name"\"`
	if [ "$mtd_info" = "" ]; then
		echo "mtd partition not find: $partition_name" 1>&2
		return 1
	fi

	local mtd_n=${mtd_info%%:*}
	if [ "$mtd_info" = "$mtd_n" ]; then
		echo "mtd_info not ok: $mtd_info" 1>&2
		return 1
	fi

	local n=${mtd_n#mtd}
	if [ "$n" = "$mtd_n" ]; then
		echo "mtd_info not ok2: $mtd_n" 1>&2
		return 1
	fi

	echo $n
}

# 等待文件出现
wait_file()
{
	local file=$1
	local times=100
	local delay=0.01

	while true;
	do
		if [ -e $file ]; then
			return 0
		fi

		if [ $times = 0 ]; then
			echo "failed to wait: $file" 1>&2
			return 1;
		fi

		let times=times-1
		sleep $delay
	done
}

# 参数个数必须等于2
if [ "$mount_path" = "" ]; then
	usage_exit "error: too few args"
fi

if [ "$3" != "" ]; then
	 usage_exit "error: too many args"
fi

# 优先匹配分区名
num=`mtd_name_to_num $partition_name`

if [ "$num" = "" ]; then
	# 获得最标准的绝对路径，如果传进来的是一个路径
	dev_path=`readlink -f $dev_path`

	# 匹配 /dev/mtdN 或者 /dev/mtdblockN 这两种情况
	# 不支持 /dev/mtdNro /dev/mtdblockNro
	if [ "${dev_path:0:5}" = "/dev/" ]; then
		tmp0=${dev_path:5}
		tmp1=${tmp0#mtd}
		let tmp11=tmp1+0

		tmp2=${tmp0#mtdblock}
		let tmp22=tmp2+0

		if [ "$tmp1" = "$tmp11" ]; then
			num=$tmp1
		fi

		if [ "$tmp2" = "$tmp22" ]; then
			num=$tmp2
		fi

		if [ "$num" = "" ]; then
			usage_exit "error: only support mtd device"
		fi
	fi

	if [ "$num" = "" ]; then
		error_exit "error: can not find the mtd partition $partition_name"
	fi
fi

dev=/dev/mtdblock$num
wait_file $dev
if [ "$?" != "0" ] ; then
	error_exit "error: why $dev is not exist ?"
fi


result=`mount | grep -e $dev -e $mount_path`
if [ "$result" != "" ]; then
	error_exit "resource busy: $result"
fi

result=`which flash_erase`
if [ "$result" = "" ]; then
	error_exit "error: flash_erase not found!"
fi

mkdir -p $mount_path
if [ ! -e $mount_path ]; then
	error_exit "can't create mount path: $mount_path"
fi

mount -t jffs2 $dev $mount_path
if [ $? != 0 ]; then
	echo "mount jffs2 error, try mount again" 1>&2
	# 擦除分区,重试一次
	flash_erase -j /dev/mtd$num 0 0 > /dev/null
	mount -t jffs2 $dev $mount_path
	if [ $? != 0 ]; then
		error_exit "mount jffs2 error again"
	fi
fi