#!/bin/bash

VERSION=$1
SRC_PATH=`pwd`/..
SPEC_FILE=$HOME"/rpmbuild/SPECS/"

echo  $SRC_PATH
echo  $SPEC_FILE

cp -rf ./ptc.spec  $SPEC_FILE
sed -i "s:<version>:$VERSION:g"  $SPEC_FILE/ptc.spec
sed -i "s:<sourcepath>:$SRC_PATH:g"  $SPEC_FILE/ptc.spec

cd $HOME/rpmbuild/SPECS
rpmbuild -bb --target=i386 ptc.spec