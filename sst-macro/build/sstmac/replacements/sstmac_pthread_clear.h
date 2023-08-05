
#ifdef SSTMAC_PTHREAD_MACRO_H 

//if sstmac pthread included, clear it
#include <sstmac/libraries/pthread/sstmac_pthread_clear_macros.h>
#define PTHREAD_MUTEX_INITIALIZER { { 0, 0, 0, 0, PTHREAD_MUTEX_TIMED_NP, 0, 0, { 0, 0 } } }
#define PTHREAD_COND_INITIALIZER { { {0}, {0}, {0, 0}, {0, 0}, 0, 0, {0, 0} } }
#define PTHREAD_ONCE_INIT 0


#endif
