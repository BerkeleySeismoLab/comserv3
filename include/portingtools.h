#ifndef _PORTINGTOOLS_H_
#define _PORTINGTOOLS_H_

/*
 * 29 Sep 2020 DSN Updated for comserv3.
 */

#ifdef __cplusplus
extern "C" {
#endif
size_t strlcpy(char *dst, const char *src, size_t destsize);
size_t strlcat(char *dst, const char *src, size_t size);
void SwapDouble( double *data );
#ifdef __cplusplus
}
#endif

#endif
