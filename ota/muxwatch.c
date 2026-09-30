// muxwatch: sample PERIPHS_PIN_MUX_0..15 (0xc11080b0) in a tight loop and print, per
// register, the OR and AND of every value seen. Stock aml_i2c muxes its pins only while a
// transfer runs, so run i2c traffic alongside and the OR/AND difference shows those bits.
#include "sys.h"

static void hex8(char *o, unsigned v) {
    for (int i = 7; i >= 0; i--, v >>= 4) o[i] = "0123456789abcdef"[v & 15];
}

void _start_c(int argc, char **argv) {
    (void)argc; (void)argv;
    int fd = sc3(SYS_open, (long)"/dev/mem", 010000 /*O_SYNC*/, 0);
    if (fd < 0) sc3(SYS_exit, 3, 0, 0);
    register long r7 asm("r7") = 192;  // mmap2(NULL, 4096, PROT_READ, MAP_SHARED, fd, 0xc1108)
    register long r0 asm("r0") = 0;
    register long r1 asm("r1") = 4096;
    register long r2 asm("r2") = 1;
    register long r3 asm("r3") = 1;
    register long r4 asm("r4") = fd;
    register long r5 asm("r5") = 0xc1108;
    asm volatile("svc 0" : "+r"(r0) : "r"(r7), "r"(r1), "r"(r2), "r"(r3), "r"(r4), "r"(r5) : "memory");
    if ((unsigned long)r0 > 0xfffff000UL) sc3(SYS_exit, 4, 0, 0);
    volatile unsigned *mux = (volatile unsigned *)(r0 + 0xb0);

    unsigned or_[16], and_[16];
    for (int i = 0; i < 16; i++) or_[i] = and_[i] = mux[i];
    for (long n = 0; n < 20000000; n++)
        for (int i = 0; i < 16; i++) { unsigned v = mux[i]; or_[i] |= v; and_[i] &= v; }

    char line[] = "MUX00 or=xxxxxxxx and=xxxxxxxx\n";
    for (int i = 0; i < 16; i++) {
        line[3] = '0' + i / 10; line[4] = '0' + i % 10;
        hex8(line + 9, or_[i]); hex8(line + 22, and_[i]);
        sc3(SYS_write, 1, (long)line, sizeof line - 1);
    }
    sc3(SYS_exit, 0, 0, 0);
}
