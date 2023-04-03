/************************************************************************
 *  socket_subs.c
 * 
 * Modification History:
 *  2020-09-29 DSN Updated for comserv3.
 ************************************************************************/

#include <stdio.h>

#include <stdlib.h>
#include <termio.h>
#include <fcntl.h>
#include <string.h>
#include <time.h>
#include <unistd.h>
#include <signal.h>
#include <netdb.h>
#include <math.h>
#include <ctype.h>
#include <sys/types.h>
#include <sys/ipc.h>
#include <sys/sem.h>
#include <sys/shm.h>
#include <sys/time.h>
#include <sys/stat.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>

#include <qlib2.h>

#include "channel_info.h"
#include "datasock_codes.h"
#include "basics.h"

#include "dpstruc.h"
#include "seedstrc.h"
#include "stuff.h"
#include "timeutil.h"
#include "service.h"
#include "cfgutil.h"

/************************************************************************
 *  Externals variables and functions.
 ************************************************************************/
extern FILE *info;		/* Default FILE for messages.		*/
extern char *cmdname;		/* Program name.			*/
extern int verbosity;		/* Verbosity flag.			*/
extern int terminate_proc;	/* Terminate program flag;		*/
extern int nchannel;		/* number of entries in channelv.	*/
extern CHANNEL_INFO **channelv;	/* list of channels.			*/

/*  Signal handler variables and functions.		*/
void finish_handler(int sig);
extern int write_to_ring (seed_record_header *pseed);

const static int MIN_BLKSIZE = 128;
const static int MAX_BLKSIZE = 4096;

/************************************************************************/
/*  vsplit:								*/
/*	Split a string into a token list based on delimiter.		*/
/*	Return SUCCESS or FAILURE.					*/
/************************************************************************/
int vsplit (char *string, char *delim, int *pn, char ***ptokenv)
{
    char *p, *savep, *str;
    *pn = 0;
    p = savep = strdup(string);
    while ((str = strtok(p,delim)) != NULL) {
	p = NULL;
	*ptokenv = (*pn==0) ? 
	    (char **)malloc(sizeof(char *)*(*pn+2)) :
	    (char **)realloc(*ptokenv,sizeof(char *)*(*pn+2));
	if (*ptokenv == NULL) return (FAILURE);
	(*ptokenv)[*pn] = strdup(str);
	(*pn)++;
	(*ptokenv)[*pn] = NULL;
    }
    free (savep);
    return (SUCCESS);
}

/************************************************************************
 *  free_vsplit:
 *	Free space allocated by split.
 ************************************************************************/
void free_vsplit (int n, char **tokenv)
{
    int i;
    for (i=0; i<n; i++) {
	if (tokenv[i]) free (tokenv[i]);
    }
    if (n) free (tokenv);
}

/************************************************************************
 *  open_socket:
 *	Open a socket on the specified machine and service name/number.
 *	Returns:	 open file descriptor
 *			negative value on error.
 ************************************************************************/
int open_socket(char *server, 	/* remote node name */
                char *service) 	/* remote service name */
{
    struct sockaddr_in server_sock;
    struct hostent *hp;
    struct servent *sp;
    unsigned long  ipadd;
    int s;

    /* Create an socket on which to make the connection.		*/
    /* This function call says we want our communications to take	*/
    /* place in the internet domain (AF_INET).				*/
    /* (The other possible domain is the Unix domain).			*/
    s = socket(AF_INET, SOCK_STREAM, 0);
    if (s < 0) {
	printf(" %% Error opening socket in client.\n");
	return(FAILURE);
    }
    /* Get the IP address of the remote client in the "hp" structure.	*/
    /* It must be available to this machine or the call files.		*/
    /* Info can come from hosts file, NIS, DNS, etc.			*/
    if (isalpha(server[0])) {
	hp = gethostbyname(server);	/* probe name server by name */
    } 
    else {
	ipadd = inet_addr(server);	/* convert IP to binary format */
	hp = gethostbyaddr((char*)&ipadd, sizeof(ipadd), AF_INET);	
					/*probe name server by address */
    }

    if (hp == NULL) {
	printf(" %% Remote host is unknown to name server.\n");
	return(FAILURE); 
    } 
    else {
	printf(" %% Remote host: %s\n", hp->h_name);
    }

    /* If specified as a name the service/port must be defined by name	*/
    /* on this system.							*/
    if (isalpha(service[0])) {
	sp = getservbyname(service, "tcp");	/* probe service by name */
	if (sp == NULL) {
	    printf(" %% Service not in local host table.\n");
	    return(-1);
	} 
	else {
	    server_sock.sin_port = sp->s_port;
	}
    } 
    else {
	server_sock.sin_port = htons(atoi(service)); /* convert ASCII to int. */
    }

    /* Create 'sockaddr_in' internet address structure for connect.	*/
    server_sock.sin_family = hp->h_addrtype;
    memcpy((char*)&server_sock.sin_addr, hp->h_addr, hp->h_length);

    /* Print IP address in xx.xx.xx.xx format */
    printf(" %% IP address: %s\n", inet_ntoa(server_sock.sin_addr));
    printf(" %% Service port # is %d. \n", ntohs(server_sock.sin_port));

    /* Connect to the socket address.	*/
    if (connect( s, (struct sockaddr *) &server_sock, sizeof(server_sock)) < 0) {
	printf(" %% Attempt to connect to remote host failed.\n ");
	close (s);
	return(FAILURE);
    }

    return(s);
}


