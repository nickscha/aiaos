@echo OFF

REM CFLAGS with strict warnings and pedantic mode
set DEF_COMPILER_FLAGS=-O2 -s -m64 -march=x86-64 -mno-red-zone -mno-stack-arg-probe -pedantic -std=c89 -nodefaultlibs -nostdlib -nostdinc ^
-ffreestanding -fno-builtin -fno-exceptions -fno-asynchronous-unwind-tables ^
-Wall -Wextra -Werror -Wvla -Wconversion -Wdouble-promotion -Wsign-conversion -Wuninitialized -Winit-self ^
-Wunused -Wunused-function -Wunused-macros -Wunused-parameter -Wunused-value -Wunused-variable -Wunused-local-typedefs

REM Compile Bootloader Stages
nasm -f elf64 aiaos_bootloader_stage1.asm -o aiaos_bootloader_stage1.s.o
nasm -f elf64 aiaos_bootloader_stage2.asm -o aiaos_bootloader_stage2.s.o

REM Compile Kernel and Link Bootloader Stages
x86_64-elf-gcc %DEF_COMPILER_FLAGS% -c aiaos_kernel.c -o aiaos_kernel.c.o
x86_64-elf-ld -T aiaos_linker.ld -o aiaos_linker.o aiaos_bootloader_stage1.s.o aiaos_bootloader_stage2.s.o aiaos_kernel.c.o
x86_64-elf-objcopy -O binary aiaos_linker.o aiaos.img

REM Run QEMU
qemu-system-x86_64 -m 1g -no-reboot -drive file=aiaos.img,format=raw,index=0,media=disk
