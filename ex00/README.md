# Exercise 1 — Compile and Boot a Custom Linux Kernel

## Objective

Download the Linux kernel source, compile it with a custom configuration, install it, and boot it.

**Required:** `CONFIG_LOCALVERSION_AUTO=y` must be enabled.

## Deliverables

- `boot.log` — kernel boot log
- `.config` — kernel configuration file used for the build

## Dependencies

```bash
sudo apt update
sudo apt install build-essential libncurses-dev flex bison libssl-dev \
    libelf-dev dwarves bc git
```

## Steps

### 1. Clone the kernel source

```bash
git clone --depth 1 https://git.kernel.org/pub/scm/linux/kernel/git/torvalds/linux.git
cd linux
```

### 2. Configure

Start from the current running kernel's config and apply defaults for any new options:

```bash
cp /boot/config-$(uname -r) .config
make olddefconfig
```

Enable the required option:

```bash
scripts/config --enable CONFIG_LOCALVERSION_AUTO
scripts/config --set-str CONFIG_LOCALVERSION "-jerdos-s"
```

Verify:

```bash
grep CONFIG_LOCALVERSION_AUTO .config
# Expected: CONFIG_LOCALVERSION_AUTO=y
```

### 3. Build

```bash
make -j$(nproc)
```

### 4. Install modules and kernel

```bash
sudo make modules_install
sudo make install
```

### 5. Reboot

```bash
sudo reboot
```

After reboot, verify the new kernel is running:

```bash
uname -r
```

### 6. Capture the boot log

```bash
dmesg > boot.log
```

## Notes

- `CONFIG_LOCALVERSION_AUTO=y` appends a version string derived from git to the kernel release name (visible in `uname -r`).
- Build time varies — expect 30–90 minutes depending on hardware.
