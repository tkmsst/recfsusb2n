/* fsusb2i   (c) 2015 trinity19683
  OS dependent
  osdepend.h
  2015-12-12
*/
#pragma once
#include "types_u.h"

void miliWait(unsigned);
void* uHeapAlloc(size_t);
void uHeapFree(void* const ptr);
int uthread_mutex_init(PMUTEX *);
int uthread_mutex_lock(PMUTEX);
int uthread_mutex_unlock(PMUTEX);
int uthread_mutex_destroy(PMUTEX);

/*EOF*/