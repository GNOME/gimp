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

define(`MMX_PIXEL_OP_3A_1A', `
.globl $1_pixels_3a_3a

.align 16
$1_pixels_3a_3a:
	pushl	%edi
	pushl	%ebx
	movl	12(%esp), %edi
	movq	.alpha_mask_3a, %mm0
	$2
	subl	$ 2, %ecx
	jl	.$1_pixels_3a_3a_last
	movl	$ 8, %ebx
.$1_pixels_3a_3a_loop:
	movq	(%eax), %mm2
	movq	(%edx), %mm3
	$3
	movq	%mm1, (%edi)
	addl	%ebx, %eax
	addl	%ebx, %edx
	addl	%ebx, %edi
	subl	$ 2, %ecx
	jge	.$1_pixels_3a_3a_loop
.$1_pixels_3a_3a_last:
	test	$ 1, %ecx
	jz	.$1_pixels_3a_3a_end
	movd	(%eax), %mm2
	movd	(%edx), %mm3
	$3
	movd	%mm1, (%edi)	
.$1_pixels_3a_3a_end:		
	$4
	emms
	popl	%ebx
	popl	%edi
	ret

.globl $1_pixels_1a_1a
.align 16
$1_pixels_1a_1a:
	pushl	%edi
	pushl	%ebx
	movl	12(%esp), %edi
	movq	.alpha_mask_1a, %mm0
	subl	$ 4, %ecx
	jl	.$1_pixels_1a_1a_last3
	movl	$ 8, %ebx
.$1_pixels_1a_1a_loop:
	movq	(%eax), %mm2
	movq	(%edx), %mm3
	$3
	movq	%mm1, (%edi)
	addl	%ebx, %eax
	addl	%ebx, %edx
	addl	%ebx, %edi
	subl	$ 4, %ecx
	jge	.$1_pixels_1a_1a_loop

.$1_pixels_1a_1a_last3:
	test	$ 2, %ecx
	jz	.$1_pixels_1a_1a_last1
	movd	(%eax), %mm2
	movd	(%edx), %mm3
	$3
	addl	$ 4, %eax
	addl	$ 4, %edx
	addl	$ 4, %edi

.$1_pixels_1a_1a_last1:
	test	$ 1, %ecx
	jz	.$1_pixels_1a_1a_end

	movw	(%eax), %bx
	movd	%ebx, %mm2
	movw	(%edx), %bx
	movd	%ebx, %mm3
	$3
	movd	%mm1, %ebx
	movw	%bx, (%edi)

.$1_pixels_1a_1a_end:		
	$4
	emms
	popl	%ebx
	popl	%edi
	ret')

/* min(a,b) = a - max(a-b, 0) */
MMX_PIXEL_OP_3A_1A(`add', `', `
	movq	%mm2, %mm4
	paddusb %mm3, %mm4
	movq	%mm0, %mm1
	pandn	%mm4, %mm1
	movq	%mm2, %mm4
	psubusb %mm3, %mm4
	psubb	%mm4, %mm2
	pand	%mm0, %mm2
	por	%mm2, %mm1', `')

MMX_PIXEL_OP_3A_1A(`substract', `', `
	movq	%mm2, %mm4
	psubusb %mm3, %mm4
	movq	%mm0, %mm1
	pandn	%mm4, %mm1
	psubb	%mm4, %mm2
	pand	%mm0, %mm2
	por	%mm2, %mm1', `')

MMX_PIXEL_OP_3A_1A(`difference', `', `
	movq	%mm2, %mm4
	movq	%mm3, %mm5
	psubusb %mm3, %mm4
	psubusb	%mm2, %mm5
	movq	%mm0, %mm1
	paddb	%mm5, %mm4
	pandn	%mm4, %mm1
	psubb	%mm4, %mm2
	pand	%mm0, %mm2
	por	%mm2, %mm1', `')

MMX_PIXEL_OP_3A_1A(`multiply', `
	movq	.mult_shift, %mm7
	pxor	%mm6, %mm6',`

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
	por	%mm2, %mm1', `')

/* Could be perhaps more optimized */
MMX_PIXEL_OP_3A_1A(`darken', `', `
	movq	%mm2, %mm4
	psubusb %mm3, %mm4
	psubb	%mm4, %mm2
	movq	%mm2, %mm1', `')

MMX_PIXEL_OP_3A_1A(`lighten', `', `
	movq	%mm2, %mm4
	psubusb %mm3, %mm4
	paddb	%mm4, %mm3
	movq	%mm0, %mm1
	pandn	%mm3, %mm1

	psubb	%mm4, %mm2
	pand	%mm0, %mm2
	por	%mm2, %mm1', `')

MMX_PIXEL_OP_3A_1A(`screen', `
	movq	.mult_shift, %mm7
	pxor	%mm6, %mm6',`

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
	por	%mm3, %mm1', `')
	
.lower_ff:	.int 0x00FF00FF, 0x00FF00FF

MMX_PIXEL_OP_3A_1A(`overlay', `
	movq	.mult_shift, %mm7
	pxor	%mm6, %mm6 ',
	`call op_overlay', `')

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