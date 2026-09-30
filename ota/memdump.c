// memdump <phys hex> <len hex>: mmap /dev/mem and write it to stdout.
// Stock 3.0.8 read(/dev/mem) only covers lowmem; mmap reaches the ramoops hole.
#include "sys.h"

static unsigned long hex(const char *s) {
    unsigned long v = 0;
    if (s[0] == '0' && (s[1] == 'x' || s[1] == 'X')) s += 2;
    for (; *s; s++) v = v * 16 + (*s <= '9' ? *s - '0' : (*s | 0x20) - 'a' + 10);
    return v;
}

void _start_c(int argc, char **argv) {
    if (argc != 3) sc3(SYS_exit, 2, 0, 0);
    unsigned long phys = hex(argv[1]), len = hex(argv[2]);
    int fd = sc3(SYS_open, (long)"/dev/mem", 0 | 010000 /*O_SYNC*/, 0);
    if (fd < 0) sc3(SYS_exit, 3, 0, 0);
    // mmap2(NULL, len, PROT_READ, MAP_SHARED, fd, pgoff): 6 args, so r5 by hand.
    register long r7 asm("r7") = 192;
    register long r0 asm("r0") = 0;
    register long r1 asm("r1") = len;
    register long r2 asm("r2") = 1;
    register long r3 asm("r3") = 1;
    register long r4 asm("r4") = fd;
    register long r5 asm("r5") = phys >> 12;
    asm volatile("svc 0" : "+r"(r0) : "r"(r7), "r"(r1), "r"(r2), "r"(r3), "r"(r4), "r"(r5) : "memory");
    if ((unsigned long)r0 > 0xfffff000UL) sc3(SYS_exit, 4, 0, 0);
    sc3(SYS_write, 1, r0, len);
    sc3(SYS_exit, 0, 0, 0);
}
