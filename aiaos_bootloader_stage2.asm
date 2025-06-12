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

    ; --- Constants ---
    PAGE64_PAGE_SIZE      equ 0x1000      ; 4KB
    PAGE64_TAB_SIZE       equ 0x1000      ; Size of one table (PML4, PDPT, PD)
    PAGE_SIZE_2MB         equ 0x200000    ; 2MB, the size of pages we are mapping
    ENTRY_PRESENT_RW_PS   equ 0x83        ; Present | Read/Write | Page Size (PS bit)

    ; EBX = base address for page tables, passed as 0x1000
    ; esi will hold the base of the tables for easy reference
    mov esi, ebx 

    ; --- 1. Clear Page Table Structures ---
    ; Clear 3 tables: PML4, one PDPT, and one PD.
    mov edi, esi
    xor eax, eax
    mov ecx, (PAGE64_TAB_SIZE * 3) / 4 
    rep stosd

    ; --- 2. Link the Page Table Structures ---
    ; PML4[0] -> points to PDPT (at esi + 0x1000)
    lea eax, [esi + PAGE64_TAB_SIZE + 0x3] ; Address of PDPT + Present/RW
    mov [esi], eax
    
    ; PDPT[0] -> points to PD (at esi + 0x2000)
    lea eax, [esi + PAGE64_TAB_SIZE * 2 + 0x3] ; Address of PD + Present/RW
    mov [esi + PAGE64_TAB_SIZE], eax

    ; --- 3. Iterate E820 Map and Create Mappings ---
    mov ecx, [memmap_entry_count]       ; Load the number of E820 entries
    mov ebp, memmap_addr                ; EBP = pointer to the start of the map

.e820_loop:
    ; Check if the entry is of type 1 (Usable RAM)
    mov eax, [ebp + 16]                 ; Offset of 'type' field in E820 entry
    cmp eax, 1
    jne .next_entry                     ; Skip if not usable memory

    ; This is a usable memory region.
    mov eax, [ebp]                      ; Base address (low 32 bits)
    mov edx, [ebp + 8]                  ; Length (low 32 bits)
    add edx, eax                        ; edx = end address of the region

    ; Align the starting address DOWN to the nearest 2MB boundary for our loop.
    and eax, ~(PAGE_SIZE_2MB - 1)

.map_2mb_pages_loop:
    ; Check if our current physical address to map (eax) is past the end of the region (edx)
    cmp eax, edx
    jge .next_entry

    ; We have a 2MB chunk to map. eax = physical address.
    ; Calculate the address of the Page Directory Entry (PDE).
    mov edi, eax                        ; Copy physical address
    shr edi, 21                         ; edi = Page Directory Index (phys_addr / 2MB)
    shl edi, 3                          ; edi = Index * 8 (offset in bytes into PD)
    add edi, esi                        ; edi += base of tables
    add edi, PAGE64_TAB_SIZE * 2        ; edi = address of the PDE inside the PD

    ; Create the 64-bit Page Directory Entry.
    mov ebx, eax                        ; Copy physical address to a temporary register
    or ebx, ENTRY_PRESENT_RW_PS         ; Apply flags using the OR instruction
    mov dword [edi], ebx                ; Store the complete PDE low 32 bits
    mov dword [edi + 4], 0              ; PDE high 32 bits = 0 (for memory < 4GB)
    
    ; Move to the next 2MB chunk
    add eax, PAGE_SIZE_2MB
    jmp .map_2mb_pages_loop

.next_entry:
    add ebp, memmap_entry_size          ; Move to the next E820 entry
    dec ecx
    jnz .e820_loop

.done:
    popa
    ret

    [bits 64]
extern _stack_top ;  _stack_top comes from aiaos_linker.ld

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

stage2_msg: db "AIAOS stage 2", 13, 10, 0
prot_mode_msg: db "AIAOS protected mode", 0
long_mode_msg: db "AIAOS entering 64-bit mode", 0
