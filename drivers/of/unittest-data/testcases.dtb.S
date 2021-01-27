#include <asm-generic/vmlinux.lds.h>
.section .dtb.init.rodata,"a"
.balign STRUCT_ALIGNMENT
.global __dtb_testcases_begin
__dtb_testcases_begin:
.incbin "drivers/of/unittest-data/testcases.dtb" 
__dtb_testcases_end:
.global __dtb_testcases_end
.balign STRUCT_ALIGNMENT
