/* $Id: datasock_codes.h,v 1.1.1.1 2001/11/20 02:09:54 redi Exp $	*/
/************************************************************************/
/*  Code numbers used for datasock requests.				*/
/************************************************************************/

#define	DATASOCK_PASSWD_CODE	100
#define	DATASOCK_CHANNEL_CODE	110
#define	DATASOCK_EOT_CODE	199

/************************************************************************/
/*  Keywords used in datasock requests.					*/
/************************************************************************/
#define	PASSWD_KEYWD		"PASSWD"
#define	EOT_KEYWD		"EOT"

/************************************************************************/
/* Flags useful for defining datasock transactions.			*/
/************************************************************************/
#define	    SOCKET_REQUEST_CHANNELS	1
#define	    SOCKET_REQUEST_PASSWD	2

