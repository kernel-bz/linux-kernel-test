#ifndef __TEST_BASIC_H
#define __TEST_BASIC_H

#ifdef __cplusplus
extern "C" {
#endif

//test/basic/basic-test.c
void basic_run_time_test(void);

//test/basic/ptr-test.c
void basic_ptr_test(void);

//test/basic/types-test.c
void basic_types_test(void);
void cpus_mask_test(void);


#ifdef __cplusplus
}
#endif

#endif // __TEST_BASIC_H
