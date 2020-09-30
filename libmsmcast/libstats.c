/*  Libmsmcast Statistics

    This file is part of Libmsmcast, derived from Lib660 provided by
    Quanterra Inc and Certified Software Corporation.

    Copyright 2017 Certified Software Corporation
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
#include "libstrucs.h"
#include "libclient.h"
#include "libsampglob.h"
#include "libmsgs.h"
#include "libsupport.h"

void add_status (pmsmcast msmcast, enum tlogfld acctype, longword count)
{

    incn(msmcast->share.accmstats[acctype].accum, count) ;
    incn(msmcast->share.accmstats[acctype].accum_ds, count) ;
}

void lib_stats_timer (pmsmcast msmcast)
{
    enum tlogfld acctype ;
    integer minute, last_minute, comeff_valids ;
    longint total, val ;
    taccmstat *paccm ;

    lock (msmcast) ;
    if (msmcast->libstate == LIBSTATE_RUN)
    {
        add_status (msmcast, LOGF_COMMDUTY, 10) ;
        msmcast->share.accmstats[LOGF_COMMEFF].accum_ds = 1000 ;
    }
    else
	msmcast->share.accmstats[LOGF_COMMEFF].accum_ds = 0 ;
    for (acctype =AC_FIRST ; acctype <= AC_LAST ; acctype++)
    {
	paccm = addr(msmcast->share.accmstats[acctype]) ;
	if (paccm->ds_lcq)
	{
            switch (acctype) {
	    case LOGF_RECVBPS :
	    case LOGF_SENTBPS :
                val = paccm->accum_ds / 10 ;
                break ;
	    case LOGF_COMMDUTY :
                val = (paccm->accum_ds * 1000) / 10 ;
                break ;
	    case LOGF_THRPUT :
                val = (paccm->accum_ds * 100) / 10 ;
                break ;
	    default :
                val = paccm->accum_ds ;
                break ;
            }
	}
	paccm->accum_ds = 0 ;
    }
    unlock (msmcast) ;
    inc(msmcast->minute_counter) ;
    if (msmcast->minute_counter >= 6)
	msmcast->minute_counter = 0 ;
    else
	return ; /* not a new minute yet */
    lock (msmcast) ;
    if (msmcast->share.accmstats[LOGF_COMMDUTY].accum >= 5)
	msmcast->share.accmstats[LOGF_COMMEFF].accum = 1000 ; /* was connected */
    else
	msmcast->share.accmstats[LOGF_COMMEFF].accum = INVALID_ENTRY ;
    last_minute = msmcast->share.stat_minutes ;
    msmcast->share.stat_minutes = (msmcast->share.stat_minutes + 1) % 60 ;
    for (acctype = AC_FIRST ; acctype <= AC_LAST ; acctype++)
    {
	paccm = addr(msmcast->share.accmstats[acctype]) ;
	paccm->minutes[last_minute] = paccm->accum ;
	if (msmcast->share.stat_minutes == 0)
	{
            if (acctype != LOGF_COMMEFF)
	    { /* new hour, update current hour */
		total = 0 ;
		for (minute = 0 ; minute <= 59 ; minute++)
                    total = total + paccm->minutes[minute] ;
		paccm->hours[msmcast->share.stat_hours] = total ;
	    }
	    else
	    { /* same, but take into account invalid entries */
		comeff_valids = 0 ;
		total = 0 ;
		for (minute = 0 ; minute <= 59 ; minute++)
                    if (paccm->minutes[minute] != INVALID_ENTRY)
		    {
			inc(comeff_valids) ;
			total = total + paccm->minutes[minute] ;
		    }
		if (comeff_valids > 0)
		    total = lib_round((double)total / (comeff_valids / 60.0)) ;
		else
		    total = INVALID_ENTRY ;
		paccm->hours[msmcast->share.stat_hours] = total ;
	    }
	}
	paccm->accum = 0 ; /* start counting the new minute as zero */
    }
    if (msmcast->share.stat_minutes == 0)
    { /* move to next hour */
        msmcast->share.stat_hours = (msmcast->share.stat_hours + 1) % 24 ;
        for (acctype = AC_FIRST ; acctype <= AC_LAST ; acctype++)
	{
            paccm = addr(msmcast->share.accmstats[acctype]) ;
            paccm->hours[msmcast->share.stat_hours] = 0 ; /* start counting the new hour as zero */
	}
    }
    inc(msmcast->share.total_minutes) ;
    unlock (msmcast) ;
    if (msmcast->par_create.call_state)
    {
        msmcast->state_call.context = msmcast ;
        msmcast->state_call.state_type = ST_OPSTAT ;
        msmcast->state_call.subtype = 0 ;
        memcpy(addr(msmcast->state_call.station_name), addr(msmcast->station_ident), sizeof(msmcast->station_ident)) ;
        msmcast->state_call.info = 0 ;
        msmcast->state_call.subtype = 0 ;
        msmcast->par_create.call_state (addr(msmcast->state_call)) ;
    }
}

