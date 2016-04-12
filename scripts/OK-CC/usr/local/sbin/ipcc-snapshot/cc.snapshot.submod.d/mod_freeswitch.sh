#!/bin/bash

FS_ROOT="/usr/local/freeswitch"
FS_BIN=$FS_ROOT"/bin"
FS_CLI=$FS_BIN"/fs_cli"

echo "************************************************************************************************"
echo "FreeSWITCH Snapshot"
echo "FreeSWITCH Status:"
$FS_CLI --execute="show status"
echo "------------------------------------------------------------------------------------------"

echo "FreeSWITCH Sofia Status Profile:"
$FS_CLI --execute="sofia status profile"
echo "------------------------------------------------------------------------------------------"

echo "FreeSWITCH Sofia Status Gateway:"
$FS_CLI --execute="sofia status gateway"
echo "------------------------------------------------------------------------------------------"

echo "FreeSWITCH Sofia Status Profile Internal:"
$FS_CLI --execute="sofia status profile internal"
echo "------------------------------------------------------------------------------------------"

echo "FreeSWITCH Sofia Status Profile External:"
$FS_CLI --execute="sofia status profile external"
echo "------------------------------------------------------------------------------------------"

echo "FreeSWITCH Sofia Status Profile Internal Reg:"
$FS_CLI --execute="sofia status profile internal reg"
echo "------------------------------------------------------------------------------------------"

echo "FreeSWITCH Sofia Status Profile External Reg:"
$FS_CLI --execute="sofia status profile external reg"
echo "------------------------------------------------------------------------------------------"

echo "FreeSWITCH Show Calls:"
$FS_CLI --execute="show calls"
echo "------------------------------------------------------------------------------------------"

echo "FreeSWITCH Show Detail Calls:"
$FS_CLI --execute="show detailed_calls "
echo "------------------------------------------------------------------------------------------"

echo "FreeSWITCH Show Channels:"
$FS_CLI --execute="show channels"
echo "------------------------------------------------------------------------------------------"

echo "FreeSWITCH Show Bridges Calls:"
$FS_CLI --execute="show detailed_bridged_calls"
echo "------------------------------------------------------------------------------------------"


echo "************************************************************************************************" 