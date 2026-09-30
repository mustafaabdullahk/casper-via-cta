// Shared bits for libc-free update-binaries (ARM EABI syscalls + recovery ui_print).
#pragma once

#define SYS_exit 1
#define SYS_read 3
#define SYS_write 4
#define SYS_open 5
#define SYS_close 6
#define SYS_chmod 15
#define SYS_sync 36
#define SYS_mkdir 39

static inline long sc3(long n, long a, long b, long c) {
    register long r7 asm("r7") = n;
    register long r0 asm("r0") = a;
    register long r1 asm("r1") = b;
    register long r2 asm("r2") = c;
    asm volatile("svc 0" : "+r"(r0) : "r"(r7), "r"(r1), "r"(r2) : "memory");
    return r0;
}

static inline long sc5(long n, long a, long b, long c, long d, long e) {
    register long r7 asm("r7") = n;
    register long r0 asm("r0") = a;
    register long r1 asm("r1") = b;
    register long r2 asm("r2") = c;
    register long r3 asm("r3") = d;
    register long r4 asm("r4") = e;
    asm volatile("svc 0" : "+r"(r0) : "r"(r7), "r"(r1), "r"(r2), "r"(r3), "r"(r4) : "memory");
    return r0;
}

static int ui = -1;

// gcc emits memset for zero-filled local arrays even with -ffreestanding.
void *memset(void *d, int c, unsigned n) { char *p = d; while (n--) *p++ = c; return d; }

static inline int slen(const char *s) { int n = 0; while (s[n]) n++; return n; }

static inline void say(const char *a, const char *b) {
    sc3(SYS_write, ui, (long)"ui_print ", 9);
    sc3(SYS_write, ui, (long)a, slen(a));
    if (b) sc3(SYS_write, ui, (long)b, slen(b));
    sc3(SYS_write, ui, (long)"\nui_print\n", 10);
}

static inline void ui_init(int argc, char **argv) {
    if (argc > 2) { ui = 0; for (char *p = argv[2]; *p; p++) ui = ui * 10 + (*p - '0'); }
}

// ret -16 = EBUSY: recovery already mounted it, fine.
static inline int mount_system(void) {
    long m = sc5(21 /*mount*/, (long)"/dev/block/system", (long)"/system", (long)"ext4", 0, 0);
    return m < 0 && m != -16 ? -1 : 0;
}

// Entry: pass the raw stack (argc, argv...) to _start_c.
asm(".global _start\n_start:\n mov r0, sp\n ldr r1, [r0]\n add r2, r0, #4\n mov r0, r1\n mov r1, r2\n bl _start_c\n");