void update_op_stats (pmsmcast msmcast)
{
    enum tlogfld acctype ;
    integer lastminute, minute, hour, valids, comeff_valids ;
    integer i, good, value ;
    longint total ;
    taccmstat *paccm ;
    topstat *pops ;

    /* Initialization. */
    pops = addr(msmcast->share.opstat) ; 
    pops->data_latency = INVALID_LATENCY ;
    pops->status_latency = INVALID_LATENCY ;

    memcpy(addr(pops->station_name), addr(msmcast->station_ident), sizeof(msmcast->station_ident)) ;
    if (msmcast->saved_data_timetag > 1)
	pops->data_latency = now () - msmcast->saved_data_timetag + 0.5 ;
    lastminute = msmcast->share.stat_minutes - 1 ;
    if (lastminute < 0)
	lastminute = 59 ;
    for (acctype = AC_FIRST ; acctype <= AC_LAST ; acctype++)
    {
	paccm = addr(msmcast->share.accmstats[acctype]) ;
	if (msmcast->share.total_minutes >= 1)
	    switch (acctype) {
            case LOGF_RECVBPS :
            case LOGF_SENTBPS :
		pops->accstats[acctype][AD_MINUTE] = paccm->minutes[lastminute] / 60 ;
		break ;
            case LOGF_COMMDUTY :
		pops->accstats[acctype][AD_MINUTE] = (paccm->minutes[lastminute] * 1000) / 60 ;
		break ;
            case LOGF_THRPUT :
		pops->accstats[acctype][AD_MINUTE] = (paccm->minutes[lastminute] * 100) / 60 ;
		break ;
            default :
		pops->accstats[acctype][AD_MINUTE] = paccm->minutes[lastminute] ;
		break ;
	    }
        else
	    pops->accstats[acctype][AD_MINUTE] = INVALID_ENTRY ;
	valids = msmcast->share.total_minutes ;
	if (valids > 60)
	    valids = 60 ;
	total = 0 ;
	comeff_valids = 0 ;
	for (minute = 0 ; minute <= valids - 1 ; minute++)
        {
	    if (acctype == LOGF_COMMEFF)
	    {
                if (paccm->minutes[minute] != INVALID_ENTRY)
		{
		    inc(comeff_valids) ;
		    total = total + paccm->minutes[minute] ;
		}
	    }
            else
		total = total + paccm->minutes[minute] ;
        }
	if (valids == 0)
	    total = INVALID_ENTRY ;
	pops->minutes_of_stats = valids ;
	if (total == INVALID_ENTRY)
	    pops->accstats[acctype][AD_HOUR] = total ;
        else
	    switch (acctype) {
            case LOGF_RECVBPS :
            case LOGF_SENTBPS :
		pops->accstats[acctype][AD_HOUR] = total / (60 * valids) ;
		break ;
            case LOGF_COMMDUTY :
		pops->accstats[acctype][AD_HOUR] = (total * 1000) / (60 * valids) ;
		break ;
            case LOGF_THRPUT :
		pops->accstats[acctype][AD_HOUR] = (total * 100) / (60 * valids) ;
		break ;
            case LOGF_COMMEFF :
		if (comeff_valids == 0)
		{
                    pops->accstats[acctype][AD_HOUR] = INVALID_ENTRY ;
                    total = INVALID_ENTRY ;
		}
                else
		{
                    pops->accstats[acctype][AD_HOUR] = total / comeff_valids ;
                    total = lib_round((double)total / (comeff_valids / 60.0)) ;
		}
		break ;
            default :
		pops->accstats[acctype][AD_HOUR] = total ;
		break ;
	    }
	valids = msmcast->share.total_minutes / 60 ; /* hours */
	if (valids >= 24)
	    valids = 24 ;
        else
	    total = 0 ;
	comeff_valids = 0 ;
	for (hour = 0 ; hour <= valids - 1 ; hour++)
        {
	    if (acctype == LOGF_COMMEFF)
	    {
                if ((hour == msmcast->share.stat_hours) && (total != INVALID_ENTRY))
                    inc(comeff_valids) ;
                else if ((hour != msmcast->share.stat_hours) && (paccm->hours[hour] != INVALID_ENTRY))
		{
		    inc(comeff_valids) ;
		    total = total + paccm->hours[hour] ;
		}
	    }
            else
		total = total + paccm->hours[hour] ;
        }
	if (valids == 0)
	    total = INVALID_ENTRY ; /* need at least one hour to extrapolate */
	pops->hours_of_stats = valids ;
	if (total == INVALID_ENTRY)
	    pops->accstats[acctype][AD_DAY] = total ;
        else
	{
	    switch (acctype) {
            case LOGF_RECVBPS :
            case LOGF_SENTBPS :
		pops->accstats[acctype][AD_DAY] = total / (3600 * valids) ;
		break ;
            case LOGF_COMMDUTY :
		pops->accstats[acctype][AD_DAY] = (total * 1000) / (3600 * valids) ;
		break ;
            case LOGF_THRPUT :
		pops->accstats[acctype][AD_DAY] = (total * 100) / (3600 * valids) ;
		break ;
            case LOGF_COMMEFF :
		if (comeff_valids == 0)
		    pops->accstats[acctype][AD_DAY] = INVALID_ENTRY ;
                else
		    pops->accstats[acctype][AD_DAY] = total / (60 * comeff_valids) ;
		break ;
            default :
		pops->accstats[acctype][AD_DAY] = total ;
		break ;
	    }
	}
    }
}
