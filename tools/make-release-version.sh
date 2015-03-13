#!/bin/bash

./make-bs.sh "REALSE"
if [ $? -ne 0 ]; then
	echo "Make bs realse version."
	exit -1;
fi

./make-ctrl-panel.sh "REALSE"
if [ $? -ne 0 ]; then
	echo "Make ctrl-panel realse version."
	exit -1;
fi

./make-monitor.sh "REALSE"
if [ $? -ne 0 ]; then
	echo "Make monitor realse version."
	exit -1;
fi

./make-ptc.sh "REALSE"
if [ $? -ne 0 ]; then
	echo "Make ptc realse version."
	exit -1;
fi

./make-pts.sh "REALSE"
if [ $? -ne 0 ]; then
	echo "Make pts realse version."
	exit -1;
fi

./make-sc.sh "REALSE"
if [ $? -ne 0 ]; then
	echo "Make sc realse version."
	exit -1;
fi
