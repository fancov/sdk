#!/bin/bash

#
# Log write function
#

if [ -f /etc/ipcc/cc.clean.conf ]; then
. /etc/ipcc/cc.clean.conf 
fi

# Name:     log_rename
# Function: Check is the log file is large then 1M, if so 
#           rename the file with name-yyyymmddhhmmss
# Parameters: 
#          $1 Current file
function log_check_rename()
{
    file=$1

    if [ -z $file ]; then
        return 0
    fi

    if [ ! -f $file ]; then
        return 0
    fi

    size=`du -b $file | awk '{print $1}'`
    if [ $size -ge 1048576 ]; then
        mv $file $file-`date "+%Y%m%d%H%M%S"`
    fi
}

# Function: log_write
# Parameters: 
#     $1   LEVEL: Valid values: ERROR|WARNING|INFO|DEBUG
#     $2   MSG  : Msg Body
function log_write() 
{
    level=$1
    msg=$2
    date=`date "+%Y-%m-%d %H:%M:%S"`
    logfile=""

    echo $date" "$level" "$msg

    if [ -z $LOG_ENABLE ]; then
        return 0;
    fi
    
    if [ $LOG_ENABLE == "off" ]; then
        return 0;
    fi
    
    if [ ! -z $LOG_LEVEL ]; then
        if [ $LOG_LEVEL == "INFO" ]; then
            if [ $level == "DEBUG" ]; then
                return 0;
            fi
        elif [ $LOG_LEVEL == "WARNING" ]; then
            if [ $level == "DEBUG" ] || [ $level == "INFO" ]; then
                return 0;
            fi
        elif [ $LOG_LEVEL == "ERROR" ]; then
            if [ $level == "DEBUG" ] || [ $level == "INFO" ] || [ $level == "WARNING" ]; then
                return 0;
            fi
        fi
    fi
    
    if [ -z $LOG_PATH ]; then
        logfile="/var/ipcc/clean.log"
        mkdir -p "/var/ipcc"
    else
        logfile=$LOG_PATH"/clean.log"
        mkdir -p $LOG_PATH
    fi
    
    echo $date" ["$level"] "$msg >> $logfile

    log_check_rename $logfile
}
