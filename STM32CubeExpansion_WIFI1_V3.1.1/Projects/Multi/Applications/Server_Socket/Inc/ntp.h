#include <stdint.h>
 
 

#ifndef _NTP_H_
#define _NTP_H_



#define NTP_VERSION                     "2015-07-04"

#define NTP_PORT                        123

#define NTP_SERVER_NUM									4

#define NTP_SERVER_MAX_RETRIES					200
// Number of seconds to add to get unix local time
//  ( minus 4 hours and half for Venezuela )
#define NTP_LOCAL_DIFFERERENCE          - 60 * ( 4 * 60 + 30 )

// Delay between two synchronization
#define NTP_DELAY_SYNCHRO               83*60 // Time at witch I lost 1 minutes

// Delay in minutes between retries when failure
#define NTP_DELAY_FAILURE               3

// Number of seconds between January 1900 and January 1970 (Unix time)
#define NTP_SEVENTY_YEARS               2208988800UL

// Clock speed * number of seconds
#define NTP_ANSWER_WAIT_TIME            84000000 * NTP_DELAY_FAILURE

#define NTP_PACKET_SIZE                 48

#define NTP_SCHEDULER_THREAD_STACK_SIZE 768

#define NTP_SCHEDULER_THREAD_PRIORITY   (LOWPRIO + 2)


// define a structure of parameters for debugging and statistics
struct ntp_stru
{
  char* 		addr;
	uint8_t		server_id;
	uint32_t	lastNtpRequest;
	uint32_t	secsSince1900;
	uint8_t   server_list_index;
	uint8_t		retries_counter;
  uint8_t   fase;
  uint8_t   err;
};


//  Variables for debugging and statistics
extern struct ntp_stru ntps;

void ntpRequest(void);

uint32_t parseAnswer(uint8_t* data, uint8_t datalen);



#endif // _NTP_H_
