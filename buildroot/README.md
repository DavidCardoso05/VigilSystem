# Buildroot (Raspberry Pi 4, 64-bit)

Buildroot tree: `~/buildroot/buildroot-2026.08` (the copy in `~/workspace/buildroot/` is an identical duplicate).
Base: `raspberrypi4_64_defconfig` · toolchain: Bootlin aarch64 glibc stable (GCC 14, kernel headers 5.4) · kernel: Raspberry Pi `bcm2711`.

| File | What it is |
|------|------------|
| `configs/vigil_fragment.config` | Only the VigilSystem changes (readable list) |
| `configs/vigil_rpi4_defconfig` | Full saved config (`make savedefconfig` writes here) |
| `overlay/` | Files copied onto the Pi's root filesystem |
| `overlay/etc/init.d/S40wifi` | Loads brcmfmac and starts wpa_supplicant (dhcpcd then gets the IP) |
| `overlay/etc/init.d/S99vigil` | Starts `/usr/bin/vigil` without waiting for NTP |
| `overlay/etc/chrony.conf` | NTP, steps the clock at boot (no RTC on the Pi) |
| `overlay/etc/wpa_supplicant.conf` | **Your Wi-Fi name/password, ignored by git.** Template: `.example` |
| `overlay/etc/vigil/broker.conf.example` | MQTT settings template for the Pi |

Boot order: `S40network` (eth0) → **`S40wifi`** → `S41dhcpcd` → `S49chronyd` → `S50dropbear/sshd` → **`S99vigil`**

## Everyday commands (run in `~/buildroot/buildroot-2026.08`)

```bash
make menuconfig
```
```bash
make savedefconfig
```
Run `make savedefconfig` after any change in menuconfig. It saves the config into this repo.

```bash
make -j$(nproc) 2>&1 | tee build.log
```
The first build after adding packages takes a while. Later builds are incremental.

Restore the config from the repo (e.g. on another PC):
```bash
make BR2_DEFCONFIG=/home/david/VigilSystem/buildroot/configs/vigil_rpi4_defconfig defconfig
```

## Flash the SD card

Find the card with `lsblk` (e.g. `/dev/sdb` or `/dev/mmcblk0`). **Double-check it: `dd` overwrites the whole disk.**
```bash
sudo dd if=output/images/sdcard.img of=/dev/sdX bs=4M conv=fsync status=progress
```

## Notes
- **Removing a package** in menuconfig doesn't delete its files from `output/target`. That needs `make clean all` (a full rebuild).
- **SSH:** the image currently has both OpenSSH and Dropbear enabled, so they compete for port 22. Keep one (Dropbear is lighter) and do a clean rebuild when convenient.
- **libgpiod v1 (1.6.5)** is used, not v2: libgpiod2 needs kernel headers ≥ 5.10, and the Bootlin stable toolchain has 5.4. The class diagram's `gpiod_line_request*` is v2 API. Either write the GPIO code for v1, or switch to the Bootlin *bleeding-edge* toolchain (full rebuild).
- **Later (Ana's hardware part):** a custom `config.txt` with `dtparam=i2c_arm=on` (UPS HAT/INA219) and the PWM overlay for GPIO18/GPIO13.
