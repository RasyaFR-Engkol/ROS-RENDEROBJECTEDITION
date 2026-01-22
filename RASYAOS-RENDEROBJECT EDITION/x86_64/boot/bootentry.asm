.CODE

EXTERN RKXKernelInitializeAndSetupCPU:PROC

RKXKernelEntry PROC
	push rbp
	mov rbp, rsp

	sub rsp, 20h

	call RKXKernelInitializeAndSetupCPU

DeadLoop:
    cli
    hlt
    jmp DeadLoop
RKXKernelEntry ENDP
PUBLIC RKXKernelEntry

END
