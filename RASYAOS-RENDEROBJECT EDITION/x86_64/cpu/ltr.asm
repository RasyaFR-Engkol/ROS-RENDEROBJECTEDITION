.code

; ==========================================================
; VOID RKI_LoadTr(UINT16 TrSelector);
; Arg1 (RCX) = TrSelector
; ==========================================================
RKILoadTr PROC
    ltr cx          ; Load Task Register dengan nilai di CX (16-bit dari RCX)
    ret
RKILoadTr ENDP

; ==========================================================
; VOID RKI_ReloadSegments(UINT16 CodeSeg, UINT16 DataSeg);
; Arg1 (RCX) = CodeSeg
; Arg2 (RDX) = DataSeg
; ==========================================================
RKIReloadSegments PROC
    ; Load Data Segments
    mov ds, dx
    mov es, dx
    mov ss, dx
    mov fs, dx
    mov gs, dx  ; Hati-hati, GS base nanti ditimpa MSR, tapi init awal gapapa
    
    ; Far Return trick buat reload CS (Code Segment)
    ; Kita push CS baru dan alamat return, lalu RETFQ (Return Far)
    
    push rcx                ; Push Code Segment Selector
    lea rax, [return_here]  ; Ambil alamat label di bawah
    push rax                ; Push Return Address
    retfq                   ; Far Return (Pop IP, Pop CS)

return_here:
    ret
RKIReloadSegments ENDP

END