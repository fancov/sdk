#!/bin/bash

# $1 : config_list 的路径
# $2 : 备份文件的路径


if [ -z $1 ] || [ -z $2 ]; then
    exit
fi

if [ ! -f $1 ]; then
    exit
fi

if [ ! -d $2 ]; then
    mkdir $2 -p
fi

cd $2
#读取 config_list ，进行备份，必须是绝对路径
while read config_path
do
	if [[ "$config_path" != "/"* ]]; then    
		continue
	fi

	if [ ! -f $config_path ] && [ ! -d $config_path ]; then
		continue
	fi
	
	path=${config_path#*/}
	if [ -f $config_path ]; then
	{
		#如果是文件，就把文件名去掉
		path=${path%/*}
	}
	fi
	
	if [ ! -z $path ] && [ ! -d $path ]; then
		mkdir $path -p
	fi
	
	cp $config_path $path -R
	
done < $1