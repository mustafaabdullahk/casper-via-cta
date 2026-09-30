#!/bin/sh
# Push an image + mtdflash over adb (USB; TABLET_IP=x.x.x.x for Wi-Fi) and write it to recovery (mtd3).
# /data is RAM-backed on this tablet, so the tool is re-pushed every time.
# usage: kernel/flash-test.sh [image]   (default build/recovery-test.img; backup/recovery.img restores stock)
set -e
cd "$(dirname "$0")/.."
IMG=${1:-build/recovery-test.img}
ADB=$HOME/Android/Sdk/platform-tools/adb
if [ -n "$TABLET_IP" ]; then $ADB connect $TABLET_IP:5555 >/dev/null; A="$ADB -s $TABLET_IP:5555"; else A="$ADB -d"; fi
$A wait-for-device
$A push ota/mtdflash-binary /data/local/tmp/mtdflash >/dev/null
$A push "$IMG" /data/local/tmp/img >/dev/null
$A shell 'chmod 755 /data/local/tmp/mtdflash; su -c "/data/local/tmp/mtdflash /dev/mtd/mtd3 /data/local/tmp/img && sync"'
