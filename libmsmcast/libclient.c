/*  Libmsmcast client interface

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

#include "libclient.h"
#include "libstrucs.h"
#include "libmsgs.h"

void lib_create_context (tcontext *ct, tpar_create *cfg) /* If ct = NIL return, check resp_err */
{
    lib_create_msmcast (ct, cfg) ;
}

enum tliberr lib_destroy_context (tcontext *ct) /* Return error if any */
{
    return lib_destroy_msmcast (ct) ;
}

enum tliberr lib_register (tcontext ct, tpar_register *rpar)
{
    enum tliberr result = 0 ;
    pmsmcast msmcast ;

    msmcast = ct ;
    if (msmcast == NIL)
	return LIBERR_INVCTX ;
    result = lib_register_msmcast (msmcast, rpar) ;
    if (msmcast->share.target_state == LIBSTATE_WAIT)
	libmsgadd (msmcast, LIBMSG_NOIP, "") ;
    return result ;
}

enum tlibstate lib_get_state (tcontext ct, enum tliberr *err, topstat *retopstat)
{
    pmsmcast msmcast ;

    msmcast = ct ;
    if (msmcast == NIL)
    {
        *err = LIBERR_INVCTX ;
        return LIBSTATE_IDLE ; /* not really */
    }
    *err = msmcast->share.liberr ;
    lock (msmcast) ;
    update_op_stats (msmcast) ;
    memcpy (retopstat, addr(msmcast->share.opstat), sizeof(topstat)) ;
    unlock (msmcast) ;
    return msmcast->libstate ;
}

void lib_change_state (tcontext ct, enum tlibstate newstate, enum tliberr reason)
{
    pmsmcast msmcast ;

    msmcast = ct ;
    if (msmcast == NIL)
	return ;
    lock (msmcast) ;
    msmcast->share.target_state = newstate ;
    msmcast->share.liberr = reason ;
    unlock (msmcast) ;
}

const tmodules modules = 
{
    {/*name*/"Libclient", /*ver*/VER_LIBCLIENT},     {/*name*/"Libstrucs", /*ver*/VER_LIBSTRUCS},
    {/*name*/"libmsgs", /*ver*/VER_LIBMSGS},         {/*name*/"", /*ver*/0},
    {/*name*/"", /*ver*/0}, {/*name*/"", /*ver*/0},
    {/*name*/"", /*ver*/0}, {/*name*/"", /*ver*/0},
    {/*name*/"", /*ver*/0}, {/*name*/"", /*ver*/0},
    {/*name*/"", /*ver*/0}, {/*name*/"", /*ver*/0}
};

pmodules lib_get_modules (void)
{
    return (pmodules)addr(modules) ;
}

enum tliberr lib_flush_data (tcontext ct)
{
    return LIBERR_NOERR ;
}
