# Pinmux + GPIO state as stock Android leaves it (backup/regs/stock-*.bin).
# ponytail: register copy instead of a Meson6 pinctrl/gpio driver; write one when a pin needs changing.
pinmux() {
  devmem 0xc11080b0 32 0x00000000
  devmem 0xc11080b4 32 0x03800000
  devmem 0xc11080b8 32 0x0000fc04  # + bits 10-15: SD card port B (u-boot mmcinfo state)
  devmem 0xc11080bc 32 0x00200000
  devmem 0xc11080c0 32 0x00000000
  # MUX5: stock aml_i2c sets these only during a transfer (caught with ota/muxwatch):
  #   bits 26-27 = i2c_A (touch, cameras), bits 30-31 = i2c_B (codec, accel). Keep them on.
  devmem 0xc11080c4 32 0xcc00000f
  devmem 0xc11080c8 32 0x00000000
  devmem 0xc11080cc 32 0x00000000
  devmem 0xc11080d0 32 0x00000000
  devmem 0xc11080d4 32 0x00001ab0
  devmem 0xc11080d8 32 0x00000000
  devmem 0xc11080dc 32 0x00000000
  devmem 0xc11080e0 32 0x00000000
  devmem 0xc11080e4 32 0x00000000
  devmem 0xc11080e8 32 0x00080000
  devmem 0xc11080ec 32 0x00000000
}
# GPIO banks 0-6 at 0xc1108030: (EN_N, O, I) triplets; O before EN_N so pins come up at the stock level
gpio_state() {
  devmem 0xc1108034 32 0xf7ffffff; devmem 0xc1108030 32 0xf7ffffff
  devmem 0xc1108040 32 0xffffffff; devmem 0xc110803c 32 0xffffffff
  devmem 0xc110804c 32 0xffffffff; devmem 0xc1108048 32 0xfffdfef7
  devmem 0xc1108058 32 0xffffffff; devmem 0xc1108054 32 0xffffffff
  devmem 0xc1108064 32 0xffffffff; devmem 0xc1108060 32 0xffffefff
  # bank5 stays as u-boot left it (bits 23-28 and 31 low outputs): Android had no SD card and
  # switches these off; with its values the card sticks busy. Touch does not care.
  devmem 0xc1108070 32 0x607fffff; devmem 0xc110806c 32 0x607fffff
  devmem 0xc110807c 32 0x00000000; devmem 0xc1108078 32 0x00000000
  devmem 0xc8100024 32 0x8fff0ff7  # AO O_EN_N/O
}