/************************************************************************
 *  get_mspacket:
 *	Read a miniSEED packet from the socket, and return a DATA_HDR
 *	structure for the packet.  Calling routine must free the DATA_HDR.
 *	Return NULL on failure.
 ************************************************************************/
DATA_HDR* get_mspacket(int sd, char* indata)
{
    int            nr;
    DATA_HDR      *hdr;
    BS            *bs = NULL;
    BLOCKETTE_HDR *bh = NULL;
    int goodpacket;

    goodpacket = 0;

    while(!goodpacket) {
	nr = xread(sd, (char *)indata, MIN_BLKSIZE);
	if (nr == 0) {
            fprintf (stderr, "Found end of file on socket read: %d\n",nr);
            return(NULL);
	}
	if (nr < MIN_BLKSIZE) {
            fprintf (stderr, "Error reading from socket: expected %d got %d\n",
		     MIN_BLKSIZE, nr);
            continue;
	}
	if ((hdr = decode_hdr_sdr ((SDR_HDR *)indata, MIN_BLKSIZE)) == NULL) {
	    fprintf (stderr, "Error decoding SEED data hdr\n");
	    continue;
	}
	if (hdr->blksize > MIN_BLKSIZE) {
	    nr = xread (sd, (char *)indata + MIN_BLKSIZE, hdr->blksize-MIN_BLKSIZE);
	    if (nr < hdr->blksize-MIN_BLKSIZE) {
                fprintf (stderr, "Error reading SEED data\n");
		free_data_hdr (hdr);
                continue;
	    }
	}
        
	goodpacket = 1;

	/* Fill in the number of data frames.   */
	if ((bs = find_blockette(hdr,1001)) &&
	    (bh = (BLOCKETTE_HDR *)(bs->pb))) {
	    hdr->num_data_frames = ((BLOCKETTE_1001 *)bh)->frame_count;
	    /* Explicitly set num_data_frames for SHEAR stations.   */
	    if (hdr->num_data_frames == 0 && hdr->sample_rate != 0)
		hdr->num_data_frames = (hdr->blksize - hdr->first_data) / sizeof(FRAME);
	}
	else {
	    hdr->num_data_frames =
		(hdr->sample_rate == 0) ? 0 :
		(hdr->blksize - hdr->first_data) / sizeof(FRAME);
	}
    } /* End while !goodpacket */
    return (hdr);
}

/************************************************************************
 *  fill_from_socket:
 *	Fill ring from data received from a TCP/IP socket.
 ************************************************************************/
