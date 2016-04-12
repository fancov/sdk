#!/bin/bash

SNAPSHOT_ROOT_PATH="/var/ipcc-snapshot-history"
ROOT_PATH="/usr/local/sbin/ipcc-snapshot"
SUBMOD_PATH=$ROOT_PATH"/cc.snapshot.submod.d"

if [ ! -d $SNAPSHOT_ROOT_PATH ]; then
    mkdir -p $SNAPSHOT_ROOT_PATH
fi

file_name=$SNAPSHOT_ROOT_PATH"/snapshot-"`date "+%Y%m%d%H%M%S"`
touch $file_name
echo "" > $file_name

for submod in $(ls ${SUBMOD_PATH})
do
    if [ -z submod ]; then
        continue
    fi

    . $SUBMOD_PATH/$submod >> $file_name
done