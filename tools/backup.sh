#!/bin/sh
# Back up the raw NAND partitions of a rooted stock tablet (after ota/devsetup.zip) over adb.
# Reads /dev/mtd/mtdN (char device, 32 KiB pages); /dev/block/mtdblockN returns garbage here.
# The stock boot/recovery images are checked by their uImage CRC.
set -e
cd "$(dirname "$0")/.."
A="adb -d"
mkdir -p backup
i=0
for p in bootloader logo aml_logo recovery boot; do
	$A shell "su -c 'dd if=/dev/mtd/mtd$i of=/data/local/tmp/$p.img bs=32768 2>/dev/null; chmod 644 /data/local/tmp/$p.img'"
	$A pull /data/local/tmp/$p.img backup/ >/dev/null
	$A shell rm /data/local/tmp/$p.img
	i=$((i + 1))
done
python3 - <<'PY'
import struct, zlib
for n in ("boot", "recovery"):
    d = open(f"backup/{n}.img", "rb").read(); ks = struct.unpack("<I", d[8:12])[0]; k = d[2048:2048 + ks]
    ok = d[:8] == b"ANDROID!" and zlib.crc32(k[64:64 + struct.unpack(">I", k[12:16])[0]]) == struct.unpack(">I", k[24:28])[0]
    print(f"{n}.img: {'OK' if ok else 'BOZUK - bu yedege guvenmeyin'}")
PY
(cd backup && sha256sum *.img > SHA256SUMS)
