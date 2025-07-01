section .boot_sector
global __start

[bits 16]

__start:
    mov si, disk_address_packet
    mov ah, 0x42 ; BIOS "extended read" function
    mov dl, 0x80 ; Drive number
    int 0x13     ; BIOS disk services
    jc error_reading_disk

ignore_disk_read_error:
    SND_STAGE_ADDR equ (BOOT_LOAD_ADDR + SECTOR_SIZE)
    jmp 0:SND_STAGE_ADDR

error_reading_disk:
    ;; We accept reading fewer sectors than requested
    cmp word [dap_sectors_num], READ_SECTORS_NUM
    jle ignore_disk_read_error

end:
    hlt
    jmp end

    align 4
disk_address_packet:
    db 0x10                           ; Size of packet
    db 0                              ; Reserved, always 0
dap_sectors_num:
    dw READ_SECTORS_NUM               ; Number of sectors read
    dd (BOOT_LOAD_ADDR + SECTOR_SIZE) ; Destination address
    dq 1                              ; Sector to start at (0 is the boot sector)

READ_SECTORS_NUM equ 64
BOOT_LOAD_ADDR   equ 0x7c00
SECTOR_SIZE      equ 512
