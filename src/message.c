/* SunPTV-USB   (c) 2016 trinity19683
  message (Linux OSes)
  message.c
  2016-01-11
*/

#include <stdio.h>
#include <stdarg.h>
#include "message.h"

void u_debugMessage(const unsigned int flags, const char* FuncName, const unsigned int Line, const int retCode, const char* fmt, ...)
{
	va_list ap;

	if(0 != FuncName) {
		fprintf(stderr, "@%s ", FuncName);
	}
	if(0 != Line) {
		fprintf(stderr, "L%u ", Line);
	}

	va_start(ap, fmt);
	vfprintf(stderr, fmt, ap);
	va_end(ap);

	if(0 != retCode) {
		fprintf(stderr, ", ERR=%d", retCode);
	}
	if(flags & 0x1U) {
		fprintf(stderr, "\n");
	}
}

void dumpHex(char* const buf, const unsigned buflen, const int addr, const void* const dptr, unsigned dsize)
{
	int idx = 0, ret;
	const unsigned char* ptr = dptr;
	if(0 <= addr) {
		ret = sprintf(buf, "%04X: ", addr);
		idx += ret;
	}
	if(dsize * 3 > buflen - idx) {
		warn_info(0,"buffer overflow");
		return;
	}
	while(dsize) {
		ret = sprintf(&buf[idx], "%02X", *ptr);
		idx += ret;
		ptr++;
		dsize--;
		if(dsize) {
			buf[idx] = ' ';
			idx++;
		}
	}
	buf[idx] = 0;
}

/*EOF*/
