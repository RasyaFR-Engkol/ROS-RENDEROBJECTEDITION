.CODE

PUBLIC RtlASMMemcpy
PUBLIC RtlASMMemset

; =========================================================================
; void* RtlASMMemcpy(void* dest, const void* src, size_t count)
;
; Parameters (Windows x64 ABI):
; RCX = dest
; RDX = src
; R8  = count
;
; Return:
; RAX = dest
; =========================================================================

RtlASMMemcpy PROC
    ; Note: Di x64 kita gak pake 'FRAME' atau 'USES' dulu biar simple (Leaf Function)
    ; Kita manual push/pop register non-volatile (RSI, RDI)
    
    push rdi
    push rsi

    mov rdi, rcx        ; Dest
    mov rsi, rdx        ; Source
    mov rax, rcx        ; Simpan return value (Dest)

    mov rcx, r8         ; Count
    shr rcx, 3          ; Count / 8

    cld                 ; Clear Direction Flag (Defensive coding)
    rep movsq           ; Copy 8 bytes at a time

    mov rcx, r8         ; Ambil count lagi
    and rcx, 7          ; Count % 8

    rep movsb           ; Copy sisa byte

    pop rsi
    pop rdi
    ret
RtlASMMemcpy ENDP

; =========================================================================
; VOID* RtlASMMemset(void* dest, int c, size_t count)
;
; Parameters:
; RCX = dest
; RDX = val (c)
; R8  = count
; =========================================================================

RtlASMMemset PROC
    push rdi

    mov rdi, rcx        ; Destination
    mov rax, rdx        ; Value (c)
    mov r9, rcx         ; Simpan dest buat return value

    ; Trik memperbanyak byte ke seluruh register RAX:
    movzx eax, al       ; Pastikan cuma byte bawah yg ada
    mov rdx, 0101010101010101h ; Pake suffix 'h' buat hex di MASM
    imul rax, rdx       ; Sekarang RAX isinya pattern penuh

    ; Logic 8 byte
    mov rcx, r8
    shr rcx, 3
    cld
    rep stosq           ; Isi memori [RDI] dengan RAX

    ; Sisa byte
    mov rcx, r8
    and rcx, 7
    rep stosb

    mov rax, r9         ; Return original dest
    pop rdi
    ret
RtlASMMemset ENDP

END