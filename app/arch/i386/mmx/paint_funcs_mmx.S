/*
MMX code to supplement some functions in paint_funcs.c
for the Gimp.
 
Copyright (C) 1999, 2001 David Monniaux
*/

.text
.align 4

.globl intel_cpu_features

intel_cpu_features:
	pushl	%ebx
	pushfl
	popl	%eax
	xor	$ 0x200000, %eax
	pushl	%eax
	popfl
	pushfl
	popl	%edx
	xor	%eax, %edx
	xor	%eax, %eax
	test	$ 0x200000, %edx
	jnz 	.intel_cpu_features_end
	movl	$ 1, %eax
	cpuid
	movl	%edx, %eax
.intel_cpu_features_end:
	popl	%ebx
	ret

.alpha_mask_1a:	.int 0xFF00FF00, 0xFF00FF00
.mult_shift:	.int 0x00800080, 0x00800080
.alpha_mask_3a:	.int 0xFF000000, 0xFF000000



/* min(a,b) = a - max(a-b, 0) */

.globl add_pixels_3a_3a

.align 16
add_pixels_3a_3a:
	pushl	%edi
	pushl	%ebx
	movl	12(%esp), %edi
	movq	.alpha_mask_3a, %mm0
	
	subl	$ 2, %ecx
	jl	.add_pixels_3a_3a_last
	movl	$ 8, %ebx
.add_pixels_3a_3a_loop:
	movq	(%eax), %mm2
	movq	(%edx), %mm3
	
	movq	%mm2, %mm4
	paddusb %mm3, %mm4
	movq	%mm0, %mm1
	pandn	%mm4, %mm1
	movq	%mm2, %mm4
	psubusb %mm3, %mm4
	psubb	%mm4, %mm2
	pand	%mm0, %mm2
	por	%mm2, %mm1
	movq	%mm1, (%edi)
	addl	%ebx, %eax
	addl	%ebx, %edx
	addl	%ebx, %edi
	subl	$ 2, %ecx
	jge	.add_pixels_3a_3a_loop
.add_pixels_3a_3a_last:
	test	$ 1, %ecx
	jz	.add_pixels_3a_3a_end
	movd	(%eax), %mm2
	movd	(%edx), %mm3
	
	movq	%mm2, %mm4
	paddusb %mm3, %mm4
	movq	%mm0, %mm1
	pandn	%mm4, %mm1
	movq	%mm2, %mm4
	psubusb %mm3, %mm4
	psubb	%mm4, %mm2
	pand	%mm0, %mm2
	por	%mm2, %mm1
	movd	%mm1, (%edi)	
.add_pixels_3a_3a_end:		
	
	emms
	popl	%ebx
	popl	%edi
	ret

.globl add_pixels_1a_1a
.align 16
add_pixels_1a_1a:
	pushl	%edi
	pushl	%ebx
	movl	12(%esp), %edi
	movq	.alpha_mask_1a, %mm0
	subl	$ 4, %ecx
	jl	.add_pixels_1a_1a_last3
	movl	$ 8, %ebx
.add_pixels_1a_1a_loop:
	movq	(%eax), %mm2
	movq	(%edx), %mm3
	
	movq	%mm2, %mm4
	paddusb %mm3, %mm4
	movq	%mm0, %mm1
	pandn	%mm4, %mm1
	movq	%mm2, %mm4
	psubusb %mm3, %mm4
	psubb	%mm4, %mm2
	pand	%mm0, %mm2
	por	%mm2, %mm1
	movq	%mm1, (%edi)
	addl	%ebx, %eax
	addl	%ebx, %edx
	addl	%ebx, %edi
	subl	$ 4, %ecx
	jge	.add_pixels_1a_1a_loop

.add_pixels_1a_1a_last3:
	test	$ 2, %ecx
	jz	.add_pixels_1a_1a_last1
	movd	(%eax), %mm2
	movd	(%edx), %mm3
	
	movq	%mm2, %mm4
	paddusb %mm3, %mm4
	movq	%mm0, %mm1
	pandn	%mm4, %mm1
	movq	%mm2, %mm4
	psubusb %mm3, %mm4
	psubb	%mm4, %mm2
	pand	%mm0, %mm2
	por	%mm2, %mm1
	addl	$ 4, %eax
	addl	$ 4, %edx
	addl	$ 4, %edi

.add_pixels_1a_1a_last1:
	test	$ 1, %ecx
	jz	.add_pixels_1a_1a_end

	movw	(%eax), %bx
	movd	%ebx, %mm2
	movw	(%edx), %bx
	movd	%ebx, %mm3
	
	movq	%mm2, %mm4
	paddusb %mm3, %mm4
	movq	%mm0, %mm1
	pandn	%mm4, %mm1
	movq	%mm2, %mm4
	psubusb %mm3, %mm4
	psubb	%mm4, %mm2
	pand	%mm0, %mm2
	por	%mm2, %mm1
	movd	%mm1, %ebx
	movw	%bx, (%edi)

.add_pixels_1a_1a_end:		
	
	emms
	popl	%ebx
	popl	%edi
	ret


.globl substract_pixels_3a_3a

.align 16
substract_pixels_3a_3a:
	pushl	%edi
	pushl	%ebx
	movl	12(%esp), %edi
	movq	.alpha_mask_3a, %mm0
	
	subl	$ 2, %ecx
	jl	.substract_pixels_3a_3a_last
	movl	$ 8, %ebx
