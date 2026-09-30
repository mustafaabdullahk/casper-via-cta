// update-binary: move the vendor setup wizard out of /system/app.
// /data lives in RAM on this tablet, so every boot is a "first boot" and the wizard reran.
// Provision.apk stays: it sets device_provisioned, without it Home doesn't work.
#include "sys.h"

void _start_c(int argc, char **argv) {
    ui_init(argc, argv);
    if (mount_system()) { say("cannot mount /system", 0); sc3(SYS_exit, 2, 0, 0); }
    sc3(SYS_mkdir, (long)"/system/bak", 0755, 0);
    long r = sc3(38 /*rename*/, (long)"/system/app/SetupWizard_signed_platform.apk",
                 (long)"/system/bak/SetupWizard_signed_platform.apk", 0);
    say(r == 0 ? "setup wizard moved to /system/bak" : r == -2 ? "setup wizard already gone" : "rename FAILED", 0);
    sc3(SYS_sync, 0, 0, 0);
    sc3(52 /*umount2*/, (long)"/system", 0, 0);
    sc3(SYS_exit, r == 0 || r == -2 ? 0 : 1, 0, 0);
}
