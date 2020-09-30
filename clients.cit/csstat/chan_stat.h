
#include <iostream>

using std::ostream;


// this class holds channel statistics on a given channel/location code
//  all updates should be done in here.

// This function gets passed a mseed packet at initialization and via the updateStats() call.

// the updateStats() outputs to the stdoutput specified (default is stdout) a message of the form
// DATETIME PARAMNAME CHAN LOC-CODE: message

class ChanLocStat {

private:
	char channel_name[4];   // NULL terminated channel
	char location_code[3];	// NULL terminated 2 char location code
	double cumulative_gaps;	// the cumulative seconds of gaps seen since start
	double data_last_epoch;		// the last epoch of end time for the last packet
	double data_first_epoch;	// the 	first start time of data
	unsigned long num_packets;	// counter of number of packets seen
	unsigned long num_gaps;	// counter of number of packet gaps seen
	
public:
	ChanLocStat(char *pkt);
	~ChanLocStat();
	void updateStats(char *pkt);		// update the statistics and output to stdout
	char * getStats();		// return stats strings on demand
	friend ostream& operator<<(ostream &os, const ChanLocStat &c);
};
