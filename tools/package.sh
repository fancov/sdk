#!/bin/bash

VERSION=$1
SRC_PATH=`pwd`/..
SPEC_FILE=$HOME"/rpmbuild/SPECS/"

echo  $SRC_PATH
echo  $SPEC_FILE

cp -rf ./apps.spec  $SPEC_FILE
sed -i "s:<version>:$VERSION:g"  $SPEC_FILE/apps.spec
sed -i "s:<sourcepath>:$SRC_PATH:g"  $SPEC_FILE/apps.spec

cd $HOME/rpmbuild/SPECS
rpmbuild -bb apps.spec