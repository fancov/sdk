#!/bin/bash

source /etc/ipcc/cc.backup.conf

#前一天的日期
yesterday=`date -d "1 days ago" +%Y%m%d`
today=`date +%Y%m%d`
twodaysago=`date -d "2 days ago" +%Y%m%d`

# path of the models
ROOT_PATH=$root_path
if [ -z $ROOT_PATH ]; then
    ROOT_PATH="/usr/local/sbin/ipcc-backup"
fi
ROOT_PATH=$ROOT_PATH/"cc.backup.submod.d"

function data_backup() {
    local DIRNAME=$backup_path"/"$1
    #当前日期的前一天
    local THEDAYBEDORE=`date -d "$1 -1 days" "+%Y%m%d"`
    if [ ! -d $DIRNAME ];then
    	mkdir $DIRNAME -p
    fi

    #备份配置文件，如果日期超过当前日期的三天，就不用备份了，备份了也会被删除
    if [ $1 -ge ${twodaysago} ]; then
    {
        echo "backup configuration files..."
        #备份配置文件
        . $2/"mod_config.sh" $2/"config_list" $DIRNAME
        echo "backup configuration files end"
    }
    else
    {
        echo "Not need backup : "
    }
    fi


    for submod in $(ls $2)
    do
        if [ -z submod ]; then
            continue
        fi

        if [ "$submod" = "mod_config.sh" ]; then
            #单独执行，参数不一样
            continue;
        fi

        echo "Processing submod "$submod

        # Exec submod
        echo "Start processing submod: "$submod
        . $2/$submod $1 $DIRNAME
        echo "Finished to process submod: "$submod", Ret: "$?
    done
}


#检查 .ipcc_backup_running 是否存在
cur_dir=$(pwd)
if [ ! -f ${cur_dir}/.ipcc_backup_running ];then
{
    #创建文件
    touch ${cur_dir}/.ipcc_backup_running
}
else
{
    #已存在，不允许再执行
    echo "Already in operation."
    exit
}
fi

#读取上次备份的时间，如果获取不到，就只备份昨天的就行了
if [ -f ${cur_dir}/.ipcc_backup_date ];then
{
    last_date=`sed -n 1p ${cur_dir}/.ipcc_backup_date`
    echo $last_date
    if [[ "$last_date" =~ "^[0-9]+$" ]]; then
    {
        last_date=$yesterday
    }
    fi

    if [ -z "$last_date" ]; then
    {
        last_date=$yesterday
    }
    fi
}
else
{
    last_date=$yesterday
}
fi

last_date=`date -d "${last_date} 1 days" "+%Y%m%d"`

if [ ! -d $backup_path ];then
    mkdir $backup_path -p
fi

while [ ${last_date} -le ${today} ]
do
    echo $last_date

    #写到文件中记录当前备份的日期
    echo ${last_date} > ${cur_dir}/.ipcc_backup_date
    data_backup $last_date $ROOT_PATH
    last_date=`date -d "$last_date 1 days" "+%Y%m%d"`
done

#删除三天前的备份文件
cd ${backup_path}
for dirlist in $(ls ${backup_path})
do
    if [ -d ${dirlist} ]; then
    {
        if [ ${dirlist} -le ${twodaysago} ];then
            rm -rf $dirlist
        fi
    }
    fi
done

#判断日志的大小，如果超过1M,则将log更名为log+时间
if [ -f $log_path ]; then
{
    log_size=`du -b $log_path | awk '{print int($1/1024)}'`
    if [ ${log_size} -ge 1024 ]; then
    {
        mv $log_path $log_path`date +%Y%m%d`
        echo "create log file "$log_path`date +%Y%m%d`
    }
    fi
}
fi

#删除文件 .ipcc_backup_running 
rm ${cur_dir}/.ipcc_backup_running