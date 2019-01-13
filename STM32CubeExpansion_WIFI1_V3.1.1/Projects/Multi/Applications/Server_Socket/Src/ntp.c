#include "ntp.h"
#include "wifi_interface.h"
#include <string.h>


struct ntp_stru ntps;


char *ntp_server_list[] = { "0.it.pool.ntp.org", 
														"1.it.pool.ntp.org", 
														"2.it.pool.ntp.org", 
														"3.it.pool.ntp.org" };


// Protocol for NTP (udp)
char *protocol = "u";


char *hostname = "0.it.pool.ntp.org";

void SetTimeUnixSec( uint32_t ut )
{
  /*RTCDateTime timespec;
  struct tm * ptim;

  ptim = gmtime( & ut );
  rtcConvertStructTmToDateTime( ptim, 0, & timespec );
  rtcSetTime( & RTCD1, & timespec );
	*/
}


void ntpClient( void)
{

  char     buffer[ NTP_PACKET_SIZE ];  

  ntps.fase = 1;
  ntps.err = 0;

 
  // Initialize message for NTP request
  memset( buffer, 0, NTP_PACKET_SIZE );
  buffer[0] = 0xE3;   // LI, Version, Mode
  buffer[1] = 0;     // Stratum, or type of clock
  buffer[2] = 6;     // Polling Interval
  buffer[3] = 0xEC;  // Peer Clock Precision
  // 8 bytes of zero for Root Delay & Root Dispersion
  buffer[12] = 49;
  buffer[13] = 0x4E;
  buffer[14] = 49;
	buffer[15] = 52;


	ntps.addr = ntp_server_list[ ntps.server_list_index ];

	// Opening the socket
	ntps.fase = 2;
	WiFi_Status_t status =	wifi_socket_client_open((uint8_t *)ntp_server_list[ ntps.server_list_index ], NTP_PORT, (uint8_t *)protocol, &ntps.server_id);
	if(status!=WiFi_MODULE_SUCCESS)
  {
    printf("Error in socket opening");
    return;
  }
	HAL_Delay(500);
	
	// Send UDP Packet
  ntps.fase = 3;
  status = wifi_socket_client_write(ntps.server_id,NTP_PACKET_SIZE,buffer);
  if(status!=WiFi_MODULE_SUCCESS)
  {
    printf("Error in socket opening");
    return;
  }
	//HAL_Delay(500);	
	
}



// Check if the answer from the nto server is correct
uint32_t parseAnswer(uint8_t* data, uint8_t datalen)
{
	/// RESPONSE
	uint8_t  mode, stratum;
	
  // Check length of response
  ntps.fase = 4;
  if( datalen < NTP_PACKET_SIZE )
  {
    ntps.err = datalen;
    return 0;
  }

  // Check Mode (must be Server or Broadcast)
  ntps.fase = 5;
  mode = data[ 0 ] & 7;//0b00000111;
  if( mode != 4 && mode != 5 )
  {
    ntps.err = mode;
    return 0;
  }

  // Check stratum != 0 (kiss-of-death response)
  ntps.fase = 6;
  stratum = data[ 1 ];
  if( stratum == 0 )
	{
		return 0;
	}

  // Combine four high bytes of Transmit Timestamp to get NTP time
  //  (seconds since Jan 1 1900):
  ntps.fase = 0;
  ntps.secsSince1900 = data[ 40 ] << 24 | data[ 41 ] << 16 |
                  data[ 42 ] <<  8 | data[ 43 ];
	
	
	ntps.lastNtpRequest = ntps.secsSince1900;

  if( ntps.secsSince1900 == 0L )
  {
    // ERROR
    return 0L;
  }
  return ntps.secsSince1900;
}

void ntpRequest( void )
{
	ntpClient();
	
  // Exit if no valid response
  /*if( secsSince1900 == 0 )
    return false;

  // Save current local time
  rtcGetTime( & RTCD1, & timespec );
  rtcConvertDateTimeToStructTm( & timespec, & timp, NULL );
  ntps.unixLocalTime = mktime( & timp );

  // Update RTC
  ntps.lastUnixTime = ntps.unixTime;
  ntps.unixTime = secsSince1900 - NTP_SEVENTY_YEARS + NTP_LOCAL_DIFFERERENCE;
  rtcConvertStructTmToDateTime( gmtime( & ntps.unixTime ), 0, & timespec );
  rtcSetTime( & RTCD1, & timespec );

  return true;*/
}

// Print statistic

void ntpPrint( void )
{
  /*uint32_t    elapsedTime;
  uint32_t    lag;
  uint32_t    drift = 0;

  CONSOLE_PRINT( "\n\rRTC updated from NTP server %s\n\r", ipaddr_ntoa( & ntps.addr ));
  CONSOLE_PRINT( "Local time:   %s\r", asctime( gmtime( & ntps.unixLocalTime )));
  CONSOLE_PRINT( "Updated time: %s\r", asctime( gmtime( & ntps.unixTime )));
  if( ntps.lastUnixTime > 0 )
  {
    elapsedTime = ntps.unixTime - ntps.lastUnixTime;
    lag = ntps.unixLocalTime - ntps.unixTime;
    if( lag != 0 )
      drift = 10000 * lag / elapsedTime;
    CONSOLE_PRINT( "Time since last update: %i s\r\n", elapsedTime);
    CONSOLE_PRINT( "Time lag:     %D s\r\n", lag );
    CONSOLE_PRINT( "Drift:        %d.%02u %%\r\n", drift / 100, drift % 100 );
  }*/
}

