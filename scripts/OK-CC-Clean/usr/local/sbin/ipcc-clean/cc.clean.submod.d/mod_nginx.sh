#!/bin/bash

#
# Rotate function for nginx
#

#
# 1. rename the current file 
# 2. check if the old file is timeout
#


# Define the nginx pid file 
NGINX_PID="/usr/local/nginx/logs/nginx.pid"

# Define the log file path
NGINX_LOG_PATH="/usr/local/nginx/logs/"

if [ -f $NGINX_PID ]; then
    return;
fi

if [ -d $NGINX_LOG_PATH ]; then
    return;
fi

if [ ! -f $NGINX_LOG_PATH"access.log" ]; then
    return;
fi

if [ ! -f $NGINX_LOG_PATH"error.log" ]; then
    return;
fi

date=`date "+%Y%m%d%H%M%S"`
mv $NGINX_LOG_PATH"access.log"   $NGINX_LOG_PATH"access-"$date".log"
mv $NGINX_LOG_PATH"error.log"    $NGINX_LOG_PATH"access-"$date".log"

#send signal, make sure the nginx reopen the log
kill -USR1 `cat ${NGINX_PID}`


# Start check the file time
# include the configuration file
if [ -f /etc/ipcc/cc.clean.conf ]; then
. /etc/ipcc/cc.clean.conf 
fi

if [ -z $ROOT_PATH ]; then
    ROOT_PATH="/usr/local/sbin/ipcc-clean"
fi

if [ ! -f $ROOT_PATH"/cc.clean.log.sh" ]; then
    echo "LOG mod is missing."
    return;
fi
. $ROOT_PATH"/cc.clean.log.sh"

if [ ! -f $ROOT_PATH"/cc.clean.report.sh" ]; then
    log_write "ERROR" "Report mod is missing."
    return;
fi
. $ROOT_PATH"/cc.clean.report.sh"

# Define the mod name
NAME="nginx"

# Define the max time the secure log can be save(in day)
MAX_TIME=30

# Define secure log root path
FILE_ROOT_PATH=$NGINX_LOG_PATH

# Define the secure log file prefix
# We cannot delete the current log (secure)
FILE_PREFIX="access-"
FILE_SUBFIX=".log"

total_size=0
total_file=0

log_write "INFO" "Processing nginx log file."

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
log_write "INFO" "Processed secure log file. Total:"$total_file", Size:"$total_size


# Define the secure log file prefix
# We cannot delete the current log (secure)
FILE_PREFIX="error-"
FILE_SUBFIX=".log"

total_size=0
total_file=0

log_write "INFO" "Processing nginx log file."

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
log_write "INFO" "Processed secure log file. Total:"$total_file", Size:"$total_size