.substract_pixels_3a_3a_loop:
	movq	(%eax), %mm2
	movq	(%edx), %mm3
	
	movq	%mm2, %mm4
	psubusb %mm3, %mm4
	movq	%mm0, %mm1
	pandn	%mm4, %mm1
	psubb	%mm4, %mm2
	pand	%mm0, %mm2
	por	%mm2, %mm1
	movq	%mm1, (%edi)
	addl	%ebx, %eax
	addl	%ebx, %edx
	addl	%ebx, %edi
	subl	$ 2, %ecx
	jge	.substract_pixels_3a_3a_loop
.substract_pixels_3a_3a_last:
	test	$ 1, %ecx
	jz	.substract_pixels_3a_3a_end
	movd	(%eax), %mm2
	movd	(%edx), %mm3
	
	movq	%mm2, %mm4
	psubusb %mm3, %mm4
	movq	%mm0, %mm1
	pandn	%mm4, %mm1
	psubb	%mm4, %mm2
	pand	%mm0, %mm2
	por	%mm2, %mm1
	movd	%mm1, (%edi)	
.substract_pixels_3a_3a_end:		
	
	emms
	popl	%ebx
	popl	%edi
	ret

.globl substract_pixels_1a_1a
.align 16
substract_pixels_1a_1a:
	pushl	%edi
	pushl	%ebx
	movl	12(%esp), %edi
	movq	.alpha_mask_1a, %mm0
	subl	$ 4, %ecx
	jl	.substract_pixels_1a_1a_last3
	movl	$ 8, %ebx
.substract_pixels_1a_1a_loop:
	movq	(%eax), %mm2
	movq	(%edx), %mm3
	
	movq	%mm2, %mm4
	psubusb %mm3, %mm4
	movq	%mm0, %mm1
	pandn	%mm4, %mm1
	psubb	%mm4, %mm2
	pand	%mm0, %mm2
	por	%mm2, %mm1
	movq	%mm1, (%edi)
	addl	%ebx, %eax
	addl	%ebx, %edx
	addl	%ebx, %edi
	subl	$ 4, %ecx
	jge	.substract_pixels_1a_1a_loop

.substract_pixels_1a_1a_last3:
	test	$ 2, %ecx
	jz	.substract_pixels_1a_1a_last1
	movd	(%eax), %mm2
	movd	(%edx), %mm3
	
	movq	%mm2, %mm4
	psubusb %mm3, %mm4
	movq	%mm0, %mm1
	pandn	%mm4, %mm1
	psubb	%mm4, %mm2
	pand	%mm0, %mm2
	por	%mm2, %mm1
	addl	$ 4, %eax
	addl	$ 4, %edx
	addl	$ 4, %edi

.substract_pixels_1a_1a_last1:
	test	$ 1, %ecx
	jz	.substract_pixels_1a_1a_end

	movw	(%eax), %bx
	movd	%ebx, %mm2
	movw	(%edx), %bx
	movd	%ebx, %mm3
	
	movq	%mm2, %mm4
	psubusb %mm3, %mm4
	movq	%mm0, %mm1
	pandn	%mm4, %mm1
	psubb	%mm4, %mm2
	pand	%mm0, %mm2
	por	%mm2, %mm1
	movd	%mm1, %ebx
	movw	%bx, (%edi)

.substract_pixels_1a_1a_end:		
	
	emms
	popl	%ebx
	popl	%edi
	ret


.globl difference_pixels_3a_3a

.align 16
difference_pixels_3a_3a:
	pushl	%edi
	pushl	%ebx
	movl	12(%esp), %edi
	movq	.alpha_mask_3a, %mm0
	
	subl	$ 2, %ecx
	jl	.difference_pixels_3a_3a_last
	movl	$ 8, %ebx
.difference_pixels_3a_3a_loop:
	movq	(%eax), %mm2
	movq	(%edx), %mm3
	
	movq	%mm2, %mm4
	movq	%mm3, %mm5
	psubusb %mm3, %mm4
	psubusb	%mm2, %mm5
	movq	%mm0, %mm1
	paddb	%mm5, %mm4
	pandn	%mm4, %mm1
	psubb	%mm4, %mm2
	pand	%mm0, %mm2
	por	%mm2, %mm1
	movq	%mm1, (%edi)
	addl	%ebx, %eax
	addl	%ebx, %edx
	addl	%ebx, %edi
	subl	$ 2, %ecx
	jge	.difference_pixels_3a_3a_loop
.difference_pixels_3a_3a_last:
	test	$ 1, %ecx
	jz	.difference_pixels_3a_3a_end
	movd	(%eax), %mm2
	movd	(%edx), %mm3
	
	movq	%mm2, %mm4
	movq	%mm3, %mm5
	psubusb %mm3, %mm4
	psubusb	%mm2, %mm5
	movq	%mm0, %mm1
	paddb	%mm5, %mm4
	pandn	%mm4, %mm1
	psubb	%mm4, %mm2
	pand	%mm0, %mm2
	por	%mm2, %mm1
	movd	%mm1, (%edi)	
.difference_pixels_3a_3a_end:		
	
	emms
	popl	%ebx
	popl	%edi
	ret

.globl difference_pixels_1a_1a
.align 16
difference_pixels_1a_1a:
	pushl	%edi
	pushl	%ebx
	movl	12(%esp), %edi
	movq	.alpha_mask_1a, %mm0
	subl	$ 4, %ecx
	jl	.difference_pixels_1a_1a_last3
	movl	$ 8, %ebx
