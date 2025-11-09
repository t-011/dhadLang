global _start
_start:
   mov rax, 0
   push rax
   push QWORD [rsp + 0]
   mov rax, 1
   push rax
   pop rbx
   pop rax
   cmp rax, rbx
   setg al
   movzx rax, al
   push rax
   pop rax
   cmp rax, 0
   jz label0
   mov rax, 1
   push rax
   pop rax
   mov QWORD [rsp + 0], rax
   add rsp, 0
   jmp endlabel1
label0:
   push QWORD [rsp + 0]
   mov rax, 2
   push rax
   pop rbx
   pop rax
   cmp rax, rbx
   sete al
   movzx rax, al
   push rax
   pop rax
   cmp rax, 0
   jz label2
   mov rax, 2
   push rax
   pop rax
   mov QWORD [rsp + 0], rax
   add rsp, 0
   jmp endlabel1
label2:
   push QWORD [rsp + 0]
   mov rax, 0
   push rax
   pop rbx
   pop rax
   cmp rax, rbx
   setne al
   movzx rax, al
   push rax
   pop rax
   cmp rax, 0
   jz label3
   mov rax, 3
   push rax
   pop rax
   mov QWORD [rsp + 0], rax
   add rsp, 0
   jmp endlabel1
label3:
   push QWORD [rsp + 0]
   mov rax, 0
   push rax
   pop rbx
   pop rax
   cmp rax, rbx
   setne al
   movzx rax, al
   push rax
   pop rax
   cmp rax, 0
   jz label4
   mov rax, 4
   push rax
   pop rax
   mov QWORD [rsp + 0], rax
   add rsp, 0
   jmp endlabel1
label4:
   mov rax, 5
   push rax
   pop rax
   mov QWORD [rsp + 0], rax
   add rsp, 0
endlabel1:
   push QWORD [rsp + 0]
   mov rax, 60
   pop rdi
   syscall
   mov rax, 60
   mov rdi, 0
   syscall
