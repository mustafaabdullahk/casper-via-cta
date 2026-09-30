// update-binary for stock JB recovery: dump raw MTD partitions to /system/bak.
// ponytail: no libc — recovery kernel is 3.0.8, static glibc refuses to run on it.
// Read-only on NAND; only writes to /system/bak (Android resets /cache to 0770 on boot).

#include "sys.h"

static char buf[128 * 1024];

// Copy one partition; returns 0 on success.
static int dump(const char *name, const char *src, const char *dst) {
    int in = sc3(SYS_open, (long)src, 0 /*O_RDONLY*/, 0);
    if (in < 0) return -1;
    int out = sc3(SYS_open, (long)dst, 01 | 0100 | 01000 /*WRONLY|CREAT|TRUNC*/, 0644);
    if (out < 0) { sc3(SYS_close, in, 0, 0); say("cannot create ", dst); return -2; }
    long n;
    while ((n = sc3(SYS_read, in, (long)buf, sizeof buf)) > 0)
        if (sc3(SYS_write, out, (long)buf, n) != n) { n = -1; break; }
    sc3(SYS_close, in, 0, 0);
    sc3(SYS_close, out, 0, 0);
    sc3(SYS_chmod, (long)dst, 0644, 0);
    say(n < 0 ? "READ/WRITE ERROR: " : "ok: ", name);
    return n < 0 ? -3 : 0;
}

static const char *parts[] = { "bootloader", "logo", "aml_logo", "recovery", "boot" };

void _start_c(int argc, char **argv) {
    ui_init(argc, argv);
    say("casper-via: backing up MTD 0-4 to /system/bak", 0);
    if (mount_system()) { say("cannot mount /system", 0); sc3(SYS_exit, 2, 0, 0); }
    sc3(SYS_mkdir, (long)"/system/bak", 0755, 0);
    sc3(SYS_chmod, (long)"/system/bak", 0755, 0);
    int fails = 0;
    for (int i = 0; i < 5; i++) {
        char blk[] = "/dev/block/mtdblock0", chr[] = "/dev/mtd/mtd0";
        char dst[64] = "/system/bak/";
        blk[19] = '0' + i; chr[12] = '0' + i;
        int d = 12; for (const char *s = parts[i]; *s; s++) dst[d++] = *s;
        for (const char *s = ".img"; *s; s++) dst[d++] = *s;
        dst[d] = 0;
        // char device first: on this Amlogic NAND, mtdblock reads return garbage past 4 KiB
        int r = dump(parts[i], chr, dst);
        if (r == -1) r = dump(parts[i], blk, dst);
        if (r) { say("FAILED: ", parts[i]); fails++; }
    }
    // Keep recovery's own view of the kernel for later porting work.
    dump("cmdline", "/proc/cmdline", "/system/bak/cmdline.txt");
    long n = sc3(103 /*syslog*/, 3 /*READ_ALL*/, (long)buf, sizeof buf);
    int f = sc3(SYS_open, (long)"/system/bak/dmesg.txt", 01 | 0100 | 01000, 0644);
    if (f >= 0 && n > 0) sc3(SYS_write, f, (long)buf, n);
    if (f >= 0) { sc3(SYS_close, f, 0, 0); sc3(SYS_chmod, (long)"/system/bak/dmesg.txt", 0644, 0); }
    sc3(SYS_sync, 0, 0, 0);
    sc3(52 /*umount2*/, (long)"/system", 0, 0);
    say(fails ? "done with errors" : "done, all partitions saved", 0);
    sc3(SYS_exit, fails ? 1 : 0, 0, 0);
}