.difference_pixels_1a_1a_loop:
	movq	(%eax), %mm2
	movq	(%edx), %mm3
	
	movq	%mm2, %mm4
	movq	%mm3, %mm5
	psubusb %mm3, %mm4
	psubusb	%mm2, %mm5
	movq	%mm0, %mm1
	paddb	%mm5, %mm4
	pandn	%mm4, %mm1
	psubb	%mm4, %mm2
	pand	%mm0, %mm2
	por	%mm2, %mm1
	movq	%mm1, (%edi)
	addl	%ebx, %eax
	addl	%ebx, %edx
	addl	%ebx, %edi
	subl	$ 4, %ecx
	jge	.difference_pixels_1a_1a_loop

.difference_pixels_1a_1a_last3:
	test	$ 2, %ecx
	jz	.difference_pixels_1a_1a_last1
	movd	(%eax), %mm2
	movd	(%edx), %mm3
	
	movq	%mm2, %mm4
	movq	%mm3, %mm5
	psubusb %mm3, %mm4
	psubusb	%mm2, %mm5
	movq	%mm0, %mm1
	paddb	%mm5, %mm4
	pandn	%mm4, %mm1
	psubb	%mm4, %mm2
	pand	%mm0, %mm2
	por	%mm2, %mm1
	addl	$ 4, %eax
	addl	$ 4, %edx
	addl	$ 4, %edi

.difference_pixels_1a_1a_last1:
	test	$ 1, %ecx
	jz	.difference_pixels_1a_1a_end

	movw	(%eax), %bx
	movd	%ebx, %mm2
	movw	(%edx), %bx
	movd	%ebx, %mm3
	
	movq	%mm2, %mm4
	movq	%mm3, %mm5
	psubusb %mm3, %mm4
	psubusb	%mm2, %mm5
	movq	%mm0, %mm1
	paddb	%mm5, %mm4
	pandn	%mm4, %mm1
	psubb	%mm4, %mm2
	pand	%mm0, %mm2
	por	%mm2, %mm1
	movd	%mm1, %ebx
	movw	%bx, (%edi)

.difference_pixels_1a_1a_end:		
	
	emms
	popl	%ebx
	popl	%edi
	ret


.globl multiply_pixels_3a_3a

.align 16
multiply_pixels_3a_3a:
	pushl	%edi
	pushl	%ebx
	movl	12(%esp), %edi
	movq	.alpha_mask_3a, %mm0
	
	movq	.mult_shift, %mm7
	pxor	%mm6, %mm6
	subl	$ 2, %ecx
	jl	.multiply_pixels_3a_3a_last
	movl	$ 8, %ebx
.multiply_pixels_3a_3a_loop:
	movq	(%eax), %mm2
	movq	(%edx), %mm3
	

	movq	%mm2, %mm1
	punpcklbw %mm6, %mm1
	movq	%mm3, %mm5
	punpcklbw %mm6, %mm5
	pmullw	%mm5, %mm1
	paddw	%mm7, %mm1
	movq	%mm1, %mm5
	psrlw	$ 8, %mm5
	paddw	%mm5, %mm1
	psrlw	$ 8, %mm1

	movq	%mm2, %mm4
	punpckhbw %mm6, %mm4
	movq	%mm3, %mm5
	punpckhbw %mm6, %mm5
	pmullw	%mm5, %mm4
	paddw	%mm7, %mm4
	movq	%mm4, %mm5
	psrlw	$ 8, %mm5
	paddw	%mm5, %mm4
	psrlw	$ 8, %mm4

	packuswb %mm4, %mm1

	movq	%mm0, %mm4
	pandn	%mm1, %mm4
	movq	%mm4, %mm1

	movq	%mm2, %mm4
	psubusb %mm3, %mm4
	psubb	%mm4, %mm2
	pand	%mm0, %mm2
	por	%mm2, %mm1
	movq	%mm1, (%edi)
	addl	%ebx, %eax
	addl	%ebx, %edx
	addl	%ebx, %edi
	subl	$ 2, %ecx
	jge	.multiply_pixels_3a_3a_loop
.multiply_pixels_3a_3a_last:
	test	$ 1, %ecx
	jz	.multiply_pixels_3a_3a_end
	movd	(%eax), %mm2
	movd	(%edx), %mm3
	

	movq	%mm2, %mm1
	punpcklbw %mm6, %mm1
	movq	%mm3, %mm5
	punpcklbw %mm6, %mm5
	pmullw	%mm5, %mm1
	paddw	%mm7, %mm1
	movq	%mm1, %mm5
	psrlw	$ 8, %mm5
	paddw	%mm5, %mm1
	psrlw	$ 8, %mm1

	movq	%mm2, %mm4
	punpckhbw %mm6, %mm4
	movq	%mm3, %mm5
	punpckhbw %mm6, %mm5
	pmullw	%mm5, %mm4
	paddw	%mm7, %mm4
	movq	%mm4, %mm5
	psrlw	$ 8, %mm5
	paddw	%mm5, %mm4
	psrlw	$ 8, %mm4

	packuswb %mm4, %mm1

	movq	%mm0, %mm4
	pandn	%mm1, %mm4
	movq	%mm4, %mm1

	movq	%mm2, %mm4
	psubusb %mm3, %mm4
	psubb	%mm4, %mm2
	pand	%mm0, %mm2
	por	%mm2, %mm1
	movd	%mm1, (%edi)	
.multiply_pixels_3a_3a_end:		
	
	emms
	popl	%ebx
	popl	%edi
	ret

