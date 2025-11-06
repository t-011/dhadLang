global _start
_start:
   mov rax, 9
   push rax
   mov rax, 2
   push rax
   pop rbx
   pop rax
   idiv rbx
   push rdx
   mov rax, 1
   push rax
   pop rbx
   pop rax
   add rax, rbx
   push rax
   mov rax, 2
   push rax
   pop rbx
   pop rax
   imul rax, rbx
   push rax
   mov rax, 3
   push rax
   mov rax, 5
   push rax
   pop rbx
   pop rax
   imul rax, rbx
   push rax
   push QWORD [rsp + 8]
   mov rax, 60
   pop rdi
   syscall
   mov rax, 60
   mov rdi, 0
   syscall
