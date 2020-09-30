#include <stdio.h>
#include <errno.h>
#include "stuff.h"

#define ERRBUFLEN 256

int syserr(char *msg)
{
  int errnum = errno;
  char errbuf[ERRBUFLEN];
  char const *errmsg = strerror_buf(errnum, errbuf, ERRBUFLEN);
  fprintf(stdout,"ERROR: %s ( errno: %d",errmsg,errnum);
  fflush(stdout);
  return(1);
}

void fatalsyserr(char *msg)
{
  int errnum = errno;
  char errbuf[ERRBUFLEN];
  char const *errmsg = strerror_buf(errnum, errbuf, ERRBUFLEN);
  fprintf(stdout,"FATAL ERROR: %s ( errno: %d",errmsg,errnum);
  fflush(stdout);
  exit(1);
}

