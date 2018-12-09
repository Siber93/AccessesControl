/*
 *
 *  NTP Client on STM32-E407 with ChibiOs
 *
 *  Copyright (c) 2015 by Jean-Michel Gallego
 *
 *  Please read file ReadMe.txt for instructions
 *
 *  This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 *  This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 */
 
 
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
#define NTP_DELAY_SYNCHRO               10

// Delay in minutes between retries when failure
#define NTP_DELAY_FAILURE               2

// Number of seconds between January 1900 and January 1970 (Unix time)
#define NTP_SEVENTY_YEARS               2208988800UL

// Clock speed * number of seconds
#define NTP_ANSWER_WAIT_TIME            84000000 * 3

#define NTP_PACKET_SIZE                 48

#define NTP_SCHEDULER_THREAD_STACK_SIZE 768

#define NTP_SCHEDULER_THREAD_PRIORITY   (LOWPRIO + 2)


// define a structure of parameters for debugging and statistics
struct ntp_stru
{
  char* 		addr;
	uint8_t		server_id;
  uint32_t  unixLocalTime;
  uint32_t  unixTime;
  uint32_t  lastUnixTime;
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
