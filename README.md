# Casper Via CTA-E07-14A için mainline Linux

2014 yapımı **Casper Via CTA-E07-14A** tabletine (Amlogic AML8726-MX / Meson6, Android 4.1.1)
güncel mainline Linux (7.2) ve SD kart üzerinde **Alpine Linux** ile dokunmatik bir masaüstü
(labwc) kurar. Stok Android NAND'de olduğu gibi kalır; mainline kernel yalnızca **recovery**
bölümüne yazılır ve **Vol+ ile açılışta** başlar.

> ⚠️ **Uyarı:** Bu proje resmi değildir ve Casper ile bağlantısı yoktur. Tabletin NAND'ine
> yazar, stok recovery'yi siler ve root erişimi açar. Cihazı kullanılamaz hale getirebilir,
> garantiyi geçersiz kılar. Her adımı anlayarak ve **yedek alarak** uygulayın; sorumluluk
> sizdedir.

## Durum

| Donanım | Durum | Not |
|---|---|---|
| CPU (Cortex-A9) | ⚠️ | Tek çekirdek, u-boot'un bıraktığı hızda (≈816 MHz). SMP ve cpufreq yok |
| Ekran 1024×600 | ✅ | `simpledrm` (u-boot'un framebuffer'ı, GPU hızlandırması yok) |
| Arka ışık | ✅ | `/sys/class/backlight/c1108768.backlight`, 0–13 |
| Dokunmatik Goodix GT811 | ✅ | Kendi sürücümüz (`patches/0002`), yoklama ile |
| İvmeölçer MMA8653 | ✅ | IIO |
| Pil (AXP209) | ✅ | Kapasite/voltaj/akım. Şarj/USB durumu yok (PMIC kesmesi bağlı değil) |
| microSD | ✅ | Yalnızca 1-bit mod, 25 MHz (4-bit'te veri bozuluyor) |
| Wi-Fi MediaTek MT7601U | ✅ | `mt7601u` + firmware |
| USB OTG | ✅ | Cihaz modu: seri konsol (`/dev/ttyACM0`) |
| Ses (WM8960) | ❌ | Meson6 I2S sürücüsü yok |
| GPU Mali-400 MP2 | ❌ | `lima` henüz bağlanmadı |
| Kameralar | ❌ | |
| Dahili NAND | ❌ | Mainline'da Amlogic NFTL sürücüsü yok; sistem SD karttan çalışır |

## Gereksinimler

- Linux bir PC ve veri taşıyan bir micro-USB kablosu
- En az 4 GB microSD kart (içeriği silinir)
- Paketler (Debian/Ubuntu adlarıyla): `adb`, `gcc-arm-linux-gnueabihf`, `gcc-arm-linux-gnueabi`,
  `mkbootimg`, `git`, `make`, `flex`, `bison`, `bc`, `libssl-dev`, `python3`, `python3-cryptography`

## Kurulum

### 1. Kaynaklar

```sh
git clone https://github.com/mustafaabdullahk/casper-via-cta.git
cd casper-via-cta
git clone --depth 1 -b v7.2.8 https://git.kernel.org/pub/scm/linux/kernel/git/stable/linux.git linux
```

### 2. Tableti hazırlama (root + kalıcı adb)

Tablette **Ayarlar → Geliştirici seçenekleri → USB hata ayıklama**'yı açın. Stok recovery
herkese açık AOSP test anahtarıyla imzalanmış paketleri kabul eder; bu repodaki paketler o
anahtarla imzalanır (anahtar ilk derlemede AOSP'den indirilir).

```sh
make -C ota                      # imzalı devsetup.zip / backup.zip / nosetup.zip
ota/sideload.sh ota/devsetup.zip # recovery'de "apply update from ADB" seçin
```

`devsetup.zip`, `/system/xbin/su` kurar ve adb'yi kalıcı olarak açık tutar. Normal açılışla
Android'e dönün.

> Bazı cihazlarda `/data` bozuk olduğundan RAM'de çalışır ve kurulum sihirbazı her açılışta
> gelir; o durumda `ota/sideload.sh ota/nosetup.zip` sihirbazı `/system/bak`'a taşır.

### 3. Yedek

```sh
tools/backup.sh
```

`backup/` altına bootloader, logo, recovery ve boot bölümlerini alır ve `boot`/`recovery`
imajlarını CRC ile doğrular. **Bu yedek olmadan devam etmeyin.** Dosyalar Casper'a aittir,
paylaşmayın.

### 4. Kernel

```sh
kernel/build.sh        # yamaları uygular, busybox + MT7601U firmware'ini indirir, derler
kernel/flash-test.sh   # build/recovery-test.img'i recovery bölümüne yazar ve doğrular
```

Stok recovery bu adımda silinir. Geri yüklemek için: `kernel/flash-test.sh backup/recovery.img`.

### 5. SD kart (Alpine + masaüstü)

Kartı PC'de tek bölüm, **ext4** olarak biçimlendirin (örnek: `sudo mkfs.ext4 -L casper-root
/dev/sdX1`). Sonra kök dosya sistemini üretip kartın köküne kopyalayın:

```sh
WIFI_SSID='agim' WIFI_PASS='sifrem' SSH_PUBKEY=~/.ssh/id_ed25519.pub \
  ROOT_PASSWORD='bir-sifre' tools/mkrootfs.sh
cp tools/alpine/casper-rootfs.tar.gz /media/$USER/casper-root/ && sync
```

Wi-Fi şifresi dosyaya düz metin olarak değil, PSK olarak yazılır. `ROOT_PASSWORD` verilmezse
SSH yalnızca anahtarla açılır.

### 6. Açılış

Kartı tablete takın, tableti kapatın, **Vol+ basılı tutarken güç tuşuna** basın. İlk açılışta
arşiv açılır (birkaç dakika). Ardından Wi-Fi, SSH (`ssh root@<ip>`, IP ekranda yazar), TLP ve
masaüstü kendiliğinden başlar.

- Sol dock: uygulamalar, terminal, NetSurf, Mousepad, dosyalar, ekran klavyesi
- Üst panel: pencereler, parlaklık, Wi-Fi, pil, saat

**Android'e dönüş:** tableti normal açın; `boot` bölümüne dokunulmaz.

## Bilinen sorunlar

- **5.–6. adımlardaki ilk açılış otomasyonu** (arşivin tablette açılması ve internetsiz
  `apk fix`) henüz sıfırdan bir kartla uçtan uca denenmedi; geliştirici tabletinde aynı
  adımlar elle yapıldı. Sorun yaşarsanız USB konsoldan (`tools/tablet.py`) bakın ve issue açın.
- Mainline'dan `reboot` tableti Android yerine şarj ekranına düşürür; güç tuşuyla açın.
- Tek çekirdek ve yazılımla çizim: masaüstü kullanılabilir ama yavaştır.
- SD kart 1-bit modda çalıştığı için yavaştır.
- Dokunmatik ve ivmeölçer sürücüleri açılışta pinmux'tan önce başlar; `kernel/init` onları
  sonradan yeniden bağlar.

## Geliştirme

- **USB konsol:** mainline açıkken `tools/tablet.py run 'dmesg | tail'`, `tools/tablet.py push`.
- **Meson6 pinctrl/saat sürücüsü yok:** `kernel/pinmux.sh` stok Android'deki pinmux/GPIO
  durumunu register kopyasıyla uygular. Değerler `backup/regs/` altındaki dökümlerden gelir.
- **Kernel yamaları:** `patches/`
  - `0001` i2c-meson: kesmesiz veriyolunda yoklama
  - `0002` GT811 sürücüsü
  - `0003` AXP209: kesmesiz pil/ADC
  - `0004` meson-mx-sdio: port seçimi
  - `0005` Meson6 LED PWM arka ışık
- **Donanım notları:** DTS (`dts/meson6-casper-via.dts`) ve `kernel/init`'teki yorumlar, her
  değerin nasıl bulunduğunu anlatır. Örnekler: GT811 config'inin son baytındaki "uygula"
  bayrağı, SD yuvasının güç pinleri, i2c pinmux'unun yalnızca aktarım sırasında açılması.

Katkılar (issue ve PR) memnuniyetle karşılanır. Lütfen donanımla ilgili bulguları, nasıl
doğruladığınızla birlikte yazın.

## Lisans

Kernel yamaları, sürücüler ve DTS dosyaları Linux ile aynı koşullarda **GPL-2.0** (DTS'ler
`GPL-2.0 OR MIT`) lisanslıdır.
