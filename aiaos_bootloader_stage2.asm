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

    PAGE64_PAGE_SIZE    equ 0x1000     ; 4KB
    PAGE64_TAB_SIZE     equ 0x1000     ; Size of a page table (512 entries * 8 bytes/entry = 4KB)
    PAGE64_TAB_ENT_NUM  equ 512
    PAGE_SIZE_2MB       equ 0x200000   ; 2MB
    ENTRY_PRESENT_RW_PS equ 0x83       ; Present | Read/Write | Page Size (for 2MB pages)

    ; EBX = base address for page tables (e.g., 0x1000)
    mov edi, ebx
    xor eax, eax
    mov ecx, PAGE64_TAB_SIZE * 4 / 4   ; Clear 4 pages (PML4, PDPT, PD, and one PT)
    rep stosd                          ; Clears 0x1000, 0x2000, 0x3000, 0x4000

    ; Store base in esi for reuse
    mov esi, ebx ; esi = 0x1000 (PML4 base)

    ; --- Level 4: PML4 (Page Map Level 4) Table (at 0x1000) ---
    ; PML4[0] -> points to PDPT (at 0x2000)
    lea eax, [esi + PAGE64_TAB_SIZE + 0x3] ; Address of PDPT + Present/RW
    mov [esi], eax
    mov dword [esi + 4], 0                 ; Zero out upper 32 bits (PML4 entry is 64-bit)

    ; --- Level 3: PDPT (Page Directory Pointer Table) (at 0x2000) ---
    ; PDPT[0] -> points to PD (at 0x3000)
    lea eax, [esi + PAGE64_TAB_SIZE * 2 + 0x3] ; Address of PD + Present/RW
    mov [esi + PAGE64_TAB_SIZE], eax           ; This is PDPT[0]
    mov dword [esi + PAGE64_TAB_SIZE + 4], 0   ; Zero out upper 32 bits

    ; --- Level 2: PD (Page Directory) Table (at 0x3000) ---
    ; Now, populate the PD to map the necessary 2MB regions.
    ; Your kernel + stack occupy up to 0x219DFF.
    ; This means you need to map at least the 2MB page starting at 0x0
    ; AND the 2MB page starting at 0x200000.

    mov edi, esi ; edi = 0x1000 (PML4 base)

    ; Map the first 2MB page (0x000000 - 0x1FFFFF)
    ; PD entry index 0 (for virtual addresses 0x000000-0x1FFFFF)
    mov eax, 0x000000 | ENTRY_PRESENT_RW_PS ; Physical address 0 + flags
    mov [edi + PAGE64_TAB_SIZE * 2 + 0*8], eax ; PD[0]
    mov dword [edi + PAGE64_TAB_SIZE * 2 + 0*8 + 4], 0

    ; Map the second 2MB page (0x200000 - 0x3FFFFF)
    ; This is where your stack lives (0x19e00 to 0x219dff)
    ; PD entry index 1 (for virtual addresses 0x200000-0x3FFFFF)
    mov eax, 0x200000 | ENTRY_PRESENT_RW_PS ; Physical address 0x200000 + flags
    mov [edi + PAGE64_TAB_SIZE * 2 + 1*8], eax ; PD[1]
    mov dword [edi + PAGE64_TAB_SIZE * 2 + 1*8 + 4], 0

    ; You can extend this for as many 2MB pages as you have physical memory,
    ; up to a reasonable limit, or based on your E820 map.
    ; For a 16MB QEMU setup, you'd map:
    ; PD[0] -> 0x000000 - 0x1FFFFF
    ; PD[1] -> 0x200000 - 0x3FFFFF
    ; ...
    ; PD[7] -> 0xE00000 - 0xFFFFFF (Covers up to 16MB - 1 byte)

    ; Original E820 loop (simplified, as manual mapping is often safer initially)
    ; The original loop and identity_map_region_2mb might be redundant or problematic
    ; if not carefully implemented for 2MB pages that span regions.
    ; For now, let's keep the manual mapping for guaranteed coverage.
    ; If you truly want to use E820, you need to refine `identity_map_region_2mb`
    ; to iterate through 2MB aligned chunks within the E820 region.

    ; --- Removed the E820 mapping loop for clarity and to solve the immediate issue ---
    ; Keeping the E820 loop if you can fix identity_map_region_2mb
    ; However, the manual mapping for PD[0] and PD[1] directly addresses your crash.

    ; Optional: If you want to continue mapping more memory based on E820
    ; You'd need a robust `identity_map_region_2mb` that correctly handles
    ; partial 2MB page starts/ends for E820 regions.
    ; A simpler approach is to map full 2MB pages up to the maximum physical
    ; memory you expect to use.

    ; Example for mapping all of your 16MB QEMU memory with 2MB pages:
    ; mov eax, 0x0
    ; mov ecx, 8 ; Map 8 * 2MB = 16MB
    ; mov edi, esi + PAGE64_TAB_SIZE * 2 ; Base of PD table
    ; .map_2mb_pages_loop:
    ;    mov dword [edi], eax | ENTRY_PRESENT_RW_PS
    ;    mov dword [edi + 4], 0
    ;    add eax, PAGE_SIZE_2MB
    ;    add edi, 8 ; Next PD entry
    ;    loop .map_2mb_pages_loop

.done:
    popa
    ret


    [bits 64]
extern _stack_top ;  _stack_top_aligned comes from linker

start_long_mode:
    mov ebx, long_mode_msg
    call print_string64

    ;; (1) Load a basic Interrupt Descriptor Table (IDT)
    call setup_idt64
    lidt [idt64_pointer]

    ;; (2) Enable SSE/XMM support
    mov   rax, cr0
    and   rax, ~(1<<2)     ; clear EM
    mov   cr0, rax
    mov   rax, cr4
    or    rax, 1<<9        ; set OSFXSR
    mov   cr4, rax

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

stage2_msg: db "Hello from stage 2", 13, 10, 0
prot_mode_msg: db "Hello from protected mode", 0
long_mode_msg: db "Entered 64-bit mode", 0
