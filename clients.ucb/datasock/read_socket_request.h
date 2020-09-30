/* $Id: read_socket_request.h,v 1.1.1.1 2001/11/20 02:09:53 redi Exp $	*/
int read_socket_request (int sd,	/* Input socket file descriptor.*/
			 STATCHAN **pws,/* ptr to STATCHAN structure.	*/
			 int *pnws,	/* ptr to # STATCHAN entries.	*/
			 int flag,	/* request flag.		*/
			 char *passwd);	/* optional password to match.	*/
