#ifndef TS_DEBUG
#define TS_DEBUG

#define BUILD_BUG_ON(cond) extern void __build_bug_on_dummy(int a[1 - 2*!!(cond)])

#endif

