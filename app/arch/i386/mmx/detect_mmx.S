.data
 cpu_flags: .long 0

.text
.align 4
.globl intel_cpu_features

intel_cpu_features:
        pushl   %ebx
	pushfl
	popl %eax

	movl %eax,%ecx

	xorl $0x040000,%eax
	pushl %eax

	popfl
	pushfl

	popl %eax
	xorl %ecx,%eax
	jz .intel_cpu_features_end   # Processor is 386

	pushl %ecx
	popfl

	movl %ecx,%eax
	xorl $0x200000,%eax

	pushl %eax
	popfl
	pushfl

	popl %eax
	xorl %ecx,%eax
	je .intel_cpu_features_end

	pushal

	movl $1,%eax
	cpuid

	movl %edx,cpu_flags

	popal

	movl cpu_flags,%eax

.intel_cpu_features_end:
        popl    %ebx
        ret
