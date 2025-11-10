global _start
_start:
   mov rax, 1
   push rax
   jmp overlabel0func
func:
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
   push QWORD [rsp + 40]
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
overlabel0func:
   mov rax, 3
   push rax
   mov rax, 4
   push rax
   mov rax, 2
   push rax
   mov rax, 3
   push rax
   call func
   add rsp, 32
   push rax
   mov rax, 60
   pop rdi
   syscall
   mov rax, 60
   mov rdi, 0
   syscall
