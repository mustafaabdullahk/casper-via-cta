// mtdflash <mtd char dev> <image>: erase partition, write image, read back and compare.
// Runs as root on stock Android. /dev/block/mtdblockN returns garbage on this Amlogic
// NAND (reads past the first 4 KiB are wrong), so only the char device is trusted.
#include "sys.h"

#define MEMGETINFO   0x80204d01  /* _IOR('M', 1, struct mtd_info_user) */
#define MEMERASE     0x40084d02  /* _IOW('M', 2, struct erase_info_user) */
#define MEMGETBADBLOCK 0x40084d0b /* _IOW('M', 11, loff_t) */

struct mtd_info { unsigned char type; unsigned flags, size, erasesize, writesize, oobsize; unsigned long long pad; };

static char page[32768], back[32768];

static void out(const char *a, const char *b) {
    sc3(SYS_write, 1, (long)a, slen(a));
    if (b) sc3(SYS_write, 1, (long)b, slen(b));
    sc3(SYS_write, 1, (long)"\n", 1);
}
static void die(const char *m) { out("mtdflash: ", m); sc3(SYS_exit, 1, 0, 0); }

void _start_c(int argc, char **argv) {
    if (argc != 3) die("usage: mtdflash /dev/mtd/mtdN image");
    int mtd = sc3(SYS_open, (long)argv[1], 2 /*O_RDWR*/, 0);
    int img = sc3(SYS_open, (long)argv[2], 0, 0);
    if (mtd < 0 || img < 0) die("cannot open mtd or image");

    struct mtd_info mi;
    if (sc3(54 /*ioctl*/, mtd, MEMGETINFO, (long)&mi) < 0) die("MEMGETINFO failed");
    if (mi.writesize > sizeof page) die("page size larger than buffer");
    long isz = sc3(19 /*lseek*/, img, 0, 2 /*SEEK_END*/);
    sc3(19, img, 0, 0);
    if (isz <= 0 || isz > mi.size) die("image empty or larger than partition");

    for (unsigned off = 0; off < mi.size; off += mi.erasesize) {
        long long o = off;
        if (sc3(54, mtd, MEMGETBADBLOCK, (long)&o) > 0) die("bad block in partition, refusing");
        unsigned ei[2] = { off, mi.erasesize };
        if (sc3(54, mtd, MEMERASE, (long)ei) < 0) die("erase failed");
    }
    out("erased", 0);

    // ponytail: whole-partition write, no bad-block skipping (partition is one erase block)
    long done = 0;
    while (done < isz) {
        long n = sc3(SYS_read, img, (long)page, mi.writesize);
        if (n <= 0) die("image read failed");
        for (long i = n; i < (long)mi.writesize; i++) page[i] = 0xff;
        if (sc3(SYS_write, mtd, (long)page, mi.writesize) != (long)mi.writesize) die("write failed");
        done += n;
    }
    out("written", 0);

    sc3(19, mtd, 0, 0); sc3(19, img, 0, 0);
    for (done = 0; done < isz; done += mi.writesize) {
        long n = sc3(SYS_read, img, (long)page, mi.writesize);
        if (sc3(SYS_read, mtd, (long)back, mi.writesize) != (long)mi.writesize) die("readback failed");
        for (long i = 0; i < n; i++) if (page[i] != back[i]) die("VERIFY MISMATCH");
    }
    out("verified OK", 0);
    sc3(SYS_exit, 0, 0, 0);
}
