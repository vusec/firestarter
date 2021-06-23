/* Copyright (C) 2001-2014 Free Software Foundation, Inc.
   This file is part of the GNU C Library.

   The GNU C Library is free software; you can redistribute it and/or
   modify it under the terms of the GNU Lesser General Public
   License as published by the Free Software Foundation; either
   version 2.1 of the License, or (at your option) any later version.

   The GNU C Library is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
   Lesser General Public License for more details.

   You should have received a copy of the GNU Lesser General Public
   License along with the GNU C Library; if not, see
   <http://www.gnu.org/licenses/>.  */

/* Put together and modified from GLIBC-2.19 source code. */

#include "jmpbuf-offsets.h"
#include "sysdep.h"

#undef PTR_MANGLE

ENTRY (ltckpt_asm_save_registers)
        /* Save registers.  */
        movq %rbx, (JB_RBX*8)(%rdi)
#ifdef PTR_MANGLE
# ifdef __ILP32__
        /* Save the high bits of %rbp first, since PTR_MANGLE will
           only handle the low bits but we cannot presume %rbp is
           being used as a pointer and truncate it.  Here we write all
           of %rbp, but the low bits will be overwritten below.  */
        movq %rbp, (JB_RBP*8)(%rdi)
# endif
        mov %RBP_LP, %RAX_LP
        PTR_MANGLE (%RAX_LP)
        mov %RAX_LP, (JB_RBP*8)(%rdi)
#else
        movq %rbp, (JB_RBP*8)(%rdi)
#endif
        movq %r12, (JB_R12*8)(%rdi)
        movq %r13, (JB_R13*8)(%rdi)
        movq %r14, (JB_R14*8)(%rdi)
        movq %r15, (JB_R15*8)(%rdi)
        lea 8(%rsp), %RDX_LP    /* Save SP as it will be after we return.  */
#ifdef PTR_MANGLE
        PTR_MANGLE (%RDX_LP)
#endif
        movq %rdx, (JB_RSP*8)(%rdi)
        mov (%rsp), %RAX_LP     /* Save PC we are returning to now.  */
#ifdef PTR_MANGLE
        PTR_MANGLE (%RAX_LP)
#endif
        movq %rax, (JB_PC*8)(%rdi)

        xorl %eax, %eax
        retq
END (ltckpt_asm_save_registers)

        .text
ENTRY(ltckpt_asm_restore_registers)
        /* Restore registers.  */
        mov (JB_RSP*8)(%rdi),%R8_LP
        mov (JB_RBP*8)(%rdi),%R9_LP
        mov (JB_PC*8)(%rdi),%RDX_LP
#ifdef PTR_DEMANGLE
        PTR_DEMANGLE (%R8_LP)
        PTR_DEMANGLE (%R9_LP)
        PTR_DEMANGLE (%RDX_LP)
# ifdef __ILP32__
        /* We ignored the high bits of the %rbp value because only the low
           bits are mangled.  But we cannot presume that %rbp is being used
           as a pointer and truncate it, so recover the high bits.  */
        movl (JB_RBP*8 + 4)(%rdi), %eax
        shlq $32, %rax
        orq %rax, %r9
# endif
#endif
        movq (JB_RBX*8)(%rdi),%rbx
        movq (JB_R12*8)(%rdi),%r12
        movq (JB_R13*8)(%rdi),%r13
        movq (JB_R14*8)(%rdi),%r14
        movq (JB_R15*8)(%rdi),%r15
        /* Set return value for setjmp.  */
        mov %esi, %eax
        mov %R8_LP,%RSP_LP
        movq %r9,%rbp
        jmpq *%rdx
END (ltckpt_asm_restore_registers)
