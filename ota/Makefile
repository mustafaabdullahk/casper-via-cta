CC = arm-linux-gnueabi-gcc -static -nostdlib -ffreestanding -fno-builtin -marm -march=armv7-a -O2 -Wall

all: backup.zip nosetup.zip devsetup.zip

%-binary: %.c sys.h
	$(CC) -o $@ $<

su_bin.h: su-binary
	xxd -i $< > $@

devsetup-binary: su_bin.h

%.zip: %-binary sign.py
	python3 sign.py $< $@

clean:
	rm -f *-binary *.zip su_bin.h
