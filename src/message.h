/* SunPTV-USB   (c) 2016 trinity19683
  message (Linux OSes)
  message.h
  2016-01-11
*/
#pragma once
#ifdef __cplusplus
extern "C" {
#endif
void u_debugMessage(const unsigned int flags, const char* FuncName, const unsigned int Line, const int retCode, const char* fmt, ...);
void dumpHex(char* const buf, const unsigned buflen, const int addr, const void* const dptr, unsigned dsize);
#ifdef __cplusplus
}
#endif

#define msg(...)  fprintf(stderr,__VA_ARGS__)
#define warn_msg(errCode, ...) u_debugMessage(1, 0, 0, errCode, __VA_ARGS__)
#define warn_info(errCode, ...) u_debugMessage(1, __func__, __LINE__, errCode, __VA_ARGS__)

#ifdef DEBUG
#define dmsg(...) u_debugMessage(1, 0, 0, 0, __VA_ARGS__)
#define dmsgn(...) u_debugMessage(0, 0, 0, 0, __VA_ARGS__)
#else
#define dmsg(...)
#define dmsgn(...)
#endif

/*EOF*/
