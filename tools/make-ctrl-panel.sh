#!/bin/bash

if [ $# -lt 1 ];then
	echo "Please input the paramaters"
	exit -1
fi

CURRENT_PATH=`pwd`

MAKE_PATH=../project/ctrl_panel

if [ -n $2 ]; then
	HOST_ARCH=$2
else
	HOST_ARCH=""
fi


case $1 in
	DEBUG)
		APPNAME=ctrl_paneld
		MAKEFILE_NAME="Makefile"
	;;
	RELEASE)
		APPNAME=ctrl_panel
		MAKEFILE_NAME="Makefile.Realse"
	;;
	*)
		echo "Cannot determine the Makefile. Please chech the paramaters."
		exit -1
	;;
esac

if [ -d $MAKE_PATH ];then
	cd $MAKE_PATH
else
	echo "Dir ${MAKE_PATH} not exist."
	exit -1
fi

if [ ! -f ctrl_panel.dep ]; then
	touch ctrl_panel.dep
fi

make dep -f $MAKEFILE_NAME
if [ $? -ne 0 ]; then
	echo "Make dep FAIL. App:"${APPNAME}
	exit 255
fi

make clean -f $MAKEFILE_NAME
if [ $? -ne 0 ]; then
	echo "Make clean FAIL. App:"${APPNAME}
	exit 255
fi

make all -f $MAKEFILE_NAME
if [ $? -ne 0 ]; then
	echo "Make all FAIL. App:"${APPNAME}
	exit 255
fi

if [ ! -d ${CURRENT_PATH}/apps ];then
	mkdir -p ${CURRENT_PATH}/apps
fi 

cp $APPNAME ${CURRENT_PATH}/apps/

cd $CURRENT_PATH

exit 0
