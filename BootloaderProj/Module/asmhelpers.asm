; Menggunakan Syntax MASM untuk MSVC

.code

; void EFIAPI REFIXHandoverToKernel(
;     IN UINT64 EntryPoint,       (RCX)
;     IN UINT64 Pml4Physical,     (RDX)
;     IN UINT64 KernelStackInfo,  (R8)  <-- Stack Baru
;     IN VOID* LoaderBlock       (R9)  <-- Parameter buat Kernel
; );

REFIXHandoverToKernel PROC
    ; 1. Matikan Interupsi (Wajib saat ganti stack/page table)
    cli

    ; 2. Simpan EntryPoint (RCX) ke register sementara
    ; Karena RCX nanti mau kita pakai buat ngoper argumen ke Kernel
    mov rax, rcx        ; RAX = EntryPoint Kernel

    ; 3. Ganti Panggung (Page Table)
    ; PERINGATAN: Pastikan Page Table baru ini punya 'Identity Map'
    ; untuk kode assembly INI. Kalau tidak, crash di instruksi berikutnya.
    mov cr3, rdx

    ; 4. Ganti Stack (Pindah ke Stack Kernel)
    ; R8 isinya alamat Stack Base (Top of Stack)
    mov rsp, r8

    ; 5. Persiapan Argumen Kernel
    ; Kernelmu (C function) pasti signaturenya: void KernelMain(LoaderBlock* block);
    ; Di x64, argumen ke-1 itu harus masuk RCX.
    ; Tadi LoaderBlock ada di R9. Kita pindah ke RCX.
    mov rcx, r9         

    ; 6. Setup Shadow Space (Opsional tapi Good Practice buat MSVC x64)
    ; MSVC function expect ada ruang 32 bytes (0x20) di atas stack return.
    sub rsp, 20h

    ; 7. Loncat ke Kernel!
    ; Kita call RAX (yang isinya EntryPoint tadi)
    call rax 
    
    ; 8. Harusnya gak bakal balik kesini (Dead End)
    hlt
REFIXHandoverToKernel ENDP
PUBLIC REFIXHandoverToKernel

; VOID REFIXAsmWriteCr3(UINT64 Value);

REFIXAsmWriteCr3 PROC
    mov cr3, rcx
    ret
REFIXAsmWriteCr3 ENDP
PUBLIC REFIXAsmWriteCr3

END