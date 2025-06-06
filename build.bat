@echo OFF

REM set DEF_COMPILER_FLAGS=-O2 -s -std=c89 -ffreestanding -m64 -mno-red-zone -fno-builtin -nostdinc -Wall -Wextra -Werror
set DEF_COMPILER_FLAGS=-O2 -s -m64 -mno-red-zone -pedantic -std=c89 -nodefaultlibs -nostdlib -nostdinc ^
-ffreestanding -fno-builtin -fno-exceptions -fno-asynchronous-unwind-tables ^
-Wall -Wextra -Werror -Wvla -Wconversion -Wdouble-promotion -Wsign-conversion -Wuninitialized -Winit-self ^
-Wunused -Wunused-function -Wunused-macros -Wunused-parameter -Wunused-value -Wunused-variable -Wunused-local-typedefs

REM Compile Bootloader Stages
nasm -f elf64 aiaos_bootloader_stage1.asm -o aiaos_bootloader_stage1.s.o
nasm -f elf64 aiaos_bootloader_stage2.asm -o aiaos_bootloader_stage2.s.o

REM Compile Kernel and Link Bootloader Stages
x86_64-elf-gcc %DEF_COMPILER_FLAGS% -c aiaos_kernel.c -o aiaos_kernel.c.o
x86_64-elf-ld -T aiaos_linker.ld -o linked.o aiaos_bootloader_stage1.s.o aiaos_kernel.c.o aiaos_bootloader_stage2.s.o
x86_64-elf-objcopy -O binary linked.o aiaos.img

REM Run QEMU
qemu-system-x86_64 -no-reboot -drive file=aiaos.img,format=raw,index=0,media=disk