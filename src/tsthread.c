/* SunPTV-USB   (c) 2016 trinity19683
  TS USB I/O thread (Linux OSes)
  tsthread.c
  2016-02-05
*/

#include <errno.h>
#include <sys/ioctl.h>
#include <linux/usbdevice_fs.h>
#include <string.h>
#include <poll.h>
#include <pthread.h>

#include "usbops.h"
#include "osdepend.h"
#include "tsbuff.h"
#include "tsthread.h"
#include "message.h"

#define ROUNDUP(n,w) (((n) + (w)) & ~(unsigned)(w))

#define NUM_ISOC_PACKET  16

struct tsthread_param {
	void* thread_ptr;    //# pointer to thread data
	unsigned char volatile  flags;
	/* if 0x01 flagged, issue a new request.
	   if 0x02 flagged, cancel requests and stop thread.
	*/
	const struct usb_endpoint_st*  pUSB;
	void* buffer;    //# data buffer (in heap memory)
	int*  actual_length;    //# actual length of each buffer block
	unsigned buff_unitSize;
	int buff_num;
	int buff_push;
	int buff_pop;
};


static int submitNextURB(struct tsthread_param* const ps, struct usbdevfs_urb* const urbp)
{
	//# isochronous URB request
	int ret = 0, j;

	memset(urbp, 0, sizeof(struct usbdevfs_urb) + sizeof(struct usbdevfs_iso_packet_desc) * NUM_ISOC_PACKET );
	urbp->type = (ps->pUSB->endpoint & 0x100) ? USBDEVFS_URB_TYPE_ISO : USBDEVFS_URB_TYPE_BULK;
	urbp->endpoint = ps->pUSB->endpoint & 0xFF;
	urbp->usercontext = &(ps->actual_length[ps->buff_push]);
	urbp->buffer = ps->buffer + (ps->buff_push * ps->buff_unitSize);

	if(USBDEVFS_URB_TYPE_BULK == urbp->type) {
		urbp->buffer_length = ps->buff_unitSize;
		ps->actual_length[ps->buff_push] = -2;
	}else{
		urbp->number_of_packets = NUM_ISOC_PACKET;
		urbp->flags = USBDEVFS_URB_ISO_ASAP;
		for(j = 0; j < NUM_ISOC_PACKET; j++ ) {
			urbp->iso_frame_desc[j].length = ps->buff_unitSize;
			ps->actual_length[ps->buff_push + j] = -2;
		}
	}

	if(ioctl(ps->pUSB->fd, USBDEVFS_SUBMITURB, urbp) < 0) {
		ret = -errno;
		warn_info(errno,"submitURB failed");
		urbp->buffer = NULL;
	}else{
		int next_index = ps->buff_push;
		if(USBDEVFS_URB_TYPE_BULK == urbp->type) {
			next_index ++;
		}else{
			next_index += NUM_ISOC_PACKET;
		}
		ps->buff_push = (next_index < ps->buff_num) ? next_index : 0;
	}
	return ret;
}

