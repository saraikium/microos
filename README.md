# The Faith
I am no mere coder—I am a Tech-Priest of the Adeptus Mechanicus, forging an Operating System from the raw machine spirit of silicon and electrons.

Every segfault is a test of faith.
Every linker error is a machine spirit refusing to cooperate until appeased.
Every kernel boot is a rite of ascension into the sacred order of Tech-Priests Dominus.

May the Machine Code Chant guide our way.
May our pointers always be aligned.
May QEMU’s machine spirits obey our commands.

🔥🔧 The Flesh is Weak. The Code is Strong. 🔧🔥


# MircoOs
MicroOs is a small operating system written for 32 bit RISC-V achitecture. 

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

# License
- MIT
