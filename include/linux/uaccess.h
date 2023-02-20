/* SPDX-License-Identifier: GPL-2.0 */
#ifndef UACCESS_H
#define UACCESS_H

#include <assert.h>

#include <linux/compiler.h>
#include <uapi/asm-generic/errno-base.h>

#define USER_MEM (1024*1024)
static void *__user_addr_min, *__user_addr_max;

static inline void __chk_user_ptr(const volatile void *p, size_t size)
{
    //assert(p >= __user_addr_min && p + size <= __user_addr_max);
}

#define put_user(x, ptr)					\
({								\
	typeof(ptr) __pu_ptr = (ptr);				\
	__chk_user_ptr(__pu_ptr, sizeof(*__pu_ptr));		\
	WRITE_ONCE(*(__pu_ptr), x);				\
	0;							\
})

#define get_user(x, ptr)					\
({								\
	typeof(ptr) __pu_ptr = (ptr);				\
	__chk_user_ptr(__pu_ptr, sizeof(*__pu_ptr));		\
	x = READ_ONCE(*(__pu_ptr));				\
	0;							\
})

static void volatile_memcpy(volatile char *to, const volatile char *from, 
			    unsigned long n)
{
	while (n--)
		*(to++) = *(from++);
}

//static inline int copy_from_user(void *to, const void __user volatile *from, unsigned long n)
static inline int copy_from_user(void *to, const void volatile *from, unsigned long n)
{
	__chk_user_ptr(from, n);
	volatile_memcpy(to, from, n);
	return 0;
}

//static inline int copy_to_user(void __user volatile *to, const void *from, unsigned long n)
static inline int copy_to_user(void volatile *to, const void *from, unsigned long n)
{
	__chk_user_ptr(to, n);
	volatile_memcpy(to, from, n);
	return 0;
}


//233 lines
//extern __must_check int check_zeroed_user(const void __user *from, size_t size);
extern __must_check int check_zeroed_user(const void *from, size_t size);



//282 lines
static __always_inline __must_check int
//copy_struct_from_user(void *dst, size_t ksize, const void __user *src, size_t usize)
copy_struct_from_user(void *dst, size_t ksize, const void *src, size_t usize)
{
    size_t size = min(ksize, usize);
    size_t rest = max(ksize, usize) - size;

    /* Deal with trailing bytes. */
    if (usize < ksize) {
        memset(dst + size, 0, rest);
    } else if (usize > ksize) {
        int ret = check_zeroed_user(src + size, rest);
        if (ret <= 0)
            return ret ?: -E2BIG;
    }
    /* Copy the interoperable parts of the struct. */
    if (copy_from_user(dst, src, size))
        return -EFAULT;
    return 0;
}



//344 lines
/**
 * probe_kernel_address(): safely attempt to read from a location
 * @addr: address to read from
 * @retval: read into this variable
 *
 * Returns 0 on success, or -EFAULT.
 */
#define probe_kernel_address(addr, retval)		\
    probe_kernel_read(&retval, addr, sizeof(retval))

#ifndef user_access_begin
#define user_access_begin(ptr,len) access_ok(ptr, len)
#define user_access_end() do { } while (0)
#define unsafe_op_wrap(op, err) do { if (unlikely(op)) goto err; } while (0)
#define unsafe_get_user(x,p,e) unsafe_op_wrap(__get_user(x,p),e)
#define unsafe_put_user(x,p,e) unsafe_op_wrap(__put_user(x,p),e)
#define unsafe_copy_to_user(d,s,l,e) unsafe_op_wrap(__copy_to_user(d,s,l),e)
static inline unsigned long user_access_save(void) { return 0UL; }
static inline void user_access_restore(unsigned long flags) { }
#endif

#ifdef CONFIG_HARDENED_USERCOPY
void usercopy_warn(const char *name, const char *detail, bool to_user,
           unsigned long offset, unsigned long len);
void __noreturn usercopy_abort(const char *name, const char *detail,
                   bool to_user, unsigned long offset,
                   unsigned long len);
#endif

#endif /* UACCESS_H */
