	.file	"switch-case.c"
	.option nopic
	.text
	.align	1
	.type	scase1, @function
scase1:
	addi	sp,sp,-32
	sd	s0,24(sp)
	addi	s0,sp,32
	mv	a5,a0
	sw	a5,-20(s0)
	lw	a5,-20(s0)
	sext.w	a5,a5
	beqz	a5,.L2
	lw	a5,-20(s0)
	sext.w	a4,a5
	li	a5,1
	beq	a4,a5,.L3
	j	.L6
.L2:
	li	a5,0
	j	.L5
.L3:
	li	a5,1
	j	.L5
.L6:
	li	a5,3
.L5:
	mv	a0,a5
	ld	s0,24(sp)
	addi	sp,sp,32
	jr	ra
	.size	scase1, .-scase1
	.align	1
	.type	scase2, @function
scase2:
	addi	sp,sp,-32
	sd	s0,24(sp)
	addi	s0,sp,32
	mv	a5,a0
	sw	a5,-20(s0)
	lw	a5,-20(s0)
	sext.w	a4,a5
	li	a5,2
	bgt	a4,a5,.L8
	lw	a5,-20(s0)
	sext.w	a5,a5
	bgtz	a5,.L9
	lw	a5,-20(s0)
	sext.w	a5,a5
	beqz	a5,.L10
	j	.L11
.L8:
	lw	a5,-20(s0)
	sext.w	a4,a5
	li	a5,3
	beq	a4,a5,.L12
	j	.L11
.L10:
	li	a5,0
	j	.L7
.L9:
	li	a5,2
	j	.L7
.L12:
	li	a5,3
	j	.L7
.L11:
.L7:
	mv	a0,a5
	ld	s0,24(sp)
	addi	sp,sp,32
	jr	ra
	.size	scase2, .-scase2
	.section	.rodata
	.align	3
.LC0:
	.string	"ret=%d\n"
	.text
	.align	1
	.globl	main
	.type	main, @function
main:
	addi	sp,sp,-32
	sd	ra,24(sp)
	sd	s0,16(sp)
	addi	s0,sp,32
	li	a0,1
	call	scase1
	mv	a5,a0
	sw	a5,-20(s0)
	lw	a5,-20(s0)
	mv	a1,a5
	lui	a5,%hi(.LC0)
	addi	a0,a5,%lo(.LC0)
	call	printf
	li	a0,2
	call	scase2
	mv	a5,a0
	sw	a5,-20(s0)
	lw	a5,-20(s0)
	mv	a1,a5
	lui	a5,%hi(.LC0)
	addi	a0,a5,%lo(.LC0)
	call	printf
	li	a5,0
	mv	a0,a5
	ld	ra,24(sp)
	ld	s0,16(sp)
	addi	sp,sp,32
	jr	ra
	.size	main, .-main
	.ident	"GCC: (GNU) 8.3.0"
	.section	.note.GNU-stack,"",@progbits
