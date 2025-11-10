section .bss
_g_label0:
    resq 1

section .text
global _start
_start:
   mov rax, 1
   push rax
   pop rax
   mov QWORD [_g_label0], rax
   mov rax, 3
   push rax
   push QWORD [rsp + 0]
   mov rax, 60
   pop rdi
   syscall
   add rsp, 0
   add rsp, 8
   jmp overlabel2func
funclabel1:
   mov rax, QWORD [rsp + 32]
   push rax
   mov rax, QWORD [rsp + 32]
   push rax
   mov rax, QWORD [rsp + 32]
   push rax
   mov rax, QWORD [rsp + 32]
   push rax
   push QWORD [rsp + 24]
   push QWORD [rsp + 24]
   pop rbx
   pop rax
   add rax, rbx
   push rax
   push QWORD [rsp + 16]
   pop rbx
   pop rax
   add rax, rbx
   push rax
   push QWORD [rsp + 8]
   pop rbx
   pop rax
   add rax, rbx
   push rax
   push QWORD [_g_label0]
   pop rbx
   pop rax
   add rax, rbx
   push rax
   pop rax
   add rsp, 32
   ret
   add rsp, 0
   ret
   add rsp, 32
overlabel2func:
   mov rax, 3
   push rax
   mov rax, 4
   push rax
   mov rax, 2
   push rax
   mov rax, 3
   push rax
   call funclabel1
   add rsp, 32
   push rax
   mov rax, 60
   pop rdi
   syscall
   mov rax, 60
   mov rdi, 0
   syscall