.globl multiply_pixels_1a_1a
.align 16
multiply_pixels_1a_1a:
	pushl	%edi
	pushl	%ebx
	movl	12(%esp), %edi
	movq	.alpha_mask_1a, %mm0
	subl	$ 4, %ecx
	jl	.multiply_pixels_1a_1a_last3
	movl	$ 8, %ebx
.multiply_pixels_1a_1a_loop:
	movq	(%eax), %mm2
	movq	(%edx), %mm3
	

	movq	%mm2, %mm1
	punpcklbw %mm6, %mm1
	movq	%mm3, %mm5
	punpcklbw %mm6, %mm5
	pmullw	%mm5, %mm1
	paddw	%mm7, %mm1
	movq	%mm1, %mm5
	psrlw	$ 8, %mm5
	paddw	%mm5, %mm1
	psrlw	$ 8, %mm1

	movq	%mm2, %mm4
	punpckhbw %mm6, %mm4
	movq	%mm3, %mm5
	punpckhbw %mm6, %mm5
	pmullw	%mm5, %mm4
	paddw	%mm7, %mm4
	movq	%mm4, %mm5
	psrlw	$ 8, %mm5
	paddw	%mm5, %mm4
	psrlw	$ 8, %mm4

	packuswb %mm4, %mm1

	movq	%mm0, %mm4
	pandn	%mm1, %mm4
	movq	%mm4, %mm1

	movq	%mm2, %mm4
	psubusb %mm3, %mm4
	psubb	%mm4, %mm2
	pand	%mm0, %mm2
	por	%mm2, %mm1
	movq	%mm1, (%edi)
	addl	%ebx, %eax
	addl	%ebx, %edx
	addl	%ebx, %edi
	subl	$ 4, %ecx
	jge	.multiply_pixels_1a_1a_loop

.multiply_pixels_1a_1a_last3:
	test	$ 2, %ecx
	jz	.multiply_pixels_1a_1a_last1
	movd	(%eax), %mm2
	movd	(%edx), %mm3
	

	movq	%mm2, %mm1
	punpcklbw %mm6, %mm1
	movq	%mm3, %mm5
	punpcklbw %mm6, %mm5
	pmullw	%mm5, %mm1
	paddw	%mm7, %mm1
	movq	%mm1, %mm5
	psrlw	$ 8, %mm5
	paddw	%mm5, %mm1
	psrlw	$ 8, %mm1

	movq	%mm2, %mm4
	punpckhbw %mm6, %mm4
	movq	%mm3, %mm5
	punpckhbw %mm6, %mm5
	pmullw	%mm5, %mm4
	paddw	%mm7, %mm4
	movq	%mm4, %mm5
	psrlw	$ 8, %mm5
	paddw	%mm5, %mm4
	psrlw	$ 8, %mm4

	packuswb %mm4, %mm1

	movq	%mm0, %mm4
	pandn	%mm1, %mm4
	movq	%mm4, %mm1

	movq	%mm2, %mm4
	psubusb %mm3, %mm4
	psubb	%mm4, %mm2
	pand	%mm0, %mm2
	por	%mm2, %mm1
	addl	$ 4, %eax
	addl	$ 4, %edx
	addl	$ 4, %edi

.multiply_pixels_1a_1a_last1:
	test	$ 1, %ecx
	jz	.multiply_pixels_1a_1a_end

	movw	(%eax), %bx
	movd	%ebx, %mm2
	movw	(%edx), %bx
	movd	%ebx, %mm3
	

	movq	%mm2, %mm1
	punpcklbw %mm6, %mm1
	movq	%mm3, %mm5
	punpcklbw %mm6, %mm5
	pmullw	%mm5, %mm1
	paddw	%mm7, %mm1
	movq	%mm1, %mm5
	psrlw	$ 8, %mm5
	paddw	%mm5, %mm1
	psrlw	$ 8, %mm1

	movq	%mm2, %mm4
	punpckhbw %mm6, %mm4
	movq	%mm3, %mm5
	punpckhbw %mm6, %mm5
	pmullw	%mm5, %mm4
	paddw	%mm7, %mm4
	movq	%mm4, %mm5
	psrlw	$ 8, %mm5
	paddw	%mm5, %mm4
	psrlw	$ 8, %mm4

	packuswb %mm4, %mm1

	movq	%mm0, %mm4
	pandn	%mm1, %mm4
	movq	%mm4, %mm1

	movq	%mm2, %mm4
	psubusb %mm3, %mm4
	psubb	%mm4, %mm2
	pand	%mm0, %mm2
	por	%mm2, %mm1
	movd	%mm1, %ebx
	movw	%bx, (%edi)

.multiply_pixels_1a_1a_end:		
	
	emms
	popl	%ebx
	popl	%edi
	ret

/* Could be perhaps more optimized */

.globl darken_pixels_3a_3a

.align 16
darken_pixels_3a_3a:
	pushl	%edi
	pushl	%ebx
	movl	12(%esp), %edi
	movq	.alpha_mask_3a, %mm0
	
	subl	$ 2, %ecx
	jl	.darken_pixels_3a_3a_last
	movl	$ 8, %ebx
.darken_pixels_3a_3a_loop:
	movq	(%eax), %mm2
	movq	(%edx), %mm3
	
	movq	%mm2, %mm4
	psubusb %mm3, %mm4
	psubb	%mm4, %mm2
	movq	%mm2, %mm1
	movq	%mm1, (%edi)
	addl	%ebx, %eax
	addl	%ebx, %edx
	addl	%ebx, %edi
	subl	$ 2, %ecx
	jge	.darken_pixels_3a_3a_loop
