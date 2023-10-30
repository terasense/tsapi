#pragma once

//
// Helper macro
//

#define OFFSET_OF(T, F) ((size_t)&((T*)(0))->F)
#define SIZE_WITH(T, F) (OFFSET_OF(T, F) + sizeof(((T*)(0))->F))

#define ARR_SZ(a) (sizeof(a)/sizeof(a[0]))
#define STRZ_LEN(s) (ARR_SZ(s)-1)

#define xstr(a) _str(a)
#define _str(a) #a
