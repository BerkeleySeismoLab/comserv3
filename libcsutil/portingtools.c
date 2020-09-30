/*
 * Porting tool for linux.
 *
 * 29 Sep 2020 DSN Updated for comserv3.
 */
#include <string.h>

/* I stole this from 
  http://www.chiark.greenend.org.uk/ucgi/~richardk/cvsweb/strlcpy/rjk-strlcat.c
  we should find a real open source, or otherwize available one
*/

/*
size_t strlcpy(char *dst, const char *src, size_t destsize) {
  if(strlen(src) > destsize -1) {
    strncpy(dst, src, destsize-1);
    return destsize-1;
  } else {
    strcpy(dst, src);
    return strlen(src);
  }
}
*/
/* copy src to dst, guaranteeing a null terminator

   If src is too big, truncate it.

   Return strlen(src).

*/

void SwapDouble( double *data )
{
  char temp;

  union {
      char   c[8];
  } dat;

  memcpy( &dat, data, sizeof(double) );
  temp     = dat.c[0];
  dat.c[0] = dat.c[7];
  dat.c[7] = temp;

  temp     = dat.c[1];
  dat.c[1] = dat.c[6];
  dat.c[6] = temp;

  temp     = dat.c[2];
  dat.c[2] = dat.c[5];
  dat.c[5] = temp;

  temp     = dat.c[3];
  dat.c[3] = dat.c[4];
  dat.c[4] = temp;
  memcpy( data, &dat, sizeof(double) );
  return;
}

size_t strlcpy(char *dst, const char *src,  size_t size) {
  size_t n = size;

  /* copy bytes from src to dst.
     if there's no space left, stop copying
     if we copy a '\0', stop copying */
  while(n > 0 && (*dst++ = *src++))
    --n;

  n = size - n;
  
  if(n == size) {
    /* overflow; so truncate the string, and... */
    if(size)
      dst[-1] = 0;
    /* ...work out what the length would have been had there been
       space in the buffer */
#if STRLEN_FASTER
    n += strlen(src);
#else
    {
      const char *s;
      s = src;
      while(*src++)
	;
      n += src - s - 1;
    }
#endif
  }
  
  return n;
}

size_t strlcat(char *dst, const char *src, size_t size) {
  size_t n = 0;

  /* find the end of the string in dst */
#if STRLEN_FASTER
  if(!size)
    return strlen(src);
  n = strlen(dst);
  dst += n;
#else
  while(n < size && *dst++)
    ++n;

  if(n >= size)
    return size + strlen(src);
  /* back up over the '\0' */
  --dst;
#endif
  
  /* copy bytes from src to dst.
     if there's no space left, stop copying
     if we copy a '\0', stop copying */
  while(n < size) {
    if(!(*dst++ = *src++))
      return n;
    ++n;
  }

  if(n == size) {
    /* overflow; so truncate the string, and... */
    if(size)
      dst[-1] = 0;
    /* ...work out what the length would have been had there been
       space in the buffer */
    n += strlen(src);
  }
  
  return n;
}
