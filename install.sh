#!/bin/bash
set -e

clang -arch arm64 sysinfod_server.c -o sysinfod

set +e
sudo -nv &> /dev/null
if [ $? != 0 ]
then
    echo "Moving sysinfod to /usr/local/bin; may prompt for sudo"
fi

set -e

sudo cp sysinfod /usr/local/bin/sysinfod
sudo cp com.logan.sysinfod.plist /Library/LaunchDaemons/com.logan.sysinfod.plist

# If our service is already launched, we need to remove it before re-launching
if launchctl print gui/501/com.logan.sysinfod &> /dev/null
then
    sudo launchctl bootout gui/501 /Library/LaunchDaemons/com.logan.sysinfod.plist
fi

sudo launchctl bootstrap gui/501 /Library/LaunchDaemons/com.logan.sysinfod.plist

echo "Successfully installed sysinfod"
