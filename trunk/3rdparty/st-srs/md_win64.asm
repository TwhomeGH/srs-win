; SPDX-License-Identifier: MIT
; Copyright (c) 2021-2022 The SRS Authors
;
; x64 Windows MSVC assembly for ST (State Threads) context switching.
; Windows x64 calling convention: rcx = arg1 (env), edx/rdx = arg2 (val)
;
; This file is intentionally small: it is the ABI boundary, not the scheduler.
; A stackful coroutine switch must save/restore the complete Windows x64
; non-volatile register set (GPRs plus XMM6-XMM15), the stack pointer, the
; return PC, and the floating-point control state. C cannot express that
; operation, MSVC x64 has no inline assembly, so MASM is the supported route.
; The scheduler and run queues remain in sched.c.
; Must preserve: rbx, rbp, rdi, rsi, r12-r15, xmm6-xmm15, mxcsr, x87 CW.

; Internal __jmp_buf layout indices
JB_RBX equ 0
JB_RBP equ 1
JB_R12 equ 2
JB_R13 equ 3
JB_R14 equ 4
JB_R15 equ 5
JB_RSP equ 6
JB_PC  equ 7
JB_XMM6 equ 8
JB_XMM7 equ 10
JB_XMM8 equ 12
JB_XMM9 equ 14
JB_XMM10 equ 16
JB_XMM11 equ 18
JB_XMM12 equ 20
JB_XMM13 equ 22
JB_XMM14 equ 24
JB_XMM15 equ 26
JB_MXCSR equ 28
JB_X87CW equ 29
JB_RDI equ 30
JB_RSI equ 31

.code

; int _st_md_cxt_save(_st_jmp_buf_t env);
; Saves current register state into env, returns 0.
; rcx = pointer to jmp_buf
align 16
_st_md_cxt_save PROC PUBLIC
    ; Save non-volatile registers
    mov     qword ptr [rcx+JB_RBX*8], rbx
    mov     qword ptr [rcx+JB_RBP*8], rbp
    mov     qword ptr [rcx+JB_R12*8], r12
    mov     qword ptr [rcx+JB_R13*8], r13
    mov     qword ptr [rcx+JB_R14*8], r14
    mov     qword ptr [rcx+JB_R15*8], r15
    ; Windows x64 also requires RDI/RSI to survive a context switch.
    mov     qword ptr [rcx+JB_RDI*8], rdi
    mov     qword ptr [rcx+JB_RSI*8], rsi
    ; Windows x64 ABI makes XMM6-XMM15 non-volatile.
    movdqu  xmmword ptr [rcx+JB_XMM6*8], xmm6
    movdqu  xmmword ptr [rcx+JB_XMM7*8], xmm7
    movdqu  xmmword ptr [rcx+JB_XMM8*8], xmm8
    movdqu  xmmword ptr [rcx+JB_XMM9*8], xmm9
    movdqu  xmmword ptr [rcx+JB_XMM10*8], xmm10
    movdqu  xmmword ptr [rcx+JB_XMM11*8], xmm11
    movdqu  xmmword ptr [rcx+JB_XMM12*8], xmm12
    movdqu  xmmword ptr [rcx+JB_XMM13*8], xmm13
    movdqu  xmmword ptr [rcx+JB_XMM14*8], xmm14
    movdqu  xmmword ptr [rcx+JB_XMM15*8], xmm15
    stmxcsr dword ptr [rcx+JB_MXCSR*8]
    fnstcw  word ptr [rcx+JB_X87CW*8]
    ; Save stack pointer (skip return address on stack)
    lea     r8,  [rsp+8]
    mov     qword ptr [rcx+JB_RSP*8], r8
    ; Save return address as PC
    mov     r9,  [rsp]
    mov     qword ptr [rcx+JB_PC*8],  r9
    ; Return 0
    xor     eax, eax
    ret
_st_md_cxt_save ENDP

; void _st_md_cxt_restore(_st_jmp_buf_t env, int val);
; Restores register state from env and returns val (or 1 if val==0).
; rcx = pointer to jmp_buf, edx = val to return
align 16
_st_md_cxt_restore PROC PUBLIC
    ; Restore non-volatile registers
    mov     rbx, qword ptr [rcx+JB_RBX*8]
    mov     rbp, qword ptr [rcx+JB_RBP*8]
    mov     r12, qword ptr [rcx+JB_R12*8]
    mov     r13, qword ptr [rcx+JB_R13*8]
    mov     r14, qword ptr [rcx+JB_R14*8]
    mov     r15, qword ptr [rcx+JB_R15*8]
    mov     rdi, qword ptr [rcx+JB_RDI*8]
    mov     rsi, qword ptr [rcx+JB_RSI*8]
    movdqu  xmm6,  xmmword ptr [rcx+JB_XMM6*8]
    movdqu  xmm7,  xmmword ptr [rcx+JB_XMM7*8]
    movdqu  xmm8,  xmmword ptr [rcx+JB_XMM8*8]
    movdqu  xmm9,  xmmword ptr [rcx+JB_XMM9*8]
    movdqu  xmm10, xmmword ptr [rcx+JB_XMM10*8]
    movdqu  xmm11, xmmword ptr [rcx+JB_XMM11*8]
    movdqu  xmm12, xmmword ptr [rcx+JB_XMM12*8]
    movdqu  xmm13, xmmword ptr [rcx+JB_XMM13*8]
    movdqu  xmm14, xmmword ptr [rcx+JB_XMM14*8]
    movdqu  xmm15, xmmword ptr [rcx+JB_XMM15*8]
    ldmxcsr dword ptr [rcx+JB_MXCSR*8]
    fldcw   word ptr [rcx+JB_X87CW*8]
    ; Ensure return value is non-zero
    test    edx, edx
    mov     eax, 1
    cmove   edx, eax
    mov     eax, edx
    ; Restore PC and stack pointer
    mov     r8,  qword ptr [rcx+JB_PC*8]
    mov     rsp, qword ptr [rcx+JB_RSP*8]
    ; Jump to saved PC
    jmp     r8
_st_md_cxt_restore ENDP

END
