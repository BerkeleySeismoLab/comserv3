/************************************************************************
 *  scnl_convert.c
 *
 * Modification History:
 *  2020-09-29 DSN Updated for comserv3.
 ************************************************************************/

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <qlib2.h>
#include "scnl_convert.h"

static S2S *exp_scnl = NULL;
static int max_exp = 0;
static int n_exp = 0;
static int incr_exp = INCR_SCNL;
static S2S *wild_scnl = NULL;
static int max_wild = 0;
static int n_wild = 0;
static int incr_wild = INCR_SCNL;


static int compare_scnl( const void *a, const void *b) 
{
    S2S *as = (S2S*)a;
    S2S *bs = (S2S*)b;
    int ns, nc, nn;
    
    if ((ns = strcmp(as->scnl_in.s, bs->scnl_in.s)) != 0) return( ns );
    if ((nc = strcmp(as->scnl_in.c, bs->scnl_in.c)) != 0) return( nc );
    if ((nn = strcmp(as->scnl_in.n, bs->scnl_in.n)) != 0) return( nn );
    return(   strcmp(as->scnl_in.l, bs->scnl_in.l));
}


int s2s_set(S2S S)
{
    char *s, *c, *n, *l;
    S2S *ss;
    
    s = S.scnl_in.s;
    c = S.scnl_in.c;
    n = S.scnl_in.n;
    l = S.scnl_in.l;
    if (s && c && n && l) {
	int ws, wc, wn, wl, wc3;
	ws = (strcmp(s, "*") == 0);
	wc = (strcmp(c, "*") == 0);
	wn = (strcmp(n, "*") == 0);
	wl = (strcmp(l, "*") == 0);
	wc3 = (c[2] == '?');
	if (ws || wc || wc3 || wn || wl) {
	    /* we have a wildcard */
	    if (n_wild == max_wild) {
		max_wild += incr_wild;
		wild_scnl = (S2S*) realloc(wild_scnl, max_wild * 
					   sizeof(S2S));
		if (wild_scnl == NULL) {
		    fprintf(stderr, "s2s_set: out of memory for wild_scnl\n");
		    exit(1);
		}
	    }
	    ss = &wild_scnl[n_wild];
	    memset(ss, 0, sizeof(S2S));
	    ss->scnl_in.s = strdup(s);
	    ss->scnl_in.c = strdup(c);
	    ss->scnl_in.n = strdup(n);
	    ss->scnl_in.l = strdup(l);
	    if ( ! (ss->scnl_in.s && ss->scnl_in.c && ss->scnl_in.n && ss->scnl_in.l)) {
		fprintf(stderr, "s2s_set: out of memory for wild_scnl\n");
		exit(1);
	    }
	    if (ws) 
		ss->ns = 0;
	    else    
		ss->ns = strlen(s)+1;
	    if (wc)
		ss->nc = 0;
	    else if (wc3)
		ss->nc = 2;
	    else
		ss->nc = 4;
	    if (wn)
		ss->nn = 0;
	    else
		ss->nn = strlen(n)+1;
	    if (wl)
		ss->nl = 0;
	    else
		ss->nl = strlen(l)+1;
	    n_wild++;
	}
	else {
	    /* no wildcard */
	    if (n_exp == max_exp) {
		max_exp += incr_exp;
		exp_scnl = (S2S*) realloc(exp_scnl, max_exp * 
					  sizeof(S2S));
		if (exp_scnl == NULL) {
		    fprintf(stderr, "s2s_set: out of memory for exp_scnl\n");
		    exit(1);
		}
	    }
	    ss = &exp_scnl[n_exp];
	    memset(ss, 0, sizeof(S2S));
	    ss->scnl_in.s = strdup(s);
	    ss->scnl_in.c = strdup(c);
	    ss->scnl_in.n = strdup(n);
	    ss->scnl_in.l = strdup(l);
	    if ( ! (ss->scnl_in.s && ss->scnl_in.c && ss->scnl_in.n && ss->scnl_in.l)) {
		fprintf(stderr, "s2s_set: out of memory for exp_scnl\n");
		exit(1);
	    }
	    n_exp++;
	}
	ss->scnl_out.s = strdup(S.scnl_out.s);
	ss->scnl_out.n = strdup(S.scnl_out.n);
	ss->scnl_out.c = strdup(S.scnl_out.c);
	ss->scnl_out.l = strdup(S.scnl_out.l);
	if (! (ss->scnl_out.s && ss->scnl_out.n && ss->scnl_out.c && ss->scnl_out.l)) {
	    fprintf(stderr, "s2s_set: out of memnory for SNCL\n");
	    exit(1);
	}
	return (1);
    }
    return(0);
}

