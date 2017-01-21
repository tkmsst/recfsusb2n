/* fsusb2i   (c) 2015 trinity19683
  OS dependent (Linux OSes)
  osdepend.c
  2015-12-20
*/

#include <pthread.h>
#include <malloc.h>
#include <errno.h>
#include <unistd.h>
#include "osdepend.h"

void miliWait(unsigned msec)
{
	usleep(msec * 1000U);
}

void* uHeapAlloc(size_t sz)
{
	return valloc( sz );
}

void uHeapFree(void* const ptr)
{
	free( ptr );
}

int uthread_mutex_init(PMUTEX *p)
{
	if(NULL == p)
		return sizeof(pthread_mutex_t);
	if(NULL == *p) {
		void* const ptr = malloc(sizeof(pthread_mutex_t));
		if(NULL == ptr)
			return -ENOMEM;
		*p = ptr;
	}
	return pthread_mutex_init(*p, NULL);
}

int uthread_mutex_lock(PMUTEX p)
{
	return pthread_mutex_lock(p);
}

int uthread_mutex_unlock(PMUTEX p)
{
	return pthread_mutex_unlock(p);
}

int uthread_mutex_destroy(PMUTEX p)
{
	if(NULL == p) return -EINVAL;
	const int ret = pthread_mutex_destroy(p);
	if(0 != ret) return ret;
	free(p);
	return 0;
}

/*EOF*/