#!/bin/bash

#
# Script for cleaning the xferlog log
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

# Define the mod name
NAME="xferlog"

# Define the max time the xferlog log can be save(in day)
MAX_TIME=30

# Define xferlog log root path
FILE_ROOT_PATH="/var/log"

# Define the xferlog log file prefix
# We cannot delete the current log (xferlog)
FILE_PREFIX="xferlog-"
FILE_SUBFIX=""

total_size=0
total_file=0

log_write "INFO" "Processing xferlog log file."

files=`find $FILE_ROOT_PATH -mtime +$MAX_TIME -name "$FILE_PREFIX*$FILE_SUBFIX"`
if [ ${#files[@]} -gt 0 ]; then
    for file in $files
    do
        if [ -f $file ]; then
            continue
        fi

        file_size=`ll file | awk '{print $5}'`
        total_size=`expr $total_size + $file_size`
        total_file=`expr $total_file + 1`
        rm -rf $file
        log_write "DEBUG" "Delete file "$file", size"$file_size
    done
fi

report_write $NAME $total_file $total_size
log_write "INFO" "Processed xferlog log file. Total:"$total_file", Size:"$total_size