static int reapURB(struct tsthread_param* const ps)
{
	struct usbdevfs_urb *urbp;
	int i = 0, j, ret;

	for(;;) {
		if((ret = ioctl(ps->pUSB->fd, USBDEVFS_REAPURBNDELAY, (void *)(&urbp)) ) < 0) {
			if(EAGAIN == errno || EWOULDBLOCK == errno) {
				//# no more data to be received
				ret = -EAGAIN;
			}else{
				warn_info(errno,"reapURB_ndelay failed");
				ret = -errno;
			}
		}
		if(0 > ret) break;

		//# received
		int* const   pLen = urbp->usercontext;
		if(USBDEVFS_URB_TYPE_BULK == urbp->type) {
			if(0 == urbp->status) {
				//# success
				int actual_length = urbp->actual_length;
				if(ps->buff_unitSize < actual_length) {
					warn_info(actual_length,"reapURB overflow");
					actual_length = ps->buff_unitSize;
				}
				pLen[0] = actual_length;
			}else{
				//# failed
				pLen[0] = 0;
				warn_info(urbp->status,"reapURB");
			}
		}else{
			const int  numPkts = urbp->number_of_packets;
			for(j = 0; j < numPkts; j++ ) {
				struct usbdevfs_iso_packet_desc* const  isocDesc = &(urbp->iso_frame_desc[j]);
				if(0 == isocDesc->status) {
					//# success
					int actual_length = isocDesc->actual_length;
					if(ps->buff_unitSize < actual_length) {
						warn_info(actual_length,"reapURB overflow");
						actual_length = ps->buff_unitSize;
					}
					pLen[j] = actual_length;
				}else{
					//# failed
					pLen[j] = 0;
					warn_info(isocDesc->status,"reapURB");
				}
			}
		}
		//# mark this URB unused
		urbp->buffer = NULL;
		i++;
	}

	if(-EAGAIN != ret) {
		return ret;
	}
	return i;
}

/* TS thread function issues URB requests. */
static void tsthread(void* const param)
{
	struct tsthread_param* const ps = param;
	int ret, i;
	const unsigned  UrbSize = ROUNDUP( sizeof(struct usbdevfs_urb) + sizeof(struct usbdevfs_iso_packet_desc) * NUM_ISOC_PACKET ,0xF);
	void* ptrUrb;

	ptrUrb = uHeapAlloc(UrbSize * TS_MaxNumIO);
	if(! ptrUrb) {
		warn_info(errno,"failed to allocate");
		return;
	}

	ps->buff_push = 0;
	for(i = 0; i < TS_MaxNumIO; i++) {
		struct usbdevfs_urb* const  pUrb = ptrUrb + UrbSize * i;
		//# mark this URB unused
		pUrb->buffer = NULL;
	}

	for(;;) {
		int num = TS_MaxNumIO;
		for(i = 0; i < TS_MaxNumIO; i++) {
			struct usbdevfs_urb* const  pUrb = ptrUrb + UrbSize * i;
			if(! pUrb->buffer) {
				if(ps->flags & 0x01) {
					num--;
				}else{
					//# continue to issue a new URB request
					submitNextURB(ps, pUrb);
				}
			}
		}
		if(0 >= num) {
			//# no request
			break;
		}
		if(ps->flags & 0x02) {
			//# canceled
			reapURB(ps);
			for(i = 0; i < TS_MaxNumIO; i++) {
				struct usbdevfs_urb* const  pUrb = ptrUrb + UrbSize * i;
				if(! pUrb->buffer) continue;
				if(ioctl(ps->pUSB->fd, USBDEVFS_DISCARDURB, pUrb) < 0) {
					warn_info(errno,"failed to discard URB");
				}
			}
			break; 
		}

		struct pollfd pfd;
		pfd.fd = ps->pUSB->fd;
		pfd.events = POLLOUT | POLLERR;
		ret = poll(&pfd, 1, TS_PollTimeout);
		if(0 > ret) {
			warn_info(errno,"poll failed");
			break;
		}else{
			if(0 == ret) {
				//# timeout
				if(!(ps->flags & 0x01))  dmsg("poll timeout");
			}
			if(reapURB(ps) < 0) break;
		}
	}
	uHeapFree(ptrUrb);
	ps->flags |= 0x01;    //# continue = F
}

/* public function */