void sort_scnl(void)
{
    if (n_exp > 0)
	qsort(exp_scnl, n_exp, sizeof(S2S), compare_scnl);
    
    return;
}

int scnl2scnl( S2S* s ) 
{
    S2S *found_scnl;
    int i, found = 0;
    static char chan[DH_CHANNEL_LEN+1];
    
    /* First look in the explicit list, if there are any scnls in it */
    if (n_exp > 0) {
        found_scnl = (S2S*) bsearch(s, exp_scnl, n_exp, sizeof(S2S), compare_scnl); /*LDD*/
	
	if (found_scnl != NULL) {
	    s->scnl_out = found_scnl->scnl_out;   /* structure copy */
	    return( 1 );
	}
    }
    
    /* not in explicit list; try wild-card list */
    for (i = 0; i < n_wild; i++) {
	if (wild_scnl[i].ns > 0) {
	    /* station is not a wildcard */
	    if (strncmp(s->scnl_in.s, wild_scnl[i].scnl_in.s, wild_scnl[i].ns) != 0)
		continue;
	}
	if (wild_scnl[i].nn > 0) {
	    /* network is not a wildcard */
	    if (strncmp(s->scnl_in.n, wild_scnl[i].scnl_in.n, wild_scnl[i].nn) != 0)
		continue;
	}
	if (wild_scnl[i].nc > 0) {
	    /* channel is not "*"; could be 'XX?' */
	    if (strncmp(s->scnl_in.c, wild_scnl[i].scnl_in.c, wild_scnl[i].nc) != 0)
		continue;
	}
	if (wild_scnl[i].nl > 0) {
	    /* network is not a wildcard */
	    if (strncmp(s->scnl_in.l, wild_scnl[i].scnl_in.l, wild_scnl[i].nl) != 0)
		continue;
	}
	found = 1;
	break;
    }
    
    if (! found) return( 0 );
    
    /* We found a match; fill in the to-be-returned scnl */
    if (wild_scnl[i].scnl_out.s[0] == '*') 
	s->scnl_out.s = s->scnl_in.s;
    else
	s->scnl_out.s = wild_scnl[i].scnl_out.s;

    if (wild_scnl[i].scnl_out.n[0] == '*')
	s->scnl_out.n = s->scnl_in.n;
    else
	s->scnl_out.n = wild_scnl[i].scnl_out.n;
    
    if (wild_scnl[i].scnl_out.c[0] == '*')
	s->scnl_out.c = s->scnl_in.c;
    else if (wild_scnl[i].scnl_out.c[2] == '?') {
	strncpy(chan, wild_scnl[i].scnl_out.c, DH_CHANNEL_LEN);
	chan[2] = s->scnl_in.c[2];
	chan[3] = '\0';
	s->scnl_out.c = chan;
    }
    else
	s->scnl_out.c = wild_scnl[i].scnl_out.c;
    
    if (wild_scnl[i].scnl_out.l[0] == '*') 
	s->scnl_out.l = s->scnl_in.l;
    else
	s->scnl_out.l = wild_scnl[i].scnl_out.l;
    
    return( 1 );
}
