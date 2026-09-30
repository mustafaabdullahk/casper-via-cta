#!/bin/sh
# Reboot tablet to recovery, wait for "apply update from ADB", send a package.
# usage: ota/sideload.sh <package.zip>
set -e
A=${ADB:-$HOME/Android/Sdk/platform-tools/adb}
[ -f "$1" ] || { echo "usage: $0 <package.zip>"; exit 1; }
[ "$($A get-state 2>/dev/null)" = device ] && $A reboot recovery
echo ">> tablette 'apply update from ADB' secin"
until $A devices | grep -q sideload; do sleep 2; done
$A sideload "$1"
