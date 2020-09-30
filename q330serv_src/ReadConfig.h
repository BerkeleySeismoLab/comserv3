/*
 * File     :
 *  ReadConfig.h
 *
 * Purpose  :
 *  These routine is called by the main q330serv routine to
 *  read in the Q330 configuration values and to populate a ConfigVO
 *  with all the values.
 *
 * Author   :
 *  Phil Maechling
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
 * Modification History:
 *  4 April 2002
 *  2020-09-29 DSN Updated for comserv3.
 */

#ifndef READ_CONFIG_H
#define READ_CONFIG_H

bool readConfigFile(char* argv);
bool validateQ330servConfig(const struct q330serv_cfg& aCfg);

#endif
