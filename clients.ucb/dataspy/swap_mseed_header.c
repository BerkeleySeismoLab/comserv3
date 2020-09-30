#include <qlib2.h>

#include "stuff.h"

/********************************************************************************************
 * WARNING: this is a recursive procedure.
 * do blockettes() recursively reads blockettes in the block until the last blockette
 * (blockette->next == 0 ) or until we encounter an unknown blockette.
 * Use swab_blockettes() from qlib2 which does the actual byte swapping.
 * Return 0 on success, -1 on error (eg unknown blockette).
 * Ilya Dricker ISTI 03/31/00
 ********************************************************************************************/
int do_blockettes(char *block, short next_blockette)
{
    short next_blockette_type;
    short next_blockette_next;
    short blockette_length;
    short nb;
    BLOCKETTE_HDR* bl_hdr;
    int status = 0;

    if (next_blockette == 0) return(0);

    bl_hdr = (BLOCKETTE_HDR*)(block+next_blockette); /*the header of our current blockette */
    next_blockette_type = flip2 (bl_hdr->type);
    next_blockette_next =  flip2 (bl_hdr->next);

    nb = next_blockette_type;  /* If it is not a blockette in the list below , get out ASAP ! */
    if  (!(nb == 1000 || nb == 1001  || nb == 100 || nb == 200 ||  nb == 201 ||  nb == 300 ||  nb == 310  ||  nb == 320 \
	   ||  nb == 390 ||  nb == 395 ||  nb == 405 ||  nb == 500 || nb == 2000) ){
	fprintf(stderr, "WARNING: unsupported blockette %d is not byte-swapped\n", nb);
	return(-1);
    }

    /* If no next blockette and there is a good chance the blockette is not */
    /* blockette 300,310,320  which require exact length*/
    if (next_blockette_next == 0)
	blockette_length = 8;
    else
	blockette_length = next_blockette_next - next_blockette;   

#ifdef	CHANGE_B1000
    if(next_blockette_type == 1000)	{
	BLOCKETTE_1000* b1000;
	b1000 = (BLOCKETTE_1000*)(block+next_blockette );
	if (hdr_order == 0) /*Linux order */
	    b1000->word_order = '\001';
	else 
	    b1000->word_order = '\000';
    }
#endif

    if (swab_blockette (next_blockette_type, (char *) (block + next_blockette), blockette_length) < 0)	{
	fprintf(stderr, "WARNING: blockette %d was not byte-swapped properly\n", next_blockette_type);
	return(-1);
    }

    status = do_blockettes(block, next_blockette_next);

    return(status);
}

/************************************************************************
 * swap_mseed_header:
 *	Swap the endian of a MiniSEED Fixed Data Header.
 * Return: 0 on success, non-zero on error.
 ************************************************************************/
int swap_mseed_header (char *block)
{
    SDR_HDR* sdr_hdr;
    short next_blockette; 
    int status;

    sdr_hdr = (SDR_HDR *) (block);
    next_blockette = flip2(sdr_hdr->first_blockette);

    /* Swap bytes in a fixed header */
    sdr_hdr->time.year=flip2(sdr_hdr->time.year);
    sdr_hdr->time.day=flip2(sdr_hdr->time.day);
    sdr_hdr->time.ticks=flip2(sdr_hdr->time.ticks);
    sdr_hdr->num_samples  =  flip2(sdr_hdr->num_samples);
    sdr_hdr->sample_rate_factor = flip2(sdr_hdr->sample_rate_factor);
    sdr_hdr->sample_rate_mult = flip2(sdr_hdr->sample_rate_mult);
    sdr_hdr->num_ticks_correction = flip4(sdr_hdr->num_ticks_correction);
    sdr_hdr->first_data   = flip2(sdr_hdr->first_data);
    sdr_hdr->first_blockette  = flip2(sdr_hdr->first_blockette);

    /* Swap bytes in all blockettes */
    status = do_blockettes(block, next_blockette);

    return status;
}

