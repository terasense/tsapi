#pragma once

#define BUG_ON(cond) do {} while(0)

#define BUILD_BUG_ON(cond) extern void __build_bug_on_dummy(char a[1 - 2*!!(cond)])

