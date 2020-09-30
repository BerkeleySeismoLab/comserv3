/*
 * File     :
 *   cserv.h
 *
 * Purpose  :
 *  This defines the public interface that the comserv main presents as an
 *  subroutine function.
 *
 * Author   :
 *  Phil Maechling
 *
 * Mod Date :
 *  27 March 2002
 *
 * This program is free software; you can redistribute it and/or modify
 * it with the sole restriction that:
 * You must cause any work that you distribute or publish, that in
 * whole or in part contains or is derived from the Program or any
 * part thereof, to be licensed as a whole at no charge to all third parties.
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.
 *
 * 29 Sep 2020 DSN Updated for comserv3.
 */

#ifndef CSERV_H
#define CSERV_H

#ifdef __cplusplus
extern "C" {
#endif
    int comserv_init (csconfig *cs_cfg, char* station_code);
    int comserv_scan();
#ifdef __cplusplus
}
#endif
#endif
