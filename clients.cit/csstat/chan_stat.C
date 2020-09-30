/*
 * 29 Sep 2020 DSN Updated for comserv3.
 */

#include "chan_stat.h"
#include <stdlib.h>
#include <stdio.h>
#include <errno.h>
#include <string.h>
#include <sys/types.h>
#include <time.h>

#include "qlib2.h"

// this class holds channel statistics on a given channel/location code
//  all updates should be done in here.

// This function gets passed a mseed packet at initialization and via the updateStats() call.

extern "C" {
#include "dpstruc.h"
#include "service.h"
#include "seedstrc.h"
#include "cfgutil.h"
#include "timeutil.h"
#include "stuff.h"
}

double seed_jul (seed_time_struc *st)
{
    double t ;
      
    t = jconv (flip2(st->yr) - 1900, flip2(st->jday)) ;
    t = t + (double) st->hr * 3600.0 + (double) st->minute * 60.0 + 
	(double) st->seconds + (double) flip2(st->tenth_millisec) * 0.0001 ;
    return t ;
}



//private:
	//char channel_name[4];   // NULL terminated channel
	//char location_code[3];	// NULL terminated 2 char location code
	//long cumulative_gaps;	// the cumulative seconds of gaps seen since start
	//double data_last_epoch;		// the last epoch of end time for the last packet
	//double data_first_epoch;	// the 	first start time of data
	
ChanLocStat::ChanLocStat(char *pkt) 
{
    double a;
    seed_record_header *pseed ;


    pseed = (seed_record_header *) pkt;
    // set the channel_name
    strncpy(channel_name, pseed->channel_id, 3);
    channel_name[3]='\0';
    // set the location code
    strncpy(location_code, pseed->location_id, 2);
    location_code[2]='\0';
    // get the starting time in UNIX epoch
    data_first_epoch = seed_jul(&(pseed->starting_time));
    // get the ending time in UNIX epoch
    a = sps_rate(flip2(pseed->sample_rate_factor),flip2(pseed->sample_rate_multiplier));
    /* latency calculation */
    data_last_epoch = (data_first_epoch + flip2(pseed->samples_in_record)/a);

    cumulative_gaps=0.0;
    num_packets=1;
    num_gaps=0;
    // std::cout << "Got a new packet '"<<channel_name<<"-"<<location_code<<"'"<<std::endl;
}

ChanLocStat::~ChanLocStat() 
{
}


// the updateStats() outputs to the stdoutput specified (default is stdout) a message of the form
// DATETIME PARAMNAME CHAN LOC-CODE: message
void ChanLocStat::updateStats(char *pkt) 
{
    double start, diff;
    long diff_mills;
    double a;
    seed_record_header *pseed;
    time_t now;
    struct tm *tmstruct;
    char time_string[256];
    char start_time_string[256];
    char myseqno[7];

    num_packets++;
    now = time(0);
    tmstruct = gmtime(&now);
    strftime(time_string, 256, "%Y%m%d-%H:%M:%S", tmstruct);
    // std::cout << "Got an old packet '"<<channel_name<<"-"<<location_code<<"'"<<std::endl;
    pseed = (seed_record_header *) pkt;

    // update the statistics and output to stdout if necessary
    start = seed_jul(&(pseed->starting_time));
    a = sps_rate(flip2(pseed->sample_rate_factor),flip2(pseed->sample_rate_multiplier));
    diff = start - data_last_epoch;
    diff_mills = diff * 1000;
    if (diff != 0.0 && diff_mills != 0) {
	// convert start time to start_time_string
	now = start;
	tmstruct = gmtime(&now);
	strftime(start_time_string, 256, "%Y%m%d-%H:%M:%S", tmstruct);
	cumulative_gaps += diff;
	num_gaps++;
	strncpy(myseqno,pkt,6);
	memset(&myseqno[6],0,1);
	if (diff >0.0) {
	    fprintf(stdout, "%s DATA_GAP '%s' '%s': Gap of %f seconds starting at %f and ending at %f, seq=%s\n",
		    time_string, channel_name, location_code, diff, data_last_epoch, start, myseqno);
	} else {
	    fprintf(stdout, "%s DATA_OVERLAP '%s' '%s': Gap of %f seconds starting at %f and ending at %f seq=%s\n",
		    time_string, channel_name, location_code, diff, data_last_epoch, start, myseqno);
	}
    }
    data_last_epoch = (start + flip2(pseed->samples_in_record)/a);
}

char * ChanLocStat::getStats() 
{
    // return stats strings on demand
    // DATETIME PARAMNAME CHAN LOC-CODE: message
    return NULL;
}

ostream& operator<<(ostream &os, const ChanLocStat &c)
{
    time_t now;
    struct tm *tmstruct;
    char time_string[256];
    char msg_string[1024];
    now = time(0);
    tmstruct = gmtime(&now);
    strftime(time_string, 256, "%Y%m%d-%H:%M:%S", tmstruct);
    os << "ChanLocStat: Summary channel='"<<c.channel_name<<"' Loc Code='"<<c.location_code<<"' packets_processed="<<c.num_packets<< std::endl;

    if (c.cumulative_gaps != 0.0) {
	sprintf(msg_string, "%s CUMULATIVE_GAPS '%s' '%s': Total Gaps=%lu lasting a total of %f seconds starting at %f and ending now\n",
		time_string, c.channel_name, c.location_code, c.num_gaps, c.cumulative_gaps, c.data_first_epoch);
	os << msg_string << std::endl;
    }
    return(os);
}
