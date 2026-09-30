#!/bin/sh
# Capture raw touch events from the tablet for N seconds and print the average position.
# usage: TABLET_IP=x.x.x.x tools/touchcap.sh [seconds] [label]
cd "$(dirname "$0")/.."
T=${1:-5}
ssh -o UserKnownHostsFile=tools/known_hosts -o ConnectTimeout=8 -i "$HOME/.ssh/id_ed25519" root@${TABLET_IP:?TABLET_IP=<tablet ip> gerekli} \
	"timeout $T cat /dev/input/event0" 2>/dev/null | python3 -c '
import struct, sys
raw = sys.stdin.buffer.read(); x = y = None; pts = []
for i in range(0, len(raw) - 15, 16):
    _, _, t, c, v = struct.unpack("<IIHHi", raw[i:i + 16])
    if t == 3 and c == 0x35: x = v
    if t == 3 and c == 0x36: y = v
    if t == 0 and x is not None and y is not None: pts.append((x, y))
if not pts: sys.exit("'"${2:-touch}"': no touch")
xs, ys = zip(*pts)
print(f"'"${2:-touch}"': n={len(pts)} x~{sum(xs)//len(xs)} y~{sum(ys)//len(ys)} (x {min(xs)}-{max(xs)}, y {min(ys)}-{max(ys)})")'
