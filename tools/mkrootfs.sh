#!/bin/sh
# Build the Alpine armv7 root filesystem for the SD card, as a root-owned tarball.
# Runs as a normal user on x86_64 (static apk in usermode); package install scripts are
# skipped here and run on the tablet at first boot (kernel/init: apk fix from the cache).
#
#   WIFI_SSID=... WIFI_PASS=... [SSH_PUBKEY=~/.ssh/id_ed25519.pub] [ROOT_PASSWORD=...] \
#     tools/mkrootfs.sh            -> tools/alpine/casper-rootfs.tar.gz
set -e
cd "$(dirname "$0")/alpine" 2>/dev/null || { mkdir -p "$(dirname "$0")/alpine"; cd "$(dirname "$0")/alpine"; }
REL=v3.24
B=https://dl-cdn.alpinelinux.org/alpine/$REL
PKGS="font-awesome alpine-base iw wpa_supplicant wireless-regdb dropbear tlp i2c-tools htop
      eudev seatd dbus labwc wvkbd foot waybar fuzzel
      netsurf mousepad pcmanfm adwaita-icon-theme papirus-icon-theme font-dejavu xkeyboard-config"

if [ ! -x sbin/apk.static ]; then
	f=$(python3 -c "import re,urllib.request as u; print(sorted(set(re.findall(r'apk-tools-static-[0-9][^\"]*?\.apk', u.urlopen('$B/main/x86_64/').read().decode())))[-1])")
	python3 -c "import sys,urllib.request as u; open('apk-tools-static.apk','wb').write(u.urlopen(sys.argv[1]).read())" "$B/main/x86_64/$f"
	tar -xzf apk-tools-static.apk sbin/apk.static
fi

rm -rf staging && mkdir -p staging/etc/apk/cache
# keep the .apk files in the image so first-boot `apk fix` works without network
./sbin/apk.static --arch armv7 -X "$B/main" -X "$B/community" --allow-untrusted --root staging \
	--initdb --no-scripts --usermode --cache-dir "$PWD/staging/etc/apk/cache" -U add $PKGS

cp -a ../../rootfs-overlay/. staging/
printf 'nameserver 1.1.1.1\nnameserver 8.8.8.8\n' > staging/etc/resolv.conf
touch staging/.casper-firstboot

if [ -n "$WIFI_SSID" ]; then
	mkdir -p staging/etc/wpa_supplicant
	psk=$(python3 -c "import hashlib,sys; print(hashlib.pbkdf2_hmac('sha1', sys.argv[2].encode(), sys.argv[1].encode(), 4096, 32).hex())" "$WIFI_SSID" "$WIFI_PASS")
	printf 'network={\n\tssid="%s"\n\tpsk=%s\n}\n' "$WIFI_SSID" "$psk" > staging/etc/wpa_supplicant/wpa_supplicant.conf
	chmod 600 staging/etc/wpa_supplicant/wpa_supplicant.conf
fi
if [ -n "$SSH_PUBKEY" ]; then
	mkdir -p staging/root/.ssh && cp "$SSH_PUBKEY" staging/root/.ssh/authorized_keys
	chmod 700 staging/root/.ssh && chmod 600 staging/root/.ssh/authorized_keys
fi
if [ -n "$ROOT_PASSWORD" ]; then
	h=$(python3 -c "import crypt,sys; print(crypt.crypt(sys.argv[1], crypt.mksalt(crypt.METHOD_SHA512)))" "$ROOT_PASSWORD" 2>/dev/null)
	sed -i "s|^root:[^:]*:|root:$h:|" staging/etc/shadow
fi

# apk --usermode leaves some files without the owner read bit; tar needs to read them
find staging -type f ! -perm -u+r -exec chmod u+r {} +
find staging -type d ! -perm -u+rx -exec chmod u+rx {} +
tar --owner=0 --group=0 --numeric-owner -czf casper-rootfs.tar.gz -C staging .
ls -l casper-rootfs.tar.gz
