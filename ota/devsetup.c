// update-binary: make the tablet usable as a dev target despite /data living in RAM.
//  - /system/xbin/su (setuid root) so the kernel test loop needs no recovery afterwards
//  - build.prop: keep adb on across boots (Settings toggle is lost)
#include "sys.h"
#include "su_bin.h"  // xxd -i su-binary

// USB only: service.adb.tcp.port makes adbd drop USB, and Wi-Fi creds die with /data.
#define PROP "\npersist.sys.usb.config=mass_storage,adb\n"

static char buf[64 * 1024];

static int contains(const char *hay, int n, const char *needle) {
    int m = slen(needle);
    for (int i = 0; i + m <= n; i++) {
        int j = 0;
        while (j < m && hay[i + j] == needle[j]) j++;
        if (j == m) return 1;
    }
    return 0;
}

void _start_c(int argc, char **argv) {
    ui_init(argc, argv);
    if (mount_system()) { say("cannot mount /system", 0); sc3(SYS_exit, 2, 0, 0); }
    int fails = 0;

    sc3(SYS_mkdir, (long)"/system/xbin", 0755, 0);
    int f = sc3(SYS_open, (long)"/system/xbin/su", 01 | 0100 | 01000, 0755);
    if (f >= 0 && sc3(SYS_write, f, (long)su_binary, su_binary_len) == su_binary_len) {
        sc3(SYS_close, f, 0, 0);
        sc3(182 /*chown*/, (long)"/system/xbin/su", 0, 0);
        sc3(SYS_chmod, (long)"/system/xbin/su", 06755, 0);
        say("ok: /system/xbin/su", 0);
    } else { say("FAILED: su", 0); fails++; }

    f = sc3(SYS_open, (long)"/system/build.prop", 0, 0);
    long n = f >= 0 ? sc3(SYS_read, f, (long)buf, sizeof buf) : -1;
    if (f >= 0) sc3(SYS_close, f, 0, 0);
    if (n < 0) { say("FAILED: read build.prop", 0); fails++; }
    else if (contains(buf, n, "persist.sys.usb.config=mass_storage,adb")) say("ok: adb prop already set", 0);
    else {
        f = sc3(SYS_open, (long)"/system/build.prop", 01 | 02000 /*WRONLY|APPEND*/, 0);
        if (f >= 0 && sc3(SYS_write, f, (long)PROP, sizeof PROP - 1) == sizeof PROP - 1) say("ok: adb stays enabled", 0);
        else { say("FAILED: build.prop", 0); fails++; }
        if (f >= 0) sc3(SYS_close, f, 0, 0);
    }

    sc3(SYS_sync, 0, 0, 0);
    sc3(52 /*umount2*/, (long)"/system", 0, 0);
    say(fails ? "done with errors" : "done", 0);
    sc3(SYS_exit, fails ? 1 : 0, 0, 0);
}
