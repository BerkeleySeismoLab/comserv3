/*  Libmsmcast MiniSEED Data Processing

    This file is part of Libmsmcast, derived from Lib660 provided by
    Quanterra Inc and Certified Software Corporation.

    Copyright 2020 Berkeley Seismological Laboratory, University of California

    Libmsmcast is free software; you can redistribute it and/or modify
    it under the terms of the GNU General Public License as published by
    the Free Software Foundation; either version 2 of the License, or
    (at your option) any later version.

    Libmsmcast is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU General Public License for more details.

    You should have received a copy of the GNU General Public License
    along with Lib660; if not, write to the Free Software
    Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA  02110-1301  USA

Edit History:
   Ed Date       By  Changes
   -- ---------- --- ---------------------------------------------------
    0 2020-08-31 DSN Created from Lib660.
    1 2020-09-29 DSN Updated for comserv3.
*/

#include "libtypes.h"
#include "libseed.h"
#include "libdata.h"
#include "libclient.h"
#include "libstrucs.h"
#include "libsupport.h"
typedef char string5[6];

#ifdef DODBPRINT
#define DBPRINT(A) A
#else
#define DBPRINT(A) 
#endif

int process_mseed(pmsmcast msmcast, pbyte pbuf, int recsize)
{
    seed_header hdr;
    int status;
    string5 seed_station;
    string2 seed_network;
    string3 seed_channel;
    string2 seed_location;
    enum tpacket_class packet_class;
    double drate;
    int irate;
    pbyte copy_pbuf = pbuf;

    /* Currently accept only 512 byte MiniSEED records. */
    if (recsize != 512) return FALSE;

    memclr(&hdr, sizeof(hdr));
    DBPRINT(printf ("DEBUG:: call loadseedhdr_classify\n");)
    status = loadseedhdr_classify(&copy_pbuf, &hdr, recsize);
    DBPRINT(printf ("DEBUG:: loadseedhdr_classify rc=%d\n",status);)
    switch (status) {
    case PKC_DATA:
    case PKC_EVENT:
    case PKC_CALIBRATE:
    case PKC_TIMING:
    case PKC_MESSAGE:
    case PKC_OPAQUE:
	packet_class = status;
	break;
    default:  /* error */
	return FALSE;
    }

    charncpy(seed_station,(char *)hdr.station_id_call_letters,sizeof(seed_station)-1);
    charncpy(seed_network,(char *)hdr.seednet,sizeof(seed_network)-1);
    charncpy(seed_channel,(char *)hdr.channel_id,sizeof(seed_channel)-1);
    charncpy(seed_location,(char *)hdr.location_id,sizeof(seed_location)-1);;

    msmcast->miniseed_call.context = msmcast ;
    sprintf(msmcast->miniseed_call.station_name, "%s-%s", seed_network, seed_station);
    strcpy(msmcast->miniseed_call.seed_station, seed_station);
    strcpy(msmcast->miniseed_call.seed_network, seed_network);
    strcpy(msmcast->miniseed_call.seed_channel, seed_channel);
    strcpy(msmcast->miniseed_call.seed_location, seed_location);
    drate = sps_rate(hdr.sample_rate_factor, hdr.sample_rate_multiplier);
    irate = (drate >= 1) ? lib_round(drate) : (-1 * lib_round (1./drate));
    msmcast->miniseed_call.rate = irate;
    msmcast->miniseed_call.timestamp = 0. ;  //:: FIX THIS
    msmcast->miniseed_call.packet_class = packet_class ;
    msmcast->miniseed_call.data_size = 512 ;
    msmcast->miniseed_call.data_address = pbuf ;
    msmcast->par_create.call_minidata (addr(msmcast->miniseed_call)) ;

    return TRUE;

}
