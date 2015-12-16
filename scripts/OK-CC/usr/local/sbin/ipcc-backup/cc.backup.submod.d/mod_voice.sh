#!/bin/bash

# $1 : 需要备份的日期
# $2 : 备份文件的路径

source /etc/ipcc/cc.backup.conf

if [ "$backup_voice_file" != "true" ];then
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
    echo "upload file :"$1
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

function voice_file_pack() {
	local cur_dir workdir file_time
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
            	voice_file_pack $1 $2/${dirlist} $3
            cd ..
        else
        	#判断文件的时间，如果是昨天的，则打包
        	file_time=`ls --full-time "$dirlist" | cut -d" " -f6-7 | cut -c1,2,3,4,6,7,9,10`
        	if [ "$file_time" == "$3" ];then
        	{
        		tar -Pzcvf $1 ${cur_dir}/${dirlist}
        	}
        	fi
        fi
    done
}

THEDAYBEDORE=`date -d "$1 -1 days" "+%Y%m%d"`
VOICE_FILE=$2"voice"$1".tar.bz2"
rm -f $VOICE_FILE
voice_file_pack $VOICE_FILE $voice_file_directory $THEDAYBEDORE

if [ -f ${VOICE_FILE} ]; then
{
    ftp_upload "voice"$1".tar.bz2"
    rm $VOICE_FILE -rf
}
fi