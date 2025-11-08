global _start
_start:
   mov rax, 10
   push rax
   mov rax, 10
   push rax
label0:
   push QWORD [rsp + 8]
   mov rax, 20
   push rax
   pop rbx
   pop rax
   cmp rax, rbx
   setne al
   movzx rax, al
   push rax
   pop rax
   cmp rax, 0
   jz label1
   push QWORD [rsp + 0]
   mov rax, 2
   push rax
   pop rbx
   pop rax
   add rax, rbx
   push rax
   pop rax
   mov QWORD [rsp + 0], rax
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
