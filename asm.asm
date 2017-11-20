section .text

; void CleanseStack(unsigned long pages);
global CleanseStack
CleanseStack:
  cmp rdi,0
  je .end

  mov rax,rsp
  and rax,0xfff
  neg rax

  cmp rax,0
  je .aligned

  xor rcx,rcx
  .alignPage:
    mov qword [rsp+rcx-8],0x0000000000000000
    sub rcx,8
    cmp rcx,rax
    jne .alignPage

  ; RSP+RCX is aligned to page
  .aligned:
    pxor xmm0,xmm0

  .pageBoundary:
    mov rbx,256
    .cleanPage:
      movdqa [rsp+rcx-16],xmm0
      sub rcx,16
      dec rbx
      jnz .cleanPage

    dec rdi
    jnz .pageBoundary

  .end:
    repz retn
