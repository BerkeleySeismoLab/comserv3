/* Structure for comlink config directives for all comserv3 servers */

#ifndef CSCONFIG_H
#define CSCONFIG_H

/*
 * 29 Sep 2020 DSN Updated for comserv3.
 */

#include <stdio.h>
#include "cslimits.h"
// #include "cfgutil.h"

typedef struct {
    char client_name[CFGWIDTH];
    int32_t timeout;
} cs_clients;    

typedef struct {
    int32_t user_id;
    int32_t user_mask;
} cs_user_privilege;

typedef struct  {
    /* Info from STATIONS_INI file. */
    char server_desc[CFGWIDTH];
    char server_dir[CFGWIDTH];
    char server_source[CFGWIDTH];
    char seed_station[CFGWIDTH];
    char seed_network[CFGWIDTH];
#ifdef COMSERV2
    /* Info from either NETWORK_INI[netmon] or STATIONS_INI file. */
    char logdir[CFGWIDTH];
    char logtype[CFGWIDTH];
    /* Info from original comlink section in STATIONS_INI file */
    char lockfile[CFGWIDTH];
    char ipport[CFGWIDTH];
    char udpaddr[CFGWIDTH];
    char mcastif[CFGWIDTH];
#endif
    /* This now contains ONLY info required for comserv client-server interface. */
    char station[CFGWIDTH];
    char log_channel_id[CFGWIDTH];
    char log_location_id[CFGWIDTH];
    char timing_channel_id[CFGWIDTH];
    char timing_location_id[CFGWIDTH];
    int32_t verbose;
    int32_t segid;
    int32_t pollusecs;
    int32_t databufs;
    int32_t detbufs;
    int32_t calbufs;    
    int32_t timbufs;
    int32_t msgbufs;
    int32_t blkbufs;
    int32_t override;
    int32_t n_clients;
    cs_clients clients[MAXCLIENTS];
    int32_t n_uids;
    cs_user_privilege user_privilege[MAXUSERIDS];
} csconfig;

#ifdef __cplusplus
extern "C" {
#endif
    void initialize_csconfig (csconfig *cs_cfg);
    int verify_server_name (char *server_name);
#ifdef COMSERV2
    int getLogParamsFromNetwork (csconfig *cs_cfg); 
#endif
    int getCSServerInfo (csconfig *cs_cfg, char *server_name);
    int getCSServerParams (csconfig *cs_cfg, char *server_dir, char *section);
    int getAllCSServerParams (csconfig *cs_cfg, char *server_name);
#ifdef __cplusplus
}
#endif

#endif
