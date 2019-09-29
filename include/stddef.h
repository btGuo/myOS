#ifndef __STDDEF_H
#define __STDDEF_H

#ifdef __cplusplus
extern "C" {
#endif

#include <sys/types.h>

#ifndef NULL
#define NULL ((void*)0)
#endif

#define offsetof(TYPE, MEMBER) \
((size_t) &((TYPE *)0)->MEMBER)


#ifdef __cplusplus
}
#endif

#endif
