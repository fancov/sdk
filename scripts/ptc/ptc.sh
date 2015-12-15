#!/bin/sh

#ptc的升级脚本
#declare -i g_processID=0

help(){
    echo "Usage: $0 <process_name>"
    exit 0
}

# 参数范围检查
if [ "$#" != 1 ]; 
then   
    help
fi

#杀死之前的进程
#ps -ef | grep ptcd | grep -v grep | awk '{print $2}' | while read pid
#	 do
#		echo $pid
#		kill -9 $pid
#     done

exec ./ptcd &

#检查进程实例是否已经存在
# while [ 1 ]; do

    DTTERM=`pgrep ptcd | wc -l`
	echo "$DTTERM"
    if [ "$DTTERM" == 1 ]
    then  
        echo "process exit and date is: `date`" 
#正确输入信息到日志文件
    else
        echo "restart process: $1 and date is: `date`"
		mv ptcd_old ptcd
        exec ./${1} &
    fi
#监控时间间隔
    # sleep 1                        
# done