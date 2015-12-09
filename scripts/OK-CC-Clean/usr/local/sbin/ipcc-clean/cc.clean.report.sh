#!/bin/bash

#
# Result report function
#

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

# Function: report_write
# Parameters: 
#     $1   mod name
#     $2   total files
#     $3   total size
function report_write() 
{
    mod=$1
    total=$2
    size=$3
    reportfile=""

    if [ -z $mod ]; then
        mod="Master"
    fi
    
    if [ -z $REPORT_ENABLE ] || [ $REPORT_ENABLE == "off" ]; then
        log_write "DEBUG" "report is turn off"
        return;
    fi
    
    if [ -z $LOG_PATH ]; then
        reportfile="/var/ipcc/report.log"
        mkdir -p /var/ipcc/
    else
        reportfile=$LOG_PATH"/report.log"
        mkdir -p $LOG_PATH
    fi
    
    if [ $mod != "Master" ]; then
        if [ -z $total ]; then
            total=0
        fi
    fi
    
    echo $mod" "$total" "$size >> $reportfile;
}

# Function: report_all
# Parameters: 
#     $1   filename
function report_all() 
{
    if [ -z $1 ]; then
        return;
    fi

    if [ ! -f $1 ]; then
        return;
    fi

    if [ ! -z $RESULT_POST_URL ]; then
        curl -s $RESULT_POST_URL -d `cat $1`
    fi
}