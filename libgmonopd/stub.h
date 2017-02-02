#ifndef STUB_H
#define STUB_H

#include <stdio.h>
#include <unistd.h>

#ifndef TRUE
#  define TRUE (1)
#endif
#ifndef FALSE
#  define FALSE (0)
#endif

#define STUB_FUNCTION printf("\tSTUB: %s at " __FILE__ ", line %d, thread %d\n",__FUNCTION__,__LINE__,getpid())

#define SEGV printf("\tSEGV requested: %s at " __FILE__ ", line %d, thread %d\n",__FUNCTION__,__LINE__,getpid());printf("%d\n", *((int *)NULL))

#ifdef EXTREME_DEBUG_MESSAGES
#  define EENTER tprintf("Entering " __FUNCTION__ "() ...\n")
#  define EEXIT tprintf("Exiting " __FUNCTION__ "() ...\n")
#else
#  define EENTER
#  define EEXIT
#endif

#ifdef DEBUG_MESSAGES
#  define ENTER tprintf("entering " __FUNCTION__ "() ...\n")
#  define EXIT  tprintf("exiting " __FUNCTION__ "() ...\n")
#else
#  define ENTER
#  define EXIT
#endif

#endif
