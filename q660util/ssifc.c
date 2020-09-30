/*
    System Supervisor Access Routines
    Copyright 2016, 2017 Certified Software Corporation

Edit History:
   Ed Date       By  Changes
   -- ---------- --- ---------------------------------------------------
    0 2016-02-04 rdr Created.
*/
#include "pascal.h"
#include "xmlsup.h"
#include "ssifc.h"

typedef struct {
#ifdef X86_WIN32
  SOCKET cpath ; /* command or configuration socket */
  struct sockaddr csockin, csockout ; /* commands socket address descriptors */
#else
  integer cpath ; /* command or configuration socket */
  struct sockaddr csockin, csockout ; /* commands socket address descriptors */
#endif
} tsscom ;

tsscom sscom ;
tmsg msg ;
integer ss_timeout ; /* ms since packet sent */

static word send_seq ;

#ifdef X86_WIN32
void sleepms (integer ms)
begin

  Sleep (ms) ;
end

#else

void sleepms (integer ms)
begin
  struct timespec dly ;

  dly.tv_sec = ms div 1000 ; /* whole seconds */
  ms = ms mod 1000 ; /* remainder ms */
  dly.tv_nsec = ms * 1000000 ; /* convert to nanoseconds */
  nanosleep (addr(dly), NULL) ;
end
#endif

#ifdef DUMPPACKETS
void hexdump(void *pAddressIn, long  lSize)
{
 char szBuf[100];
 long lIndent = 1;
 long lOutLen, lIndex, lIndex2, lOutLen2;
 long lRelPos;
 struct { char *pData; unsigned long lSize; } buf;
 unsigned char *pTmp,ucTmp;
 unsigned char *pAddress = (unsigned char *)pAddressIn;

   buf.pData   = (char *)pAddress;
   buf.lSize   = lSize;

   printf ("\nhexdump size=%d\n",lSize);

   while (buf.lSize > 0)
   {
      pTmp     = (unsigned char *)buf.pData;
      lOutLen  = (int)buf.lSize;
      if (lOutLen > 16)
          lOutLen = 16;

      // create a 64-character formatted output line:
      sprintf(szBuf, " >                            "
                     "                      "
                     "    %08lX", pTmp-pAddress);
      lOutLen2 = lOutLen;

      for(lIndex = 1+lIndent, lIndex2 = 53-15+lIndent, lRelPos = 0;
          lOutLen2;
          lOutLen2--, lIndex += 2, lIndex2++
         )
      {
         ucTmp = *pTmp++;

         sprintf(szBuf + lIndex, "%02X ", (unsigned short)ucTmp);
         if(!isprint(ucTmp))  ucTmp = '.'; // nonprintable char
         szBuf[lIndex2] = ucTmp;

         if (!(++lRelPos & 3))     // extra blank after 4 bytes
         {  lIndex++; szBuf[lIndex+2] = ' '; }
      }

      if (!(lRelPos & 3)) lIndex--;

      szBuf[lIndex  ]   = '<';
      szBuf[lIndex+1]   = ' ';

      printf("%s\n", szBuf);

      buf.pData   += lOutLen;
      buf.lSize   -= lOutLen;
   }
}
#endif



void send_to_ss (void)
begin
  integer err ;

  if (sscom.cpath == INVALID_SOCKET)
    then
      return ;
  msg.seq = send_seq++ ;
  msg.spare = 0 ;
//        printf ("SendTime=%d seq=%d cmd=%X data[0]=%d\n", timestamp (),
//                (integer)(msg.seq), (integer)(msg.command), (integer)(msg.data[0])) ;
  err = sendto(sscom.cpath, (pvoid)addr(msg), msg.datalth + MSG_OVERHEAD, 0, (pvoid)addr(sscom.csockout), sizeof(struct sockaddr)) ;
  ss_timeout = 0 ;
  if (err == SOCKET_ERROR)
    then
      begin
        err =
#ifdef X86_WIN32
               WSAGetLastError() ;
#else
               errno ;
#endif
        if (err == ENOBUFS)
          then
            begin
            end
        else if (err != EWOULDBLOCK)
          then
            begin
            end
      end