.darken_pixels_3a_3a_last:
	test	$ 1, %ecx
	jz	.darken_pixels_3a_3a_end
	movd	(%eax), %mm2
	movd	(%edx), %mm3
	
	movq	%mm2, %mm4
	psubusb %mm3, %mm4
	psubb	%mm4, %mm2
	movq	%mm2, %mm1
	movd	%mm1, (%edi)	
.darken_pixels_3a_3a_end:		
	
	emms
	popl	%ebx
	popl	%edi
	ret

.globl darken_pixels_1a_1a
.align 16
darken_pixels_1a_1a:
	pushl	%edi
	pushl	%ebx
	movl	12(%esp), %edi
	movq	.alpha_mask_1a, %mm0
	subl	$ 4, %ecx
	jl	.darken_pixels_1a_1a_last3
	movl	$ 8, %ebx
.darken_pixels_1a_1a_loop:
	movq	(%eax), %mm2
	movq	(%edx), %mm3
	
	movq	%mm2, %mm4
	psubusb %mm3, %mm4
	psubb	%mm4, %mm2
	movq	%mm2, %mm1
	movq	%mm1, (%edi)
	addl	%ebx, %eax
	addl	%ebx, %edx
	addl	%ebx, %edi
	subl	$ 4, %ecx
	jge	.darken_pixels_1a_1a_loop

.darken_pixels_1a_1a_last3:
	test	$ 2, %ecx
	jz	.darken_pixels_1a_1a_last1
	movd	(%eax), %mm2
	movd	(%edx), %mm3
	
	movq	%mm2, %mm4
	psubusb %mm3, %mm4
	psubb	%mm4, %mm2
	movq	%mm2, %mm1
	addl	$ 4, %eax
	addl	$ 4, %edx
	addl	$ 4, %edi

.darken_pixels_1a_1a_last1:
	test	$ 1, %ecx
	jz	.darken_pixels_1a_1a_end

	movw	(%eax), %bx
	movd	%ebx, %mm2
	movw	(%edx), %bx
	movd	%ebx, %mm3
	
	movq	%mm2, %mm4
	psubusb %mm3, %mm4
	psubb	%mm4, %mm2
	movq	%mm2, %mm1
	movd	%mm1, %ebx
	movw	%bx, (%edi)

.darken_pixels_1a_1a_end:		
	
	emms
	popl	%ebx
	popl	%edi
	ret


.globl lighten_pixels_3a_3a

.align 16
lighten_pixels_3a_3a:
	pushl	%edi
	pushl	%ebx
	movl	12(%esp), %edi
	movq	.alpha_mask_3a, %mm0
	
	subl	$ 2, %ecx
	jl	.lighten_pixels_3a_3a_last
	movl	$ 8, %ebx
.lighten_pixels_3a_3a_loop:
	movq	(%eax), %mm2
	movq	(%edx), %mm3
	
	movq	%mm2, %mm4
	psubusb %mm3, %mm4
	paddb	%mm4, %mm3
	movq	%mm0, %mm1
	pandn	%mm3, %mm1

	psubb	%mm4, %mm2
	pand	%mm0, %mm2
	por	%mm2, %mm1
	movq	%mm1, (%edi)
	addl	%ebx, %eax
	addl	%ebx, %edx
	addl	%ebx, %edi
	subl	$ 2, %ecx
	jge	.lighten_pixels_3a_3a_loop
.lighten_pixels_3a_3a_last:
	test	$ 1, %ecx
	jz	.lighten_pixels_3a_3a_end
	movd	(%eax), %mm2
	movd	(%edx), %mm3
	
	movq	%mm2, %mm4
	psubusb %mm3, %mm4
	paddb	%mm4, %mm3
	movq	%mm0, %mm1
	pandn	%mm3, %mm1

	psubb	%mm4, %mm2
	pand	%mm0, %mm2
	por	%mm2, %mm1
	movd	%mm1, (%edi)	
.lighten_pixels_3a_3a_end:		
	
	emms
	popl	%ebx
	popl	%edi
	ret

.globl lighten_pixels_1a_1a
.align 16
lighten_pixels_1a_1a:
	pushl	%edi
	pushl	%ebx
	movl	12(%esp), %edi
	movq	.alpha_mask_1a, %mm0
	subl	$ 4, %ecx
	jl	.lighten_pixels_1a_1a_last3
	movl	$ 8, %ebx
.lighten_pixels_1a_1a_loop:
	movq	(%eax), %mm2
	movq	(%edx), %mm3
	
	movq	%mm2, %mm4
	psubusb %mm3, %mm4
	paddb	%mm4, %mm3
	movq	%mm0, %mm1
	pandn	%mm3, %mm1

	psubb	%mm4, %mm2
	pand	%mm0, %mm2
	por	%mm2, %mm1
	movq	%mm1, (%edi)
	addl	%ebx, %eax
	addl	%ebx, %edx
	addl	%ebx, %edi
	subl	$ 4, %ecx
	jge	.lighten_pixels_1a_1a_loop

.lighten_pixels_1a_1a_last3:
	test	$ 2, %ecx
	jz	.lighten_pixels_1a_1a_last1
	movd	(%eax), %mm2
	movd	(%edx), %mm3
	
	movq	%mm2, %mm4
	psubusb %mm3, %mm4
	paddb	%mm4, %mm3
	movq	%mm0, %mm1
	pandn	%mm3, %mm1

	psubb	%mm4, %mm2
	pand	%mm0, %mm2
	por	%mm2, %mm1
	addl	$ 4, %eax
	addl	$ 4, %edx
	addl	$ 4, %edi

