			# This code was produced by the Mattia La Mura
.data
FormatString1:	.string "%llu"	# used by printf to display 64-bit unsigned integers
FormatString2:	.string "%lf"	# used by printf to display 64-bit floating point numbers
FormatString3:	.string "%c"	# used by printf to display a 8-bit single character
TrueString:	.string "TRUE"	# used by printf to display the boolean value TRUE
FalseString:	.string "FALSE"	# used by printf to display the boolean value FALSE
ForDoubleIncrementation: .double 1.0
SINorCOS: .double 0.0
ForRadiant: .double 0.01745392		#=(pigreque/180)
a:	.quad 0
b:	.quad 0
cht:	.byte 0
tdbl:	.double 0.0
	.align 8
	.text		# The following lines contain the program
	.globl main	# The main function must be visible from outside
main:			# The main function body :
	movq %rsp, %rbp	# Save the position of the stack's top
	push $1
	pop a
	push $6
	pop b
	movq $0, %rax
	movb $'a',%al
	push %rax
	pop %rax
	movb %al,cht
While0:
	push a
	push $12
	pop %rax
	pop %rbx
	cmpq %rax, %rbx
	jb Vrai2	# If below
	push $0		# False
	jmp Suite2
Vrai2:	push $0xFFFFFFFFFFFFFFFF		# True
Suite2:
	pop %rax	# Get the result of expression
	cmpq $0, %rax
	je EndWhile0	# if FALSE, sortir de la boucle0
	push a
	push $2
	movq $0, %rdx
	div %rbx
	push %rdx	# MOD
	push $0
	pop %rax
	pop %rbx
	cmpq %rax, %rbx
	je Vrai4	# If equal
	push $0		# False
	jmp Suite4
Vrai4:	push $0xFFFFFFFFFFFFFFFF		# True
Suite4:
	pop %rax	# Get the result of expression
	cmpq $0, %rax
	je Else2	# if FALSE, jump to Else2
	push a
	pop %rdx	# valeur à afficher
	movq $FormatString1, %rsi	# "%llu\n"
	movl	$1, %edi
	movl	$0, %eax
	call	__printf_chk@PLT
	jmp Next2	# Do not execute the else statement
Else2:
	push a
	push $2
	pop %rbx
	pop %rax
	mulq	%rbx
	push %rax	# MUL
	pop %rdx	# valeur à afficher
	movq $FormatString1, %rsi	# "%llu\n"
	movl	$1, %edi
	movl	$0, %eax
	call	__printf_chk@PLT
Next2:
	push a
	push $1
	pop %rbx
	pop %rax
	addq	%rbx, %rax	# ADD
	push %rax
	pop a
	push a
	pop %rdx	# valeur à afficher
	movq $FormatString1, %rsi	# "%llu\n"
	movl	$1, %edi
	movl	$0, %eax
	call	__printf_chk@PLT
	jmp While0
EndWhile0:
	movq $0, %rax
	movb $'\n',%al
	push %rax
	pop %rsi			# get character in the 8 lowest bits of %si
	movq $FormatString3, %rdi	# "%c\n"
	movl	$0, %eax
	call	printf@PLT
	movl	$1, %edi
	movl	$0, %eax
	call	__printf_chk@PLT
ForInit8:
	subq $8,%rsp
	movl	$0, (%rsp)
	movl	$0, 4(%rsp)
	pop tdbl
For8:
	subq $8,%rsp
	movl	$0, (%rsp)
	movl	$1076494336, 4(%rsp)
	pop %rax	
	cmpq tdbl, %rax	
	jnae ForEND8	
	push tdbl
	push ForDoubleIncrementation
	fldl	8(%rsp)	
	fldl	(%rsp)	# first operand -> %st(0) ; second operand -> %st(1)
	faddp	%st(0),%st(1)	# %st(0) <- op1 + op2 ; %st(1)=null
	fstpl 8(%rsp)
	addq	$8, %rsp	
	pop tdbl
	push cht
	pop %rsi			# get character in the 8 lowest bits of %si
	movq $FormatString3, %rdi	# "%c\n"
	movl	$0, %eax
	call	printf@PLT
	movl	$1, %edi
	movl	$0, %eax
	call	__printf_chk@PLT
	push tdbl
	movsd	(%rsp), %xmm0		# &stack top -> %xmm0
	subq	$16, %rsp		# allocation for 3 additional doubles
	movsd %xmm0, 8(%rsp)
	movq $FormatString2, %rdi	# "%lf\n"
	movq	$1, %rax
	call	printf
nop
	addq $24, %rsp			# pop nothing
	movl	$1, %edi
	movl	$0, %eax
	call	__printf_chk@PLT
	movq $0, %rax
	movb $'\n',%al
	push %rax
	pop %rsi			# get character in the 8 lowest bits of %si
	movq $FormatString3, %rdi	# "%c\n"
	movl	$0, %eax
	call	printf@PLT
	movl	$1, %edi
	movl	$0, %eax
	call	__printf_chk@PLT
	jmp For8
ForEND8:
	movq %rbp, %rsp		# Restore the position of the stack's top
	ret			# Return from main function
