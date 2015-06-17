#!/bin/bash

./make-bs.sh "DEBUG"
if [ $? -ne 0 ]; then
	echo "Make bs debug version."
	exit -1;
fi

./make-ctrl-panel.sh "DEBUG"
if [ $? -ne 0 ]; then
	echo "Make ctrl_panel debug version."
	exit -1;
fi

./make-monitor.sh "DEBUG"
if [ $? -ne 0 ]; then
	echo "Make monitor debug version."
	exit -1;
fi

./make-ptc.sh "DEBUG"
if [ $? -ne 0 ]; then
	echo "Make ptc debug version."
	exit -1;
fi

./make-pts.sh "DEBUG"
if [ $? -ne 0 ]; then
	echo "Make pts debug version."
	exit -1;
fi

./make-sc.sh "DEBUG"
if [ $? -ne 0 ]; then
	echo "Make sc debug version."
	exit -1;
fi
