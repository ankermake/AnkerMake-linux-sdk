#!/bin/bash
key_word="SSV|RSV"

ssv_phy=""
if [[ ${1} =~ "wlan" ]]; then
    wlan_dirs=/sys/class/net/${1}/device/ieee80211/
    if [ ! -e ${wlan_dirs} ]; then
        echo "Could not find the ${1}."
        exit 1;
    fi
    # shift wlanX
    shift 1
    ssv_phy=`ls ${wlan_dirs}`
else
    phy_dirs="/sys/class/ieee80211/*"

    for phy_dir in $phy_dirs; do
        if [ ! -d ${phy_dir}/device/driver ]; then
            exit 1;
        fi
        drv_name=`ls ${phy_dir}/device/driver | grep -E $key_word`
        if [ -n ${drv_name} ]; then
            ssv_phy=`basename $phy_dir`;
            break;
        fi
    done
fi


# excute CLI
if [ -n ${ssv_phy} ]; then
    SSV_CMD_FILE=/proc/ssv/${ssv_phy}/ssv_dbg_fs
    if [ -f $SSV_CMD_FILE ]; then
        cat $SSV_CMD_FILE
    fi
else
    echo "./show_dbg_log.sh [wlanX]"
fi

