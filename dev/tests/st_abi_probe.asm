; SPDX-License-Identifier: MIT
; ABI probe for the Windows x64 st context switch.
;
; Calls st_usleep() once (which switches away and back) with every Windows x64
; non-volatile register loaded with a sentinel. If md_win64.asm forgets any
; register, the value is clobbered and the returned bitmask reports it.
;
; Return: bit per failing register, 0 means the context switch preserved the
; complete ABI state.

.code

extern st_usleep:PROC

PUBLIC st_abi_probe

CHECK_GPR MACRO reg, sentinel, bit
    mov     rdx, sentinel
    cmp     reg, rdx
    je      @F
    or      r10d, bit
@@:
ENDM

CHECK_XMM MACRO reg, bit
    mov     eax, 0A5A5A5A5h
    movd    xmm0, eax
    pshufd  xmm0, xmm0, 0
    pcmpeqb xmm0, reg
    pmovmskb eax, xmm0
    cmp     eax, 0FFFFh
    je      @F
    or      r10d, bit
@@:
ENDM

st_abi_probe PROC
    ; Preserve the caller's non-volatile state before borrowing the registers.
    push    rbx
    push    rbp
    push    rdi
    push    rsi
    push    r12
    push    r13
    push    r14
    push    r15

    ; 32 bytes shadow space, 160 bytes XMM6-15, 8 bytes control state.
    sub     rsp, 200
    movdqu  xmmword ptr [rsp+32],  xmm6
    movdqu  xmmword ptr [rsp+48],  xmm7
    movdqu  xmmword ptr [rsp+64],  xmm8
    movdqu  xmmword ptr [rsp+80],  xmm9
    movdqu  xmmword ptr [rsp+96],  xmm10
    movdqu  xmmword ptr [rsp+112], xmm11
    movdqu  xmmword ptr [rsp+128], xmm12
    movdqu  xmmword ptr [rsp+144], xmm13
    movdqu  xmmword ptr [rsp+160], xmm14
    movdqu  xmmword ptr [rsp+176], xmm15
    stmxcsr dword ptr [rsp+192]
    fnstcw  word ptr [rsp+196]

    ; Load sentinels into every non-volatile register.
    mov     rbx, 1111111111111111h
    mov     rbp, 2222222222222222h
    mov     rdi, 3333333333333333h
    mov     rsi, 4444444444444444h
    mov     r12, 1212121212121212h
    mov     r13, 1313131313131313h
    mov     r14, 1414141414141414h
    mov     r15, 1515151515151515h
    mov     eax, 0A5A5A5A5h
    movd    xmm0, eax
    pshufd  xmm0, xmm0, 0
    movdqa  xmm6, xmm0
    movdqa  xmm7, xmm0
    movdqa  xmm8, xmm0
    movdqa  xmm9, xmm0
    movdqa  xmm10, xmm0
    movdqa  xmm11, xmm0
    movdqa  xmm12, xmm0
    movdqa  xmm13, xmm0
    movdqa  xmm14, xmm0
    movdqa  xmm15, xmm0
    mov     eax, 00001F80h
    mov     dword ptr [rsp], eax
    ldmxcsr dword ptr [rsp]
    mov     eax, 027Fh
    mov     word ptr [rsp+4], ax
    fldcw   word ptr [rsp+4]

    ; This call yields to the scheduler and restores this context.
    mov     ecx, 1000
    call    st_usleep

    ; Compare every sentinel after the switch.
    xor     r10d, r10d
    CHECK_GPR rbx, 1111111111111111h, 01h
    CHECK_GPR rbp, 2222222222222222h, 02h
    CHECK_GPR rdi, 3333333333333333h, 04h
    CHECK_GPR rsi, 4444444444444444h, 08h
    CHECK_GPR r12, 1212121212121212h, 10h
    CHECK_GPR r13, 1313131313131313h, 20h
    CHECK_GPR r14, 1414141414141414h, 40h
    CHECK_GPR r15, 1515151515151515h, 80h
    CHECK_XMM xmm6,  00000100h
    CHECK_XMM xmm7,  00000200h
    CHECK_XMM xmm8,  00000400h
    CHECK_XMM xmm9,  00000800h
    CHECK_XMM xmm10, 00001000h
    CHECK_XMM xmm11, 00002000h
    CHECK_XMM xmm12, 00004000h
    CHECK_XMM xmm13, 00008000h
    CHECK_XMM xmm14, 00010000h
    CHECK_XMM xmm15, 00020000h

    ; MXCSR status bits are volatile; compare only the control bits.
    stmxcsr dword ptr [rsp]
    mov     eax, dword ptr [rsp]
    and     eax, 0FFC0h
    cmp     eax, 1F80h
    je      @F
    or      r10d, 00040000h
@@:
    fnstcw  word ptr [rsp]
    movzx   eax, word ptr [rsp]
    cmp     eax, 027Fh
    je      @F
    or      r10d, 00080000h
@@:

    mov     eax, r10d

    ; Restore the caller's state.
    ldmxcsr dword ptr [rsp+192]
    fldcw   word ptr [rsp+196]
    movdqu  xmm6,  xmmword ptr [rsp+32]
    movdqu  xmm7,  xmmword ptr [rsp+48]
    movdqu  xmm8,  xmmword ptr [rsp+64]
    movdqu  xmm9,  xmmword ptr [rsp+80]
    movdqu  xmm10, xmmword ptr [rsp+96]
    movdqu  xmm11, xmmword ptr [rsp+112]
    movdqu  xmm12, xmmword ptr [rsp+128]
    movdqu  xmm13, xmmword ptr [rsp+144]
    movdqu  xmm14, xmmword ptr [rsp+160]
    movdqu  xmm15, xmmword ptr [rsp+176]
    add     rsp, 200

    pop     r15
    pop     r14
    pop     r13
    pop     r12
    pop     rsi
    pop     rdi
    pop     rbp
    pop     rbx
    ret
st_abi_probe ENDP

.data
; No data section is needed now; the pattern is generated in a GPR.

END
