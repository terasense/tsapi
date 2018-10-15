#ifndef TS_TYPES
#define TS_TYPES

#include <ts_debug.h>

typedef signed   char  s8;
typedef unsigned char  u8;
typedef signed   short s16;
typedef unsigned short u16;

BUILD_BUG_ON(sizeof(s8) != 1);
BUILD_BUG_ON(sizeof(u8) != 1);
BUILD_BUG_ON(sizeof(s16) != 2);
BUILD_BUG_ON(sizeof(u16) != 2);

#define fld_size(T, fld) sizeof(((T*)0)->fld)

#endif

