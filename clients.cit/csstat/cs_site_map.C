/*
 * 29 Sep 2020 DSN Updated for comserv3.
 */

#include "cs_site_map.h"
#include <string.h>
#include <unistd.h>

#include <iostream>

extern "C" {
#include "stuff.h"
#include "cfgutil.h"
};

cs_site_map::cs_site_map(char *config) {
}
cs_site_map::cs_site_map() {

	char *cptr;
	cptr = new char[strlen(STATIONS_LIST)+1];
	_init(cptr);
	delete cptr;
}
void cs_site_map::_init(char *config) {

    char * server_id;
    char * site_name;
    char tmp[80];
    char str1[CFGWIDTH];
    char str2[CFGWIDTH];
    NameMap::iterator iter; 

	initialized=false;
	config_struc cfg;
	if (open_cfg (&cfg, (char *)STATIONS_LIST, (char *)"*") != 0) {
        	close_cfg (&cfg);
        	return;    /* error */
    	}
	do {
	   site_name=NULL;
	   server_id=NULL;
           /* Get the next site in the network config file.             */
           strcpy (tmp, &cfg.lastread[1]);
           tmp[strlen(tmp)-1] = '\0';    /* remove [ ]   */
           upshift (tmp);
  	   server_id = new char[strlen(tmp)+1];
           strcpy(server_id, tmp);

	   /* see if there is a site name mapping */
	   while(1) {
             read_cfg(&cfg, str1, str2); /* Skip past section header.    */
             if (str1[0] == '\0') break;
             if (strcmp(str1, "STATION") == 0 || strcmp(str1, "SITE") == 0) {
		site_name=new char[strlen(str2)+1];
		strcpy(site_name, str2);
                break;
             }
           }
	   // load them into the hash maps only if they both exist.
	   if (server_id != NULL && site_name != NULL) {
#ifdef DEBUG
		std::cout<< "Found a server_id to site mapping: "<< server_id << "->"
			<< site_name << std::endl;
#endif // DEBUG
		ServerID2SiteName[server_id] = site_name;
		SiteName2ServerID[site_name] = server_id;
	   } 
        }      while (skipto (&cfg, (char *)"*") == 0);
	close_cfg(&cfg);

	initialized=true;


// output for debuggings sake
#ifdef DEBUG
	for (iter=ServerID2SiteName.begin();  iter != ServerID2SiteName.end(); iter++)
		std::cout<<"Found Server id to Site Mapping: " << (*iter).first<< "->"
				<< (*iter).second << std::endl;
#endif //DEBUG
}

cs_site_map::~cs_site_map() {
}
char * cs_site_map::getServerID(const char *SiteName) {
  NameMap::iterator iter; 

	iter = SiteName2ServerID.find(SiteName);
	if (iter != SiteName2ServerID.end() ) {
		return (*iter).second;
	}
	return NULL;
}
char * cs_site_map::getSiteName(const char *ServerID) {
  NameMap::iterator iter; 
	iter = ServerID2SiteName.find(ServerID);
	if (iter != ServerID2SiteName.end() ) {
		return (*iter).second;
	}
	return NULL;
}
