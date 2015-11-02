#!/bin/bash

./make-bs.sh "REALSE"
if [ $? -ne 0 ]; then
	echo "Make bs release version."
	exit -1;
fi

./make-ctrl-panel.sh "REALSE"
if [ $? -ne 0 ]; then
	echo "Make ctrl-panel release version."
	exit -1;
fi

./make-monitor.sh "REALSE"
if [ $? -ne 0 ]; then
	echo "Make monitor release version."
	exit -1;
fi

./make-ptc.sh "REALSE"
if [ $? -ne 0 ]; then
	echo "Make ptc release version."
	exit -1;
fi

./make-pts.sh "REALSE"
if [ $? -ne 0 ]; then
	echo "Make pts release version."
	exit -1;
fi

./make-sc.sh "REALSE"
if [ $? -ne 0 ]; then
	echo "Make sc release version."
	exit -1;
fi
