#ifndef __TEST_BASIC_TYPE02_H
#define __TEST_BASIC_TYPE02_H

#ifdef __cplusplus
extern "C" {
#endif

//include/uapi/asm-generic/int-ll64.h
typedef __signed__ char 	__s8;
typedef unsigned char 		__u8;
typedef __signed__ short 	__s16;
typedef unsigned short 		__u16;
typedef __signed__ int 		__s32;
typedef unsigned int 		__u32;

#ifdef __GNUC__
__extension__ typedef __signed__ long long 	__s64;
__extension__ typedef unsigned long long 	__u64;
#else
typedef __signed__ long long 	__s64;
typedef unsigned long long 		__u64;
#endif

//include/asm-generic/int-ll64.h
typedef __s8 	s8;
typedef __u8 	u8;
typedef __s16 	s16;
typedef __u16 	u16;
typedef __s32 	s32;
typedef __u32 	u32;
typedef __s64 	s64;
typedef __u64 	u64;


#ifdef __cplusplus
}
#endif

#endif // __TEST_BASIC_TYPE02_H