int fill_from_socket (char *station, char *host, char *service, 
		      char *passwd, int flag)
{
    int res;
    DATA_HDR *data_hdr;
    char request_str[256];
    char seedrecord[512];
    int reading_packets = TRUE;
    int i;
    char *p;
    int status;
    int socket_channel;
    
    /* Fill in channelv info from the station list argument.		*/
    status = vsplit (station, ",", &nchannel, (char ***)&channelv);
    if (status != SUCCESS) return (status);

    /* Split each item into station, channel, etc.			*/
    for (i=0; i<nchannel; i++) {
	int nsplit;
	char **splitv;
	p = (char *)channelv[i];
	channelv[i] = (CHANNEL_INFO *)malloc(sizeof(CHANNEL_INFO));
	if (channelv[i] == NULL) {
	    fprintf (stderr, "Unable to malloc channelv info\n");
	    return (FAILURE);
	}
	memset ((void *)channelv[i], 0, sizeof(CHANNEL_INFO));
	status = vsplit (p, ".", &nsplit, &splitv);
	if (status != SUCCESS) return (FAILURE);
	switch (nsplit) {
	case 1:
	    strncpy(channelv[i]->station,splitv[0],7);
	    strncpy(channelv[i]->channel,"???",3);
	    strncpy(channelv[i]->network,"*",3);
	    strncpy(channelv[i]->location,"??",3);
	    break;
	case 2:
	    strncpy(channelv[i]->station,splitv[0],7);
	    strncpy(channelv[i]->channel,splitv[1],3);
	    strncpy(channelv[i]->network,"*",3);
	    strncpy(channelv[i]->location,"??",3);
	    break;
	case 3:
	    strncpy(channelv[i]->station,splitv[0],7);
	    strncpy(channelv[i]->network,splitv[1],3);
	    strncpy(channelv[i]->channel,splitv[2],3);
	    strncpy(channelv[i]->location,"??",3);
	    break;
	case 4:
	    strncpy(channelv[i]->station,splitv[0],7);
	    strncpy(channelv[i]->network,splitv[1],3);
	    strncpy(channelv[i]->channel,splitv[2],3);
	    strncpy(channelv[i]->location,splitv[3],3);
	    break;
	default:
	    fprintf (stderr, "Error in station string: %s\n", p);
	    return (FAILURE);
	}
	free_vsplit(nsplit,splitv);
	free (p);
    }

    /* Set up a condition handler for SIGPIPE, since a write to a	*/
    /* close pipe/socket raises a alarm, not an error return code.	*/

    /* Open the socket connection */

    while (! terminate_proc) {

	/* Allow signals to be handled with the default handler		*/
	/* while we setup the socket.  */

	signal (SIGINT,SIG_DFL);
	signal (SIGTERM,SIG_DFL);
	signal (SIGPIPE,SIG_DFL);

	socket_channel = open_socket(host, service);
	if (socket_channel < 0) {
	    printf("Error on open_socket call.\n");
	    sleep(5);
	    continue;
	}

	if (flag & SOCKET_REQUEST_PASSWD) {
	    sprintf (request_str, "%d %s %s\n", 
		     DATASOCK_PASSWD_CODE, PASSWD_KEYWD, passwd);
	    write (socket_channel, request_str, strlen(request_str));
	}
	if (flag & SOCKET_REQUEST_CHANNELS) {
	    for (i=0; i<nchannel; i++) {
		sprintf (request_str, "%d %s %s\n", DATASOCK_CHANNEL_CODE, 
			 channelv[i]->station, channelv[i]->channel);
		write (socket_channel, request_str, strlen(request_str));
	    }
	}
	if (flag) {
	    sprintf (request_str, "%d %s\n", DATASOCK_EOT_CODE, "EOT");
	    write (socket_channel, request_str, strlen(request_str)+1);
	}

/*  
 * 
 *   Reading socket to get miniSEED records.
 *   status == 0 indicates end-of-file (socket closed by remote), else its 
 *   the number of bytes received
 *
 */

	terminate_proc = 0;
	signal (SIGINT,finish_handler);
	signal (SIGTERM,finish_handler);
	signal (SIGPIPE,finish_handler);

	while(reading_packets && ! terminate_proc) {
	    data_hdr = get_mspacket(socket_channel,(char *)seedrecord);
	    if(data_hdr == NULL) {
		reading_packets = FALSE;
		continue;
	    }
	    res = write_to_ring((seed_record_header *)seedrecord);
	    if (res < 0) {
		printf("Error writing to ring\n");
	    }
	    free_data_hdr(data_hdr);
	}

	if (terminate_proc) break;
	close(socket_channel);
	sleep(5);
    }
    return (SUCCESS);
}
