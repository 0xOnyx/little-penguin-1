# task 1

## compile and exec kernel

download form the source :
https://git.kernel.org/


https://git.kernel.org/pub/scm/linux/kernel/git/torvalds/linux.git/


git clone --depth 1 https://git.kernel.org/pub/scm/linux/kernel/git/torvalds/linux.git



• Build, install, and boot it. You may choose whatever kernel configuration you prefer, but you must enable CONFIG_LOCALVERSION_AUTO=y.
Turn In
• Kernel boot log file.
• Your .config file


install dependecie:
```bash
sudo apt update
sudo apt install build-essential libncurses-dev flex bison libssl-dev \
libelf-dev dwarves bc git

```


make olddefconfig


