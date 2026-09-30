#!/bin/sh
# Build mainline for the Casper Via and wrap it the way the stock bootloader wants:
#   Android boot image { kernel = uImage(zImage + appended dtb), load 0x80008000 }
# The bootloader's "recovery" command only reads 0x600000 bytes, so that is the size cap.
set -e
cd "$(dirname "$0")/.."
TOP=$PWD
K=$TOP/linux O=$TOP/build
MK="make -C $K O=$O ARCH=arm CROSS_COMPILE=arm-linux-gnueabihf- -j$(nproc)"

# third-party blobs that are downloaded, not committed
fetch() { [ -s "$1" ] || { echo "fetching $1"; python3 -c "import sys,urllib.request as u; open(sys.argv[1],'wb').write(u.urlopen(u.Request(sys.argv[2],headers={'User-Agent':'curl/8'})).read())" "$1" "$2"; }; }
fetch kernel/busybox https://busybox.net/downloads/binaries/1.31.0-defconfig-multiarch-musl/busybox-armv7l
fetch kernel/firmware/mt7601u.bin https://gitlab.com/kernel-firmware/linux-firmware/-/raw/main/mediatek/mt7601u.bin
chmod +x kernel/busybox

# our kernel changes (idempotent: skip patches that are already applied)
for p in patches/*.patch; do
	git -C "$K" apply --reverse --check "$TOP/$p" 2>/dev/null || git -C "$K" apply "$TOP/$p"
done

# config + initramfs list carry @TOP@ so the tree can live anywhere
mkdir -p "$O"
sed "s|@TOP@|$TOP|g" kernel/casper.config > "$O/casper.config.gen"
sed "s|@TOP@|$TOP|g" kernel/initramfs.list > "$O/initramfs.list"

# out-of-tree board files -> kernel dts dir
for f in meson6.dtsi meson6-casper-via.dts; do ln -sf "$TOP/dts/$f" "$K/arch/arm/boot/dts/amlogic/$f"; done
grep -q meson6-casper-via "$K/arch/arm/boot/dts/amlogic/Makefile" ||
	echo 'dtb-$(CONFIG_MACH_MESON6) += meson6-casper-via.dtb' >> "$K/arch/arm/boot/dts/amlogic/Makefile"

[ -f "$O/.config" ] && [ "$O/.config" -nt kernel/casper.config ] ||
	KCONFIG_ALLCONFIG=$O/casper.config.gen $MK allnoconfig
$MK zImage amlogic/meson6-casper-via.dtb

cat "$O/arch/arm/boot/zImage" "$O/arch/arm/boot/dts/amlogic/meson6-casper-via.dtb" > "$O/zImage-dtb"
python3 - "$O/zImage-dtb" "$O/uImage" <<'EOF'
import struct, sys, time, zlib
data = open(sys.argv[1], 'rb').read()
# ih_os=linux(5) ih_arch=arm(2) ih_type=kernel(2) ih_comp=none(0)
hdr = struct.pack('>7I4B32s', 0x27051956, 0, int(time.time()), len(data), 0x80008000, 0x80008000,
                  zlib.crc32(data), 5, 2, 2, 0, b'casper-via mainline')
hdr = hdr[:4] + struct.pack('>I', zlib.crc32(hdr)) + hdr[8:]
open(sys.argv[2], 'wb').write(hdr + data)
EOF
# same header layout as the stock images (base 0x10000000, page 2048)
mkbootimg --kernel "$O/uImage" --base 0x10000000 --pagesize 2048 -o "$O/recovery-test.img"

size=$(stat -c %s "$O/recovery-test.img")
echo "recovery-test.img: $size bytes (limit 6291456)"
[ "$size" -le 6291456 ] || { echo "TOO BIG for bootloader's 6 MiB read"; exit 1; }
