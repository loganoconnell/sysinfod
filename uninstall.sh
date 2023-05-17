#!/bin/bash
set -e

# If our service is launched, we need to remove it
if launchctl print gui/501/com.logan.sysinfod &> /dev/null
then
    sudo launchctl bootout gui/501 /Library/LaunchDaemons/com.logan.sysinfod.plist
fi

echo "Successfully uninstalled sysinfod"
