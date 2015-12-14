#!/bin/bash


#
# System cleaner for the OK-CC system.
#

# include the configuration file
if [ -f /etc/ipcc/cc.clean.conf ]; then
. /etc/ipcc/cc.clean.conf 
fi

if [ -z $ROOT_PATH ]; then
    ROOT_PATH="/usr/local/sbin/ipcc-clean"
fi

if [ ! -f $ROOT_PATH"/cc.clean.log.sh" ]; then
    echo "LOG mod is missing."
    exit 0
fi
. $ROOT_PATH"/cc.clean.log.sh"

if [ ! -f $ROOT_PATH"/cc.clean.report.sh" ]; then
    log_write "ERROR" "Report mod is missing."
    exit 0
fi
. $ROOT_PATH"/cc.clean.report.sh"

# path of the models
root_path=$ROOT_PATH
if [ -z $root_path ]; then
    root_path="/usr/local/sbin/ipcc-clean"
fi
root_path=$root_path/"cc.clean.submod.d"

# report file path
report_path=$REPORT_PATH
if [ -z $report_path ]; then
    report_path="/var/ipcc"
fi
mkdir -p $report_path
report_path=$report_path"/report.log"
rm -rf $report_path
touch  $report_path


# read denied condig
denied=()
denied_cnt=0
if [ -f /etc/ipcc/cc.clean.denied.conf ]; then
    while read line; do
        if [ ! -z line ]; then
            denied[$denied_cnt]=$line
            denied_cnt=`expr $denied_cnt + 1`
        fi
    done < /etc/ipcc/cc.clean.denied.conf
fi

# start
log_write "INFO" "Start do cleaning. Submod Root:"$root_path", Report File:"$report_path
date=`date "+%Y-%m-%d %H:%M:%S"`
report_write "Master" "Start Date: "$date
for submod in $(ls ${root_path})
do
    if [ -z submod ]; then
        continue
    fi

    log_write "DEBUG" "Processing submod "$submod

    # check current mod is denied
    found="false"
    for (( i = 0; i < ${#denied[@]}; i++ )); do
        if [ $denied[i] == $submod ]; then
            found="true"
            break;
        fi
    done
    if [ $found == "true" ]; then
        log_write "INFO" "Submod \""$submod"\" is in denied file."
        continue
    fi

    # Exec submod
    log_write "INFO" "Start processing submod: "$submod
    . $root_path/$submod
    log_write "INFO" "Finished to process submod: "$submod", Ret: "$?
done
report_write "Master" "End: "$date
log_write "INFO" "Cleaning finished."

report_all $report_path

d=`date "+%Y%m%d%H%M%S"`
mv $report_path $report_path"-"$d
