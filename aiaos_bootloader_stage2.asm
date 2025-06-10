    section .stage2

    [bits 16]

    mov bx, stage2_msg
    call print_string16

    memmap_entry_size    equ 24
    memmap_max_entries   equ 32
    memmap_entry_count   equ 0x6000        ; safer separate location for count
    memmap_addr          equ 0x6020        ; entries start here        

    call get_memory_map

    ;; Load GDT and switch to protected mode
    cli                     ; Can't have interrupts during the switch
    lgdt [gdt32_pseudo_descriptor]

    ;; Setting cr0.PE (bit 0) enables protected mode
    mov eax, cr0
    or eax, 1
    mov cr0, eax

    ;; The far jump into the code segment from the new GDT flushes
    ;; the CPU pipeline removing any 16-bit decoded instructions
    ;; and updates the cs register with the new code segment.
    jmp CODE_SEG32:start_prot_mode

   get_memory_map:
        push ax
        push bx
        push cx
        push dx
        push si
        push di
        push bp
        push ds
        push es

        ;; Clear memory map region
        mov ax, 0
        mov es, ax
        mov di, memmap_addr
        mov cx, (memmap_entry_size * memmap_max_entries) / 2
        xor ax, ax
        rep stosw

        ;; Clear count
        mov word [memmap_entry_count], 0

        xor ebx, ebx
        xor si, si

    .get_entry:
        ;; Compute ES:DI = memmap_addr + si * 24
        mov ax, 0
        mov es, ax
        mov di, memmap_addr
        mov cx, si
        mov ax, memmap_entry_size
        mul cx
        add di, ax

        ;; INT 15h E820
        mov eax, 0xE820
        mov edx, 0x534D4150
        mov ecx, memmap_entry_size
        int 0x15
        jc .done
        cmp eax, 0x534D4150
        jne .done

        inc si
        mov [memmap_entry_count], si
        test ebx, ebx
        jnz .get_entry

    .done:
        ;; Print memory map count at top of VGA
        mov ax, 0xb800
        mov es, ax
        mov si, memmap_entry_count
        mov di, 160 * 5        ; row 5

        mov al, '0'
        add al, byte [memmap_entry_count]
        mov ah, 0x0F
        mov [es:di], ax

        pop es
        pop ds
        pop bp
        pop di
        pop si
        pop dx
        pop cx
        pop bx
        pop ax
        ret

    [bits 32]

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

    ;; Build 4 level page table and switch to long mode   
    mov ebx, 0x1000
    call build_page_table
    mov cr3, ebx            ; MMU finds the PML4 table in cr3

    ;; Enable Physical Address Extension (PAE)
    mov eax, cr4
    or eax, 1 << 5
    mov cr4, eax

    ;; The EFER (Extended Feature Enable Register) MSR (Model-Specific Register) contains fields
    ;; related to IA-32e mode operation. Bit 8 if this MSR is the LME (long mode enable) flag
    ;; that enables IA-32e operation.
    mov ecx, 0xc0000080
    rdmsr
    or eax, 1 << 8
    wrmsr

    ;; Enable paging (PG flag in cr0, bit 31)
    mov eax, cr0
    or eax, 1 << 31
    mov cr0, eax

    ;; New GDT has the 64-bit segment flag set. This makes the CPU switch from
    ;; IA-32e compatibility mode to 64-bit mode.
    lgdt [gdt64_pseudo_descriptor]

    jmp CODE_SEG64:start_long_mode

end:
    hlt
    jmp end

    ;; Builds a 4 level page table starting at the address that's passed in ebx.
