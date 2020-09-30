#include <map>
#include "string.h"





#define STATIONS_LIST   "/etc/stations.ini"       
struct LessStr
{
        bool operator() (const char* s1, const char *s2) const
        {
                return strcmp(s1, s2) < 0;
        }
}; 
typedef std::map<const char*, char *, LessStr,
		std::allocator<std::pair<const char *, char *> > >  NameMap;

class cs_site_map {
	

private:
NameMap ServerID2SiteName;
NameMap SiteName2ServerID;

void _init(char *config);
bool initialized;

public:
cs_site_map(char *config);
cs_site_map();
~cs_site_map();

bool isInitialized();
char * getServerID(const char *SiteName);
char * getSiteName(const char *ServerID);
};
