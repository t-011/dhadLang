global _start
_start:
   mov rax, 3
   push rax
   mov rax, 0
   push rax
   pop rax
   cmp rax, 0
   jz label0
   mov rax, 69
   push rax
   mov rax, 60
   pop rdi
   syscall
   add rsp, 0
   jmp endlabel1
label0:
   mov rax, 0
   push rax
   pop rax
   cmp rax, 0
   jz label2
   mov rax, 90
   push rax
   mov rax, 60
   pop rdi
   syscall
   add rsp, 0
   jmp endlabel1
label2:
   push QWORD [rsp + 0]
   mov rax, 60
   pop rdi
   syscall
   add rsp, 0
endlabel1:
   mov rax, 60
   mov rdi, 0
   syscall
