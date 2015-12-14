#!/bin/bash

#
# Script for cleaning the syslog
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
SYSLOG_NAME="syslog"

# Define the max time the syslog can be save(in day)
SYSLOG_MAX_TIME=30

# Define syslog root path
SYSLOG_ROOT_PATH="/var/log"

# Define the syslog file prefix
# We cannot delete the current log (messages)
SYSLOG_FILE_PREFIX="messages-"
SYSLOG_FILE_SUBFIX=""

total_size=0
total_file=0

log_write "INFO" "Processing syslog file."

files=`find $SYSLOG_ROOT_PATH -mtime +$SYSLOG_MAX_TIME -name "$SYSLOG_FILE_PREFIX*$SYSLOG_FILE_SUBFIX"`
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

report_write $SYSLOG_NAME $total_file $total_size
log_write "INFO" "Processed syslog file. Total:"$total_file", Size:"$total_size
