#!/bin/bash

RET_VALUE=`./make-bs.sh "REALSE"`
if [ $RET_VALUE -ne 0 ]; then
	echo "Make bs realse version."
	exit -1;
fi

RET_VALUE=`./make-ctrl-plan.sh "REALSE"`
if [ $RET_VALUE -ne 0 ]; then
	echo "Make bs realse version."
	exit -1;
fi

RET_VALUE=`./make-monitor.sh "REALSE"`
if [ $RET_VALUE -ne 0 ]; then
	echo "Make monitor realse version."
	exit -1;
fi

RET_VALUE=`./make-ptc.txt "REALSE"`
if [ $RET_VALUE -ne 0 ]; then
	echo "Make ptc realse version."
	exit -1;
fi

RET_VALUE=`./make-pts.txt "REALSE"`
if [ $RET_VALUE -ne 0 ]; then
	echo "Make pts realse version."
	exit -1;
fi

RET_VALUE=`./make-sc.sh "REALSE"`
if [ $RET_VALUE -ne 0 ]; then
	echo "Make sc realse version."
	exit -1;
fi