.lighten_pixels_1a_1a_last1:
	test	$ 1, %ecx
	jz	.lighten_pixels_1a_1a_end

	movw	(%eax), %bx
	movd	%ebx, %mm2
	movw	(%edx), %bx
	movd	%ebx, %mm3
	
	movq	%mm2, %mm4
	psubusb %mm3, %mm4
	paddb	%mm4, %mm3
	movq	%mm0, %mm1
	pandn	%mm3, %mm1

	psubb	%mm4, %mm2
	pand	%mm0, %mm2
	por	%mm2, %mm1
	movd	%mm1, %ebx
	movw	%bx, (%edi)

.lighten_pixels_1a_1a_end:		
	
	emms
	popl	%ebx
	popl	%edi
	ret


.globl screen_pixels_3a_3a

.align 16
screen_pixels_3a_3a:
	pushl	%edi
	pushl	%ebx
	movl	12(%esp), %edi
	movq	.alpha_mask_3a, %mm0
	
	movq	.mult_shift, %mm7
	pxor	%mm6, %mm6
	subl	$ 2, %ecx
	jl	.screen_pixels_3a_3a_last
	movl	$ 8, %ebx
.screen_pixels_3a_3a_loop:
	movq	(%eax), %mm2
	movq	(%edx), %mm3
	

	pcmpeqb %mm4, %mm4
	psubb	%mm2, %mm4
	pcmpeqb	%mm5, %mm5
	psubb	%mm3, %mm5

	movq	%mm4, %mm1
	punpcklbw %mm6, %mm1
	movq	%mm5, %mm3
	punpcklbw %mm6, %mm3
	pmullw	%mm3, %mm1
	paddw	%mm7, %mm1
	movq	%mm1, %mm3
	psrlw	$ 8, %mm3
	paddw	%mm3, %mm1
	psrlw	$ 8, %mm1

	movq	%mm4, %mm2
	punpckhbw %mm6, %mm2
	movq	%mm5, %mm3
	punpckhbw %mm6, %mm3
	pmullw	%mm3, %mm2
	paddw	%mm7, %mm2
	movq	%mm2, %mm3
	psrlw	$ 8, %mm3
	paddw	%mm3, %mm2
	psrlw	$ 8, %mm2

	packuswb %mm2, %mm1

	pcmpeqb	%mm3, %mm3
	psubb	%mm1, %mm3

	movq	%mm0, %mm1
	pandn	%mm3, %mm1

	movq	%mm2, %mm4
	psubusb %mm5, %mm2
	paddb	%mm2, %mm5
	pcmpeqb	%mm3, %mm3
	psubb	%mm5, %mm3

	pand	%mm0, %mm3
	por	%mm3, %mm1
	movq	%mm1, (%edi)
	addl	%ebx, %eax
	addl	%ebx, %edx
	addl	%ebx, %edi
	subl	$ 2, %ecx
	jge	.screen_pixels_3a_3a_loop
.screen_pixels_3a_3a_last:
	test	$ 1, %ecx
	jz	.screen_pixels_3a_3a_end
	movd	(%eax), %mm2
	movd	(%edx), %mm3
	

	pcmpeqb %mm4, %mm4
	psubb	%mm2, %mm4
	pcmpeqb	%mm5, %mm5
	psubb	%mm3, %mm5

	movq	%mm4, %mm1
	punpcklbw %mm6, %mm1
	movq	%mm5, %mm3
	punpcklbw %mm6, %mm3
	pmullw	%mm3, %mm1
	paddw	%mm7, %mm1
	movq	%mm1, %mm3
	psrlw	$ 8, %mm3
	paddw	%mm3, %mm1
	psrlw	$ 8, %mm1

	movq	%mm4, %mm2
	punpckhbw %mm6, %mm2
	movq	%mm5, %mm3
	punpckhbw %mm6, %mm3
	pmullw	%mm3, %mm2
	paddw	%mm7, %mm2
	movq	%mm2, %mm3
	psrlw	$ 8, %mm3
	paddw	%mm3, %mm2
	psrlw	$ 8, %mm2

	packuswb %mm2, %mm1

	pcmpeqb	%mm3, %mm3
	psubb	%mm1, %mm3

	movq	%mm0, %mm1
	pandn	%mm3, %mm1

	movq	%mm2, %mm4
	psubusb %mm5, %mm2
	paddb	%mm2, %mm5
	pcmpeqb	%mm3, %mm3
	psubb	%mm5, %mm3

	pand	%mm0, %mm3
	por	%mm3, %mm1
	movd	%mm1, (%edi)	
.screen_pixels_3a_3a_end:		
	
	emms
	popl	%ebx
	popl	%edi
	ret

.globl screen_pixels_1a_1a
.align 16
screen_pixels_1a_1a:
	pushl	%edi
	pushl	%ebx
	movl	12(%esp), %edi
	movq	.alpha_mask_1a, %mm0
	subl	$ 4, %ecx
	jl	.screen_pixels_1a_1a_last3
	movl	$ 8, %ebx
