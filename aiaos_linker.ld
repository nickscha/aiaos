# aiaos_linker.ld

MEMORY
{
    boot_sector (rwx)  : ORIGIN = 0x007c00, LENGTH = 0x000200
    stage2 (rwx)       : ORIGIN = 0x007e00, LENGTH = 0x002000
    kernel (rwx)       : ORIGIN = 0x009e00, LENGTH = 0x010000

    /* 2MB stack memory placed after kernel. (0x9e00 + 0x10000 = 0x19e00)  (end region: 0x19e00 + 0x200000 = 0x219e00) */
    stack_memory (rwx) : ORIGIN = 0x100000, LENGTH = 0x200000
}

ENTRY(__start)
SECTIONS
{
    _e820_entry_count        = 0x006000;
    _e820_address            = 0x006020;
    _paging_structures_start = 0x100000;

    .boot_sector :             { *(.boot_sector); }      > boot_sector
    .bootsign (0x7c00 + 510) : { BYTE(0x55) BYTE(0xaa) } > boot_sector
    .stage2 :                  { *(.stage2); }           > stage2
    .text :                    { *(.text); }             > kernel
    .data :                    { *(.data); }             > kernel
    .rodata :                  { *(.rodata); }           > kernel
    .bss :                     { *(.bss) *(COMMON) }     > kernel

    /* We place stack section at the end of the `stack_memory` region because stacks grow downwards. */
    .stack : AT(ORIGIN(stack_memory) + LENGTH(stack_memory) - 1)
    {
        _stack_bottom = ORIGIN(stack_memory);            /* _stack_bottom points to the very beginning of the allocated stack memory  */
        . = ORIGIN(stack_memory) + LENGTH(stack_memory); /* . (location counter) is set to the end of the stack region (top of stack) */
        _stack_top = .;                                  /* _stack_top points to the address where RSP should be initialized          */
    } > stack_memory

    /DISCARD/ : {
        *(.eh_frame)
        *(.note .note.*)
    }
}
