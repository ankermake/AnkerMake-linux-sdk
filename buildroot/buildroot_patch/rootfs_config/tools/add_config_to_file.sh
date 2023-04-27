prg_name=$0
operation=$1
config_string=$2
config_file=$3

# 显示错误和使用方法并退出
usage_exit()
{
    echo $1
    echo "usage: $prg_name operation config_string config_file"
    echo "usage: operation: add/delete"
    exit 1
}

# 参数个数必须等于3
if [ "$config_file" = "" ]; then
    usage_exit "error: too few args"
fi

# 参数个数必须等于3
if [ "$4" != "" ]; then
    usage_exit "error: too many args"
fi

# 添加配置
config_add()
{
    nums=`grep -F -n "$config_string" $config_file |cut -f1 -d:`
    if [ "$nums" = "" ]; then
        echo "$config_string" >> $config_file
    fi
}

# 删除配置
config_delete()
{
    nums=`grep -F -n "$config_string" $config_file |cut -f1 -d:`
    if [ "$nums" != "" ]; then
        sed -i "${nums}d" $config_file
    fi
}

if [ "$operation" = "add" ]; then
    config_add
else
    config_delete
fi
