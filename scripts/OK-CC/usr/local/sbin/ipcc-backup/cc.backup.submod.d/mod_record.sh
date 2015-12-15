#!/bin/bash

# $1 : 需要备份的日期
# $2 : 备份文件的路径

source /etc/ipcc/backup.conf

if [ "$backup_record_file" != "true" ];then
	#不需要备份
	exit
fi

if [ -z $1 ] || [ -z $2 ]; then
    exit
fi

if [ ! -d $2 ]; then
    mkdir $2 -p
fi

cd $2

function ftp_upload() {
    #ftp 上传
    ftp -n<<!
    open $remote_backup_addr $remote_backup_port
    user $ftp_user_name $ftp_password
    binary
    hash
    lcd $BACKDIR
    put $1
    close
    bye
!
}

function record_file_pack() {
    local cur_dir workdir
    workdir=$2
    cd ${workdir}
    if [ ${workdir} = "/" ]
    then
        cur_dir=""
    else
        cur_dir=$(pwd)
    fi

    for dirlist in $(ls ${cur_dir})
    do
        if [ -d ${dirlist} ];then
            cd ${dirlist}
                if [ -d $3 ];then
                    tar -Pzcvf $1 ${cur_dir}/${dirlist}/$3/*
                fi
            cd ..
        fi
    done
}

THEDAYBEDORE=`date -d "$1 -1 days" "+%Y%m%d"`
RECORD_FILE=$2"record"$1".tar.bz2"
rm -f $RECORD_FILE
record_file_pack $RECORD_FILE $record_file_directory $THEDAYBEDORE

if [ -f ${RECORD_FILE} ]; then
{
    ftp_upload "record"$1".tar.bz2"
    rm $RECORD_FILE -rf
}
fi