int tsthread_create(tsthread_ptr* const tptr, const struct usb_endpoint_st* const pusbep)
{
	struct tsthread_param* ps;
	{//#
		const unsigned param_size  = ROUNDUP(sizeof(struct tsthread_param), 0xF);
		const unsigned buffer_size = ROUNDUP(TS_BufSize ,0xF);
		const unsigned unitSize = ROUNDUP(pusbep->xfer_size ,0x1FF);
		const unsigned unitNum = TS_BufSize / unitSize;
		const unsigned actlen_size = sizeof(int) * unitNum;
		unsigned totalSize = param_size + actlen_size + buffer_size;
		void* ptr = uHeapAlloc( totalSize );
		if(! ptr) {
			warn_msg(errno,"failed to allocate TS buffer");
			return -1;
		}
		void* const buffer_ptr = ptr;
		ptr += buffer_size;
		ps = ptr;
		ps->buffer = buffer_ptr;
		ptr += param_size;
		ps->actual_length = ptr;
		//ptr += actlen_size;
		ps->buff_num = unitNum;
		ps->actual_length[0] = -1;   //# the first block is pending
		if(pusbep->endpoint & 0x100) { //# Isochronous
			ps->buff_unitSize = pusbep->xfer_size;
		}else{
			ps->buff_unitSize = unitSize;
		}
	}
	ps->pUSB = pusbep;
	ps->flags = 0;
	ps->buff_pop = 0;

	tsthread_start(ps);

	if(pthread_create((pthread_t *)&ps->thread_ptr, NULL, (void *)tsthread, ps) != 0) {
		warn_info(errno,"pthread_create failed");
		uHeapFree(ps->buffer);
		return -1;
	}
	*tptr = ps;
	return 0;
}

void tsthread_destroy(const tsthread_ptr ptr)
{
	struct tsthread_param* const p = ptr;

	p->flags |= 0x03;    //# continue = F, canceled = T
	if(pthread_join((pthread_t)p->thread_ptr, NULL) != 0) {
		warn_info(errno,"pthread_join failed");
	}
	uHeapFree(p->buffer);
}

void tsthread_start(const tsthread_ptr ptr)
{
	struct tsthread_param* const p = ptr;
	p->flags &= ~0x03;    //# continue = T, canceled = F
	if(p->pUSB->startstopFunc)
		p->pUSB->startstopFunc(p->pUSB->dev, 1);
}

void tsthread_stop(const tsthread_ptr ptr)
{
	struct tsthread_param* const p = ptr;
	p->flags |= 0x01;    //# continue = F
	if(p->pUSB->startstopFunc)
		p->pUSB->startstopFunc(p->pUSB->dev, 0);
}

int tsthread_read(const tsthread_ptr tptr, void ** const ptr)
{
	struct tsthread_param* const ps = tptr;
	int i, j;
	i = tsthread_readable(tptr);
	if(0 >= i) return 0;

	j = ps->buff_pop;
	ps->actual_length[ps->buff_pop] = -1;
	if(ptr) {
		*ptr = ps->buffer + (j * ps->buff_unitSize);
		ps->buff_pop = (ps->buff_num - 1 > j) ? j + 1 : 0;
	}else{
		ps->actual_length[ps->buff_push] = -1;
		ps->buff_pop = ps->buff_push;
	}
	return i;
}

int tsthread_readable(const tsthread_ptr tptr)
{
	struct tsthread_param* const ps = tptr;
	int j = ps->buff_pop;

	if(0 > j || ps->buff_num <= j) {  //# bug check
		warn_info(j,"ts.buff_pop Out of range");
		ps->buff_pop = 0;
		return 0;
	}
	do {  //# skip empty blocks
		if(0 != ps->actual_length[j] ) break;
		if(ps->buff_num -1 > j) {
			j++;
		}else{
			j = 0;
		}
	} while(j != ps->buff_pop);
	ps->buff_pop = j;
	return ps->actual_length[j];
}

int tsthread_wait(const tsthread_ptr tptr, const int timeout)
{
	struct tsthread_param* const ps = tptr;
	int ret;
	struct pollfd pfd;
	pfd.fd = ps->pUSB->fd;
	pfd.events = POLLOUT | POLLERR;
	ret = poll(&pfd, 1, timeout);
	if(0 > ret) {
		warn_info(errno,"poll failed");
	}
	return ret;
}


/*EOF*/
