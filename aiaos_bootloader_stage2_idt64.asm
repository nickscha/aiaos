; Simple default interrupt handler
isr_stub:
    ; For now, just print a character and halt
    mov dword [0xb8000], 0x0f460f46 ; 'FF' in red on the top-left of the screen
    cli
    hlt

; Sets up 256 entries in the IDT to point to our stub
setup_idt64:
    mov rdi, idt64_table            ; RDI = address of the IDT
    mov rcx, 256                    ; 256 interrupt vectors
    mov rax, isr_stub               ; RAX = address of our handler
.loop:
    ;; Create the 16-byte IDT descriptor
    mov word [rdi], ax              ; Set handler offset bits 0-15
    mov word [rdi + 2], CODE_SEG64  ; Kernel code segment selector (0x08)
    mov byte [rdi + 4], 0           ; IST (Interrupt Stack Table)
    mov byte [rdi + 5], 0x8E        ; Type: 64-bit Interrupt Gate, Present, DPL=0
    shr rax, 16
    mov word [rdi + 6], ax          ; Set handler offset bits 16-31
    shr rax, 16
    mov dword [rdi + 8], eax        ; Set handler offset bits 32-63
    mov dword [rdi + 12], 0         ; Reserved

    add rdi, 16                     ; Move to the next IDT entry
    mov rax, isr_stub               ; Restore handler address in RAX
    loop .loop

    ret

section .bss
align 16
idt64_table:
    resb 256 * 16                   ; Reserve space for 256 IDT entries

section .rodata
idt64_pointer:
    dw 256 * 16 - 1                 ; IDT limit (size in bytes - 1)
    dq idt64_table                  ; IDT base address
