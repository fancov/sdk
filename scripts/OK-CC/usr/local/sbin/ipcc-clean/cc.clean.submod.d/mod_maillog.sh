#!/bin/bash

#
# Script for cleaning the maillog
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
MAILLOG_NAME="maillog"

# Define the max time the maillog can be save(in day)
MAILLOG_MAX_TIME=30

# Define maillog root path
MAILLOG_ROOT_PATH="/var/log"

# Define the maillog file prefix
# We cannot delete the current log (mail)
MAILLOG_FILE_PREFIX="maillog-"
MAILLOG_FILE_SUBFIX=""

total_size=0
total_file=0

log_write "INFO" "Processing maillog file."

files=`find $MAILLOG_ROOT_PATH -mtime +$MAILLOG_MAX_TIME -name "$MAILLOG_FILE_PREFIX*$MAILLOG_FILE_SUBFIX"`
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

report_write $MAILLOG_NAME $total_file $total_size
log_write "INFO" "Processed maillog file. Total:"$total_file", Size:"$total_size
