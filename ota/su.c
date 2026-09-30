// Minimal su: setuid-root, runs /system/bin/sh with the caller's args ("su -c cmd" works).
// ponytail: no Superuser prompt — dev tablet, anything on it can get root. Add a
// manager app if the tablet ever holds anything that matters.
#include "sys.h"

void _start_c(int argc, char **argv) {
    char **envp = argv + argc + 1;
    sc3(214 /*setgid32*/, 0, 0, 0);
    sc3(213 /*setuid32*/, 0, 0, 0);
    argv[0] = "sh";
    sc3(11 /*execve*/, (long)"/system/bin/sh", (long)argv, (long)envp);
    sc3(SYS_exit, 127, 0, 0);
}