end

boolean recv_from_ss (void) /* Returns TRUE if message received */
begin
  longword lth ;
  integer err ;

  if (sscom.cpath == INVALID_SOCKET)
    then
      return FALSE ;
  lth = sizeof(struct sockaddr) ;
  err = recvfrom (sscom.cpath, (pvoid)addr(msg), sizeof(tmsg), 0, (pvoid)addr(sscom.csockin), (pvoid)addr(lth)) ;
#ifdef DUMPPACKETS
  if (err > 0) {
    printf("err=%d data length=%d OVH=%d\n",err,msg.datalth,MSG_OVERHEAD);
    hexdump((void *)&msg, (int)msg.datalth+MSG_OVERHEAD);
  }
#endif
  if (err == SOCKET_ERROR)
    then
      begin
        err =
#ifdef X86_WIN32
               WSAGetLastError() ;
#else
               errno ;
#endif
        if (err != EWOULDBLOCK)
          then
            if (err == ECONNRESET)
              then
                begin
                end
        return FALSE ;
      end
  else if ((err > 0) land (err == (msg.datalth + MSG_OVERHEAD)))
    then
      return TRUE ;
    else
      return FALSE ;
end

/* Returns TRUE if error */
boolean open_ss_socket (pchar pc)
begin
  integer err ;
  integer flag ;
  integer i, good ;
  struct sockaddr_in *psock ;
  longword ip ;
  word port ;
  pchar tmp ;

  tmp = strchr (pc, ':') ; /* Must have port delimeter */
  if (tmp == NIL)
    then
      return TRUE ;
  *tmp = 0 ; /* Terminate IP address */
  ip = inet_addr(pc) ;
  if (ip == INADDR_NONE)
    then
      return TRUE ;
  inc(tmp) ; /* skip to start of port */
  good = sscanf (tmp, "%d", addr(i)) ;
  if ((good != 1) lor (i < 1024) lor (i > 65535))
    then
      return TRUE ;
  port = (word)i ;
  sscom.cpath = socket (AF_INET, SOCK_DGRAM, IPPROTO_UDP) ;
  if (sscom.cpath == INVALID_SOCKET)
    then
      begin
        err =
#ifdef X86_WIN32
               WSAGetLastError() ;
#else
               errno ;
#endif
        printf("Socket Error %d on SS port\r\n", err) ;
        return TRUE ;
      end
  psock = (pointer) addr(sscom.csockout) ;
  memset(psock, 0, sizeof(struct sockaddr)) ;
  psock->sin_family = AF_INET ;
  psock->sin_port = htons(port) ;
  psock->sin_addr.s_addr = ip ;
  flag = 1 ;
#ifdef X86_WIN32
  ioctlsocket (sscom.cpath, FIONBIO, (pvoid)addr(flag)) ;
#else
  flag = fcntl (sscom.cpath, F_GETFL, 0) ;
  fcntl (sscom.cpath, F_SETFL, flag or O_NONBLOCK) ;
#endif
  psock = (pointer) addr(sscom.csockin) ;
  memset(psock, 0, sizeof(struct sockaddr)) ;
  psock->sin_family = AF_INET ;
  psock->sin_port = htons(0) ;
  if (htonl(ip) == 0x7F000001) /* 127.0.0.1 */
    then
      psock->sin_addr.s_addr = ip ; /* Need to be on loopback as well */
    else
      psock->sin_addr.s_addr = INADDR_ANY ;
#ifdef X86_WIN32
  err = bind(sscom.cpath, addr(sscom.csockin), sizeof(struct sockaddr)) ;
  if (err)
#else
  err = bind(sscom.cpath, addr(sscom.csockin), sizeof(struct sockaddr)) ;
  if (err)
#endif
    then
      begin
        err =
#ifdef X86_WIN32
               WSAGetLastError() ;
        closesocket (sscom.cpath) ;
#else
               errno ;
        close (sscom.cpath) ;
#endif
        sscom.cpath = INVALID_SOCKET ;
        printf("Bind Error %d on SS port\r\n", err) ;
        return TRUE ;
      end
  return FALSE ;
end



