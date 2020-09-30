#ifndef __TEST_DEFINE_USR_H
#define __TEST_DEFINE_USR_H

#ifdef __cplusplus
extern "C" {
#endif

#define notrace
#define __randomize_layout
#define ____cacheline_aligned
#define __cpuidle
#define __init
#define __maybe_unused


#define lockdep_assert_held(l)			do { (void)(l); } while (0)

#ifdef __cplusplus
}
#endif

#endif // __TEST_DEFINE_USR_H
