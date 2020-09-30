/*
 * File     :
 *  comserv_queue.h
 *
 * Purpose  :
 *  Comserv packet queueing routines.
 *
 *
 * Author   :
 *  Phil Maechling
 *
 *
 * Mod Date :
 *  1 April 2002
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
#ifndef COMSERV_QUEUE_H
#define COMSERV_QUEUE_H

#ifdef __cplusplus
extern "C" {
#endif

  int comserv_queue(char* buf,int len, int type);
  int comserv_anyQueueBlocking();
#ifdef __cplusplus
}
#endif
#endif