.screen_pixels_1a_1a_loop:
	movq	(%eax), %mm2
	movq	(%edx), %mm3
	

	pcmpeqb %mm4, %mm4
	psubb	%mm2, %mm4
	pcmpeqb	%mm5, %mm5
	psubb	%mm3, %mm5

	movq	%mm4, %mm1
	punpcklbw %mm6, %mm1
	movq	%mm5, %mm3
	punpcklbw %mm6, %mm3
	pmullw	%mm3, %mm1
	paddw	%mm7, %mm1
	movq	%mm1, %mm3
	psrlw	$ 8, %mm3
	paddw	%mm3, %mm1
	psrlw	$ 8, %mm1

	movq	%mm4, %mm2
	punpckhbw %mm6, %mm2
	movq	%mm5, %mm3
	punpckhbw %mm6, %mm3
	pmullw	%mm3, %mm2
	paddw	%mm7, %mm2
	movq	%mm2, %mm3
	psrlw	$ 8, %mm3
	paddw	%mm3, %mm2
	psrlw	$ 8, %mm2

	packuswb %mm2, %mm1

	pcmpeqb	%mm3, %mm3
	psubb	%mm1, %mm3

	movq	%mm0, %mm1
	pandn	%mm3, %mm1

	movq	%mm2, %mm4
	psubusb %mm5, %mm2
	paddb	%mm2, %mm5
	pcmpeqb	%mm3, %mm3
	psubb	%mm5, %mm3

	pand	%mm0, %mm3
	por	%mm3, %mm1
	movq	%mm1, (%edi)
	addl	%ebx, %eax
	addl	%ebx, %edx
	addl	%ebx, %edi
	subl	$ 4, %ecx
	jge	.screen_pixels_1a_1a_loop

.screen_pixels_1a_1a_last3:
	test	$ 2, %ecx
	jz	.screen_pixels_1a_1a_last1
	movd	(%eax), %mm2
	movd	(%edx), %mm3
	

	pcmpeqb %mm4, %mm4
	psubb	%mm2, %mm4
	pcmpeqb	%mm5, %mm5
	psubb	%mm3, %mm5

	movq	%mm4, %mm1
	punpcklbw %mm6, %mm1
	movq	%mm5, %mm3
	punpcklbw %mm6, %mm3
	pmullw	%mm3, %mm1
	paddw	%mm7, %mm1
	movq	%mm1, %mm3
	psrlw	$ 8, %mm3
	paddw	%mm3, %mm1
	psrlw	$ 8, %mm1

	movq	%mm4, %mm2
	punpckhbw %mm6, %mm2
	movq	%mm5, %mm3
	punpckhbw %mm6, %mm3
	pmullw	%mm3, %mm2
	paddw	%mm7, %mm2
	movq	%mm2, %mm3
	psrlw	$ 8, %mm3
	paddw	%mm3, %mm2
	psrlw	$ 8, %mm2

	packuswb %mm2, %mm1

	pcmpeqb	%mm3, %mm3
	psubb	%mm1, %mm3

	movq	%mm0, %mm1
	pandn	%mm3, %mm1

	movq	%mm2, %mm4
	psubusb %mm5, %mm2
	paddb	%mm2, %mm5
	pcmpeqb	%mm3, %mm3
	psubb	%mm5, %mm3

	pand	%mm0, %mm3
	por	%mm3, %mm1
	addl	$ 4, %eax
	addl	$ 4, %edx
	addl	$ 4, %edi

.screen_pixels_1a_1a_last1:
	test	$ 1, %ecx
	jz	.screen_pixels_1a_1a_end

	movw	(%eax), %bx
	movd	%ebx, %mm2
	movw	(%edx), %bx
	movd	%ebx, %mm3
	

	pcmpeqb %mm4, %mm4
	psubb	%mm2, %mm4
	pcmpeqb	%mm5, %mm5
	psubb	%mm3, %mm5

	movq	%mm4, %mm1
	punpcklbw %mm6, %mm1
	movq	%mm5, %mm3
	punpcklbw %mm6, %mm3
	pmullw	%mm3, %mm1
	paddw	%mm7, %mm1
	movq	%mm1, %mm3
	psrlw	$ 8, %mm3
	paddw	%mm3, %mm1
	psrlw	$ 8, %mm1

	movq	%mm4, %mm2
	punpckhbw %mm6, %mm2
	movq	%mm5, %mm3
	punpckhbw %mm6, %mm3
	pmullw	%mm3, %mm2
	paddw	%mm7, %mm2
	movq	%mm2, %mm3
	psrlw	$ 8, %mm3
	paddw	%mm3, %mm2
	psrlw	$ 8, %mm2

	packuswb %mm2, %mm1

	pcmpeqb	%mm3, %mm3
	psubb	%mm1, %mm3

	movq	%mm0, %mm1
	pandn	%mm3, %mm1

	movq	%mm2, %mm4
	psubusb %mm5, %mm2
	paddb	%mm2, %mm5
	pcmpeqb	%mm3, %mm3
	psubb	%mm5, %mm3

	pand	%mm0, %mm3
	por	%mm3, %mm1
	movd	%mm1, %ebx
	movw	%bx, (%edi)

.screen_pixels_1a_1a_end:		
	
	emms
	popl	%ebx
	popl	%edi
	ret
	
.lower_ff:	.int 0x00FF00FF, 0x00FF00FF


.globl overlay_pixels_3a_3a

.align 16
overlay_pixels_3a_3a:
	pushl	%edi
	pushl	%ebx
	movl	12(%esp), %edi
	movq	.alpha_mask_3a, %mm0
	
	movq	.mult_shift, %mm7
	pxor	%mm6, %mm6 
	subl	$ 2, %ecx
	jl	.overlay_pixels_3a_3a_last
	movl	$ 8, %ebx
