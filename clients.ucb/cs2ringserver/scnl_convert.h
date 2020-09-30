/* $Id: scnl_convert.h,v 1.2 2005/01/10 23:03:56 redi Exp $	*/

/* How many S2S structures to allocate at once: */
#define INCR_SCNL 10

typedef struct 
{
    char *s;
    char *c;
    char *n;
    char *l;
} SCNL;

typedef struct
{
    SCNL scnl_in;
    SCNL scnl_out;
    int ns, nc, nn, nl;
} S2S;

int s2s_set( S2S S );
void sort_scnl(void);
int scnl2scnl( S2S* );