build_page_table:
    pusha

    PAGE64_PAGE_SIZE    equ 0x1000
    PAGE64_TAB_SIZE     equ 0x1000
    PAGE64_TAB_ENT_NUM  equ 512
    PAGE_SIZE_2MB       equ 0x200000
    ENTRY_PRESENT_RW_PS equ 0x83       ; P | RW | PS

    ; EBX = base address for page tables
    mov edi, ebx
    xor eax, eax
    mov ecx, PAGE64_TAB_SIZE * 4 / 4   ; Clear 4 pages
    rep stosd

    ; Store base in esi for reuse
    mov esi, ebx

    ; PML4 -> PDP
    lea eax, [esi + PAGE64_TAB_SIZE + 0x3]
    mov [esi], eax                      ; Write lower 32 bits
    mov dword [esi + 4], 0              ; Zero out upper 32 bits

    ; PDP -> PD
    lea eax, [esi + PAGE64_TAB_SIZE * 2 + 0x3]
    mov [esi + PAGE64_TAB_SIZE], eax    ; Write lower 32 bits
    mov dword [esi + PAGE64_TAB_SIZE + 4], 0 ; Zero out upper 32 bits

    ; Base address of PD
    lea edi, [esi + PAGE64_TAB_SIZE * 2]

    ; Start of memory map
    mov ebp, memmap_addr
    mov ecx, [memmap_entry_count]
    xor esi, esi                        ; memory map index

.map_loop:
    cmp esi, ecx
    jge .done

    ; Compute address of memmap entry: entry_ptr = ebp + esi * 24
    mov edx, esi
    imul edx, memmap_entry_size
    add edx, ebp                        ; edx = address of current memmap entry

    ; Load base (low and high)
    mov eax, [edx]                      ; base[31:0]
    mov ebx, [edx + 4]                  ; base[63:32]

    ; Skip entries above 4 GiB
    test ebx, ebx
    jnz .next

    push eax                            ; Save base[31:0]

    ; Load length (low and high)
    mov ebx, [edx + 8]                  ; length[31:0]
    mov ecx, [edx + 12]                 ; length[63:32]

    ; Only map type 1 (usable)
    mov dx, [edx + 16]
    cmp dx, 1
    jne .skip

    ; Call identity map
    pop eax                             ; Restore base
    push esi
    call identity_map_region_2mb
    pop esi
    jmp .next

.skip:
    pop eax                             ; Discard base

.next:
    inc esi
    jmp .map_loop

.done:
    popa
    ret

identity_map_region_2mb:
    push eax
    push ebx
    push ecx
    push edx
    push edi

    ; Align base address down to 2MB
    mov edx, PAGE_SIZE_2MB - 1
    not edx
    and eax, edx        ; aligned_base = base & ~(2MB-1)

    ; Align length up to cover full pages
    add ebx, PAGE_SIZE_2MB - 1
    and ebx, edx        ; aligned_length = (length + 2MB - 1) & ~(2MB-1)

.loop:
    cmp ebx, 0
    jz .done

    ; Calculate PD index = base >> 21
    mov edx, eax
    shr edx, 21
    shl edx, 3           ; offset = index * 8 (64-bit entry)

    ; Write entry: base | flags
    mov dword [edi + edx], eax      ; Write the 2MB-aligned physical address
    or  dword [edi + edx], ENTRY_PRESENT_RW_PS ; OR the flags into the lower 32 bits
    mov dword [edi + edx + 4], 0    ; Zero out the upper 32 bits of the entry

    ; Advance to next 2MB page
    add eax, PAGE_SIZE_2MB
    sub ebx, PAGE_SIZE_2MB
    jmp .loop

.done:
    pop edi
    pop edx
    pop ecx
    pop ebx
    pop eax
    ret

    [bits 64]

start_long_mode:
    mov ebx, long_mode_msg
    call print_string64

    ;; (2) Enable SSE/XMM support
    mov   rax, cr0
    and   rax, ~(1<<2)     ; clear EM
    mov   cr0, rax
    mov   rax, cr4
    or    rax, 1<<9        ; set OSFXSR
    mov   cr4, rax

    ;; (3) Set Stack Address
    mov   rsp, 0x200000    ; Set stack to a 16-byte aligned address
    mov   rbp, rsp

    extern _start_kernel
    call _start_kernel

end64:
    hlt
    jmp end64

%include "aiaos_bootloader_stage2_print.asm"
%include "aiaos_bootloader_stage2_gdt32.asm"
%include "aiaos_bootloader_stage2_gdt64.asm"

stage2_msg: db "Hello from stage 2", 13, 10, 0
prot_mode_msg: db "Hello from protected mode", 0
long_mode_msg: db "Entered 64-bit mode", 0
