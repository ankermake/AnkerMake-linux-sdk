#!/bin/sh

prg_name=$0
env_name=$1
env_value=$2
profile_file=$3

# 显示错误和使用方法并退出
usage_exit()
{
	echo $1
	echo "usage: $prg_name env_name env_value profile_file"
	exit 1
}

# 参数个数必须等于3
if [ "$profile_file" = "" ]; then
	usage_exit "error: too few args"
fi

# 参数个数必须等于3
if [ "$4" != "" ]; then
	 usage_exit "error: too many args"
fi

# 如果文件不存在，那么创建
touch $profile_file
chmod +x $profile_file

# 列出export env_name 所在的行号
nums=`grep -n "^export $env_name=" $profile_file | cut -d ":" -f 1`

# 如果env_value的值为空，那么删除env_name所在的所有的行
if [ "$env_value" = "" ]; then
    for n in $nums
    do
        sed -i "${n}d" $profile_file
    done
    exit
fi

# 替换env_name所在的所有的行 ---> export env_name=env_value
# 如果不存在则追加新的一行
if [ "$nums" = "" ]; then

    echo "export $env_name=\"$env_value\"" >> $profile_file

else

for n in $nums
do
    sed -i "${n}c export $env_name=\"$env_value\"" $profile_file
done

fi
