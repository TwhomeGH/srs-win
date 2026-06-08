; SPDX-License-Identifier: MIT
; Copyright (c) 2021-2022 The SRS Authors
;
; x64 Windows MSVC assembly for ST (State Threads) context switching.
; Windows x64 calling convention: rcx = arg1 (env), edx/rdx = arg2 (val)
; Must preserve: rbx, rbp, rdi, rsi, r12-r15 (non-volatile registers)

; Internal __jmp_buf layout indices
JB_RBX equ 0
JB_RBP equ 1
JB_R12 equ 2
JB_R13 equ 3
JB_R14 equ 4
JB_R15 equ 5
JB_RSP equ 6
JB_PC  equ 7

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
