global _start
_start:
   mov rax, 0
   push rax
   push QWORD [rsp + 0]
   pop rax
   cmp rax, 0
   jz label0
   mov rax, 1
   push rax
   mov rax, 60
   pop rdi
   syscall
   add rsp, 0
   label0:
   mov rax, 0
   push rax
   mov rax, 60
   pop rdi
   syscall
   mov rax, 60
   mov rdi, 0
   syscall