.overlay_pixels_3a_3a_loop:
	movq	(%eax), %mm2
	movq	(%edx), %mm3
	call op_overlay
	movq	%mm1, (%edi)
	addl	%ebx, %eax
	addl	%ebx, %edx
	addl	%ebx, %edi
	subl	$ 2, %ecx
	jge	.overlay_pixels_3a_3a_loop
.overlay_pixels_3a_3a_last:
	test	$ 1, %ecx
	jz	.overlay_pixels_3a_3a_end
	movd	(%eax), %mm2
	movd	(%edx), %mm3
	call op_overlay
	movd	%mm1, (%edi)	
.overlay_pixels_3a_3a_end:		
	
	emms
	popl	%ebx
	popl	%edi
	ret

.globl overlay_pixels_1a_1a
.align 16
overlay_pixels_1a_1a:
	pushl	%edi
	pushl	%ebx
	movl	12(%esp), %edi
	movq	.alpha_mask_1a, %mm0
	subl	$ 4, %ecx
	jl	.overlay_pixels_1a_1a_last3
	movl	$ 8, %ebx
.overlay_pixels_1a_1a_loop:
	movq	(%eax), %mm2
	movq	(%edx), %mm3
	call op_overlay
	movq	%mm1, (%edi)
	addl	%ebx, %eax
	addl	%ebx, %edx
	addl	%ebx, %edi
	subl	$ 4, %ecx
	jge	.overlay_pixels_1a_1a_loop

.overlay_pixels_1a_1a_last3:
	test	$ 2, %ecx
	jz	.overlay_pixels_1a_1a_last1
	movd	(%eax), %mm2
	movd	(%edx), %mm3
	call op_overlay
	addl	$ 4, %eax
	addl	$ 4, %edx
	addl	$ 4, %edi

.overlay_pixels_1a_1a_last1:
	test	$ 1, %ecx
	jz	.overlay_pixels_1a_1a_end

	movw	(%eax), %bx
	movd	%ebx, %mm2
	movw	(%edx), %bx
	movd	%ebx, %mm3
	call op_overlay
	movd	%mm1, %ebx
	movw	%bx, (%edi)

.overlay_pixels_1a_1a_end:		
	
	emms
	popl	%ebx
	popl	%edi
	ret

op_overlay:
	movq	%mm2, %mm1
	punpcklbw %mm6, %mm1
	movq	%mm3, %mm5
	punpcklbw %mm6, %mm5
	pmullw	%mm5, %mm1
	paddw	%mm7, %mm1
	movq	%mm1, %mm5
	psrlw	$ 8, %mm5
	paddw	%mm5, %mm1
	psrlw	$ 8, %mm1

	pcmpeqb	%mm4, %mm4
	psubb	%mm2, %mm4
	punpcklbw %mm6, %mm4
	pcmpeqb	%mm5, %mm5
	psubb	%mm3, %mm5
	punpcklbw %mm6, %mm5
	pmullw	%mm5, %mm4
	paddw	%mm7, %mm4
	movq	%mm4, %mm5
	psrlw	$ 8, %mm5
	paddw	%mm5, %mm4
	psrlw	$ 8, %mm4

	movq	.lower_ff, %mm5
	psubw	%mm4, %mm5

	psubw	%mm1, %mm5
	movq	%mm2, %mm4
	punpcklbw %mm6, %mm4
	pmullw	%mm4, %mm5
	paddw	%mm7, %mm5
	movq	%mm5, %mm4
	psrlw	$ 8, %mm4
	paddw	%mm4, %mm5
	psrlw	$ 8, %mm5
	paddw	%mm1, %mm5

	subl	$ 8, %esp
	movq	%mm5, (%esp)

	movq	%mm2, %mm1
	punpckhbw %mm6, %mm1
	movq	%mm3, %mm5
	punpckhbw %mm6, %mm5
	pmullw	%mm5, %mm1
	paddw	%mm7, %mm1
	movq	%mm1, %mm5
	psrlw	$ 8, %mm5
	paddw	%mm5, %mm1
	psrlw	$ 8, %mm1

	pcmpeqb	%mm4, %mm4
	psubb	%mm2, %mm4
	punpckhbw %mm6, %mm4
	pcmpeqb	%mm5, %mm5
	psubb	%mm3, %mm5
	punpckhbw %mm6, %mm5
	pmullw	%mm5, %mm4
	paddw	%mm7, %mm4
	movq	%mm4, %mm5
	psrlw	$ 8, %mm5
	paddw	%mm5, %mm4
	psrlw	$ 8, %mm4

	movq	.lower_ff, %mm5
	psubw	%mm4, %mm5

	psubw	%mm1, %mm5
	movq	%mm2, %mm4
	punpckhbw %mm6, %mm4
	pmullw	%mm4, %mm5
	paddw	%mm7, %mm5
	movq	%mm5, %mm4
	psrlw	$ 8, %mm4
	paddw	%mm4, %mm5
	psrlw	$ 8, %mm5
	paddw	%mm1, %mm5

	movq	(%esp), %mm4
	addl	$ 8, %esp

	packuswb %mm5, %mm4
	movq	%mm0, %mm1
	pandn	%mm4, %mm1

	movq	%mm2, %mm4
	psubusb %mm3, %mm4
	psubb	%mm4, %mm2
	pand	%mm0, %mm2
	por	%mm2, %mm1
	ret