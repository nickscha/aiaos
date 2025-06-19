section .stage2

[bits 16]

    extern _e820_address

    mov bx, stage2_msg
    call print_string16

    ; query available physical memory
    ; we have 2 bios calls that give us various memory regions
    ; we are not going to handle non-contiguous memory so we're just taking the max out of all these
    ; regions and identity mapping up to that
    ; we keep track of the max in MEM_TOTAL

    ; E801 bios call gives
    ; - DX = amount of memory starting at 16 MiB in number of 64 KiB blocks
    ; - CX = amount of memory between 1MiB and 16MiB in KiB
    ; - carry flag set on error
    ; this only handles up to 64 * 64 KiB = 4 GiB of memory, but it works even on very old machines
    ; we don't really care about the lower 16 MiB other than the stuff that's already mapped
    mov ax,0xE801
    int 0x15
    xor dh,dh ; just in case something is in the upper bits
    shl edx,16 ; multiply by 64 KiB because it's 64 KiB blocks
    add edx,0x1000000 ; add 16 MiB because it's above the 16 MiB mark
    mov [MEM_TOTAL],edx

    ; E820 bios call queries available memory regions. this is for when we have > 4 GiB of ram
    ; - ebx = continuation value, starts at zero. resets to zero at end of list
    ; - region struct will be stored at es:di
    ; - ecx = size to which we truncate the struct. minimum 20 bytes
    ; - edx = the signature 'SMAP' encoded as a 32-bit integer
    ;
    ; returns:
    ; - eax = 'SMAP' for sanity checking
    ; - ecx = num of bytes actually written
    ; - carry flag set on error
    ;
    ; the smallest struct is:
    ;   uint64 base
    ;   uint64 length
    ;   uint32 type
    mov ax,cs
    mov es,ax ; make sure es segment is set up
    mov ds,ax

    mov di,_e820_address ; buffer for region struct. save a few bytes by not storing this in our boot sector
    xor ebx,ebx ; continuation value starts at zero

    query_mem_loop:
    mov ecx,20 ; 20 bytes, minimum struct size
    mov edx,'PAMS' ; SMAP reversed because of integer endian-ness
    mov eax,0xE820
    int 0x15
    jc query_mem_done ; error
    cmp eax,'PAMS'
    jne query_mem_done ; invalid map
    ; check type. 1 is available. if we don't ignore reserved regions we will map too much ram on
    ; < 4GiB systems because bios reports some reserved stuff up to 4 GiB
    cmp byte [di+16],1
    jne query_mem_next

    ; check if base + length is higher than MEM_TOTAL
    ; since we're still in 32-bit mode we need to do 2 32-bit cascade subtractions and check for carry
    mov ecx,[di]          ; ecx:edx = base
    mov edx,[di+4]
    add ecx,[di+8]        ; ecx:edx += length
    adc edx,[di+12]
    mov eax,[MEM_TOTAL]   ; eax:ebp = MEM_TOTAL
    mov ebp,[MEM_TOTAL+4]
    sub eax,ecx           ; eax:ebp (MEM_TOTAL) -= ecx:edx (base + length)
    sbb ebp,edx
    jnc query_mem_next
    ; carry means that eax:ebp (MEM_TOTAL) > ecx:edx (base + len) so we save the new max size
    mov [MEM_TOTAL],ecx
    mov [MEM_TOTAL+4],edx

    query_mem_next:
    ; if ebx resets to zero, we're at the end of the list
    test ebx,ebx
    jnz query_mem_loop
    query_mem_done:

    ; Can't have interrupts during the switch
    cli                    

    ; enable A20 line, this enables bit number 20 in the address
    in al,0x92
    or al,2
    out 0x92,al

    ; ds is uninitialized. lgdt uses ds as its segment so let's init it
    xor ax,ax
    mov ds,ax

    ;; Load GDT and switch to protected mode
    lgdt [gdt32_pseudo_descriptor]

    mov eax,0x11 ; paging disabled, protection bit enabled. bit4, the extension type is always 1
    mov cr0,eax

    ;; The far jump into the code segment from the new GDT flushes
    ;; the CPU pipeline removing any 16-bit decoded instructions
    ;; and updates the cs register with the new code segment.
    jmp CODE_SEG32:start_prot_mode

[bits 32]

extern _paging_structures_start

start_prot_mode:
    ;; Old segments are now meaningless
    mov ax, DATA_SEG32
    mov ds, ax
    mov ss, ax
    mov es, ax
    mov fs, ax
    mov gs, ax

    mov ebx, prot_mode_msg
    call print_string32

    ; set PAE (enables >32bit addresses through paging) and PGE (caches frequently used pages)
    mov eax,0xA0
    mov cr4,eax

    ; set the base address of the paging tables
    mov edi,_paging_structures_start

    mov ecx,[MEM_TOTAL]
    mov edx,[MEM_TOTAL+4]

    ; num of 2MiB pages, rounded up
    add ecx,0x1FFFFF ; round up by adding 2MiB - 1
    adc edx,0
    mov cl,21        ; divide by 2MiB by shifting right by 21 bits (2<<21 = 2 MiB)
    shrd ecx,edx,cl  ; this is used to do a 64-bit shift. it shifts cl bits into ecx from edx

    ; ecx = number of pages that will actually be mapped (not rounded). rest is zeroed
    ; edx = number of pages, rounded to 512 entries
    mov edx,ecx

    ; round up to chunks of 512 entries to maintain the 4KiB alignment
    add edx,0x1FF
    and edx,~0x1FF

    ; PD: 2MiB pages identity-mapped to actual memory. eax:ebx used to track address
    mov ebp,edi ; store PD pointer for later
    push ecx
    push edx
    mov eax,0x83 ; R/W, P, PS
    xor ebx,ebx
    pdloop:
    stosd
    xchg eax,ebx
    stosd
    xchg eax,ebx
    add eax,0x200000
    adc ebx,0
    dec edx
    loop pdloop
    mov ecx,edx
    call zerorest
    pop edx
    pop ecx

    xor ebx,ebx ; using ebx as a zero value register to write zeros in do_pages
    call do_pages ; PDP: 1GiB pages
    mov cr3,edi   ; set PML4 pointer
    call do_pages ; PML4: 512GiB pages

    ; set LME bit (long mode enable) in the IA32_EFER machine specific register
    ; MSRs are 64-bit wide and are written/read to/from eax:edx
    mov ecx,0xC0000080 ; this is the register number for EFER
    mov eax,0x00000100 ; LME bit set
    xor edx,edx ; other bits zero
    wrmsr

    ; enable paging
    mov eax,cr0
    bts eax,31
    mov cr0,eax

    ;; New GDT has the 64-bit segment flag set. This makes the CPU switch from
    ;; IA-32e compatibility mode to 64-bit mode.
    lgdt [gdt64_pseudo_descriptor]

    jmp CODE_SEG64:start_long_mode

end:
    hlt
    jmp end

; write ecx*2 zero dwords at edi
zerorest:
xor eax,eax
shl ecx,1
rep stosd
ret

; set up page table entries that point to the next table
; inputs:
;   edi = pointer to 4KiB aligned memory where the table will be written
;   ebp = pointer to the start of the previous table. will be the starting address for the entries
;   ecx = number of entries in the prev table (not rounded up). other entries are zeroed out
;   edx = number of entries in the prev table (rounded up to 512)
;   ebx = must be zero
; outputs:
;   ebp = pointer to the start of this table
;   edi = pointer to 4Kib aligned memory after the table
;   ecx = number of entries in this table (not rounded up)
;   edx = number of entries in this table (rounded up to 512)
do_pages:
; num of pages in this table, rounded up
shr edx,9
add edx,0x1FF
and edx,~0x1FF

; num of pages in this table, not rounded
add ecx,0x1FF
shr ecx,9

push ecx
push edx
mov eax,ebp ; start at last table
mov ebp,edi ; save address of this table for later
or eax,3 ; R/W, P
pages_loop:
stosd
xchg eax,ebx
stosd ; zero
xchg eax,ebx
add eax,0x1000 ; step by 512*8 bytes (one table)
dec edx
loop pages_loop
mov ecx,edx
call zerorest
pop edx
pop ecx
ret

[bits 64]
extern _stack_bottom ; _stack_bottom comes from aiaos_linker.ld
extern _stack_top    ; _stack_top comes from aiaos_linker.ld

start_long_mode:
    mov ebx, long_mode_msg
    call print_string64

    ;; (1) Load a basic Interrupt Descriptor Table (IDT)
    call setup_idt64
    lidt [idt64_pointer]

    ;; (2) Enable SSE/XMM/AVX support
    mov   rax, cr0
    and   rax, ~(1<<2)     ; clear EM
    mov   cr0, rax
    mov   rax, cr4
    or    rax, 1<<9        ; set OSFXSR
    mov   cr4, rax

    ;; Check if CPU supports AVX and OSXSAVE using CPUID
    mov     eax, 1
    cpuid
    test    ecx, (1 << 27)       ; Check OSXSAVE
    jz      .no_avx              ; Skip if not supported
    test    ecx, (1 << 28)       ; Check AVX support
    jz      .no_avx              ; Skip if not supported

    ;; Enable OSXSAVE in CR4
    mov     rax, cr4
    or      rax, (1 << 18)       ; Set OSXSAVE
    mov     cr4, rax

    ;; Enable XMM (bit 1) and YMM (bit 2) in XCR0 using XSETBV
    mov     ecx, 0               ; XCR0
    xor     edx, edx
    mov     eax, (1 << 1) | (1 << 2) ; XMM + YMM
    xsetbv

.no_avx:

    ;; (3) Set Stack Address
    mov   rsp, _stack_top ; Use the aligned stack top defined by the linker
    mov   rbp, rsp

    ;; (4) Call kernel
    extern _start_kernel
    call _start_kernel

end64:
    hlt
    jmp end64

%include "aiaos_bootloader_stage2_print.asm"
%include "aiaos_bootloader_stage2_gdt32.asm"
%include "aiaos_bootloader_stage2_gdt64.asm"
%include "aiaos_bootloader_stage2_idt64.asm"

stage2_msg: db "AIAOS stage 2", 13, 10, 0
prot_mode_msg: db "AIAOS protected mode", 0
long_mode_msg: db "AIAOS entering 64-bit mode", 0

global MEM_TOTAL
MEM_TOTAL: dq 0
HEXCHARS: db '0123456789ABCDEF'
