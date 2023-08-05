#ifndef SSTMAC_SYS_TYPES_H
#define SSTMAC_SYS_TYPES_H

#ifndef SSTMAC_INSIDE_STL
#define SSTMAC_INSIDE_STL

#ifdef SSTMAC_PTHREAD_MACRO_H
//if sstmac pthread included, clear it
#include <sstmac/libraries/pthread/sstmac_pthread_clear_macros.h>
#endif

#include "/usr/include/x86_64-linux-gnu/sys/types.h"
#undef SSTMAC_INSIDE_STL

#ifdef SSTMAC_PTHREAD_MACRO_H
//if sstmac pthread included, bring it back
#undef SSTMAC_PTHREAD_MACRO_H
#include <sstmac/libraries/pthread/sstmac_pthread_macro.h>
#endif

#else
#include "/usr/include/x86_64-linux-gnu/sys/types.h"
#endif

#endif // SSTMAC_SYS_TYPES_H
