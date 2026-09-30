#!/usr/bin/env python3
"""Drive the mainline tablet over its USB serial gadget (/dev/ttyACM0).

  tools/tablet.py run 'dmesg | tail'      run a shell command, print output
  tools/tablet.py push LOCAL REMOTE       copy a file (base64 over the tty)
  tools/tablet.py kexec [ZIMAGE [DTB]]    load + boot a kernel without touching NAND
                                          (default: build/arch/arm/boot/zImage + casper dtb)

The tablet side is a plain `sh` reading lines from /dev/ttyGS0 (see kernel/init), so every
command is framed with an end marker carrying the exit status.
"""
import base64, os, re, sys, termios, time, uuid

DEV = os.environ.get("TABLET_TTY") or next((f"/dev/ttyACM{i}" for i in range(4) if os.path.exists(f"/dev/ttyACM{i}")), "/dev/ttyACM0")
BUILD = os.path.join(os.path.dirname(__file__), "..", "build")
CMDLINE = "console=ttyAML0,115200 console=tty0 panic=0 loglevel=8 ignore_loglevel"


def open_tty(timeout=30):
    end = time.time() + timeout
    while not os.path.exists(DEV):
        if time.time() > end:
            sys.exit(f"{DEV} yok: tablet mainline'da mi, kablo bagli mi?")
        time.sleep(0.5)
    fd = os.open(DEV, os.O_RDWR | os.O_NOCTTY)
    a = termios.tcgetattr(fd)
    a[0] = a[1] = a[3] = 0                      # iflag, oflag, lflag: raw
    a[2] = termios.CS8 | termios.CREAD | termios.CLOCAL
    a[6][termios.VMIN], a[6][termios.VTIME] = 0, 1
    termios.tcsetattr(fd, termios.TCSANOW, a)
    termios.tcflush(fd, termios.TCIOFLUSH)
    termios.tcflush(fd, termios.TCIFLUSH)
    return fd


def run(fd, cmd, timeout=60, echo=True):
    mark = "__END_" + uuid.uuid4().hex[:8]
    os.write(fd, f"{cmd}\necho {mark}$?\n".encode())
    buf, end = b"", time.time() + timeout
    done = re.compile(re.escape(mark.encode()) + rb"(\d+)\r?\n")
    while not (m := done.search(buf)):
        if time.time() > end:
            raise TimeoutError(f"no reply to: {cmd}\n{buf.decode(errors='replace')}")
        buf += os.read(fd, 65536)
    out, status = buf[:m.start()].decode(errors="replace"), int(m.group(1))
    # the gadget tty may still echo; drop our own command lines from the output
    for line in (cmd, f"echo {mark}$?"):
        out = out.replace(line + "\n", "", 1)
    if echo:
        sys.stdout.write(out)
    return status, out


def push(fd, local, remote):
    data = base64.b64encode(open(local, "rb").read())
    size = os.path.getsize(local)
    mark = "__PUSHED_" + uuid.uuid4().hex[:8]
    # head -c reads exactly the payload, so no EOF char is needed on a raw tty
    os.write(fd, f"head -c {len(data)} | base64 -d > {remote}; echo {mark}$(wc -c < {remote})\n".encode())
    for i in range(0, len(data), 4096):
        os.write(fd, data[i:i + 4096])
    buf, end = b"", time.time() + 120
    done = re.compile(re.escape(mark.encode()) + rb"\s*(\d+)\r?\n")
    while not (m := done.search(buf)):
        if time.time() > end:
            raise TimeoutError("push timed out")
        buf += os.read(fd, 65536)
    got = int(m.group(1))
    if got != size:
        sys.exit(f"push {local}: {got} != {size} bytes")
    print(f"pushed {local} -> {remote} ({size} bytes)")


def kexec(fd, zimage, dtb):
    push(fd, zimage, "/tmp/zImage")
    push(fd, dtb, "/tmp/board.dtb")
    st, _ = run(fd, f"kexec -l /tmp/zImage --dtb=/tmp/board.dtb --command-line='{CMDLINE}'")
    if st:
        sys.exit("kexec -l failed")
    os.write(fd, b"kexec -e\n")
    os.close(fd)
    print("kexec -e sent; waiting for the new kernel's gadget...")
    time.sleep(5)
    fd = open_tty(60)
    run(fd, "uname -a; cat /proc/uptime")


if __name__ == "__main__":
    if len(sys.argv) < 2:
        sys.exit(__doc__)
    fd = open_tty()
    op = sys.argv[1]
    if op == "run":
        st, _ = run(fd, " ".join(sys.argv[2:]))
        sys.exit(st)
    elif op == "push":
        push(fd, sys.argv[2], sys.argv[3])
    elif op == "kexec":
        z = sys.argv[2] if len(sys.argv) > 2 else os.path.join(BUILD, "arch/arm/boot/zImage")
        d = sys.argv[3] if len(sys.argv) > 3 else os.path.join(BUILD, "arch/arm/boot/dts/amlogic/meson6-casper-via.dtb")
        kexec(fd, z, d)
    else:
        sys.exit(__doc__)
