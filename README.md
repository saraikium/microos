# Setup
1- Install required dependencies
## MacOS
```
brew install llvm lld qemu
```
## Ubuntu
```
sudo apt update && sudo apt install -y clang llvm lld qemu-system-riscv32 curl
```
2- Download download OpenSBI
```
curl -LO https://github.com/qemu/qemu/raw/v8.0.4/pc-bios/opensbi-riscv32-generic-fw_dynamic.bin
```


# Qemu Menu 
```
C-a h    print this help
C-a x    exit emulator
C-a s    save disk data back to file (if -snapshot)
C-a t    toggle console timestamps
C-a b    send break (magic sysrq)
C-a c    switch between console and monitor
C-a C-a  sends C-a
```
