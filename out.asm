global _start
_start:
   mov rax, 3
   push rax
label0:
   push QWORD [rsp + 0]
   pop rax
   cmp rax, 0
   jz label1
   push QWORD [rsp + 0]
   mov rax, 5
   push rax
   pop rbx
   pop rax
   add rax, rbx
   push rax
   push QWORD [rsp + 0]
   mov rax, 60
   pop rdi
   syscall
   add rsp, 0
   jmp label0
label1:
   push QWORD [rsp + 0]
   mov rax, 60
   pop rdi
   syscall
   mov rax, 60
   mov rdi, 0
   syscall
