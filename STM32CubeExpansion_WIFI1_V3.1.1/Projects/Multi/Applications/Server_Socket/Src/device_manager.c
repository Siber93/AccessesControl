#include "device_manager.h"
#include <string.h>
#include "wifi_interface.h"
#include "ntp.h"



struct dm_stru dms;

// Protocol for discovery (udp)
char *protocolD = "u";

// Protocol for commands (tcp)
char *protocolC = "t";

WiFi_Status_t status;

/*
 *		Init Struct
*/
void DM_Init(char* ip, int len)
{
	dms.addr = ip;
	dms.addr_len = len;
	dms.d1_state = 0;
	dms.d2_state = 0;
	
}


/*
 *		Send UDP Broadcast request
*/
void SendDiscovery()
{
	char     buffer[ DISCOVERY_PKT_SIZE ];  

  // Initialize message for Discovery request
  memset( buffer, 0, DISCOVERY_PKT_SIZE );
  buffer[0] = DISCOVERY_SEQ1_BYTE;
  buffer[1] = DISCOVERY_SEQ2_BYTE;
	buffer[2] = dms.addr_len;
	strcpy(buffer+3, dms.addr);

	// Open the socket
	status =	wifi_socket_client_open((uint8_t *)DISCOVERY_ADDRESS, DISCOVERY_PORT, (uint8_t *)protocolD, &dms.discovery_socket);
	if(status!=WiFi_MODULE_SUCCESS)
  {
    printf("Error in socket opening");
    return;
  }
	HAL_Delay(500);
	
	// Send UDP Packet
  status = wifi_socket_client_write(dms.discovery_socket,3 + dms.addr_len, buffer);
  if(status!=WiFi_MODULE_SUCCESS)
  {
    printf("Error on sending the pkt");
    return;
  }	
}


/*
 *		Send UDP Broadcast request
*/
void DM_Kernel(dsm_state_t state)
{
	switch(state)
	{
		case dsm_state_reset:
			// Open socket for commands
			status = wifi_socket_server_open(COMMAND_PORT, (uint8_t *)protocolC, &dms.command_socket);
			// Prepare to send multicast discovery			
			state = dsm_state_send_multicast;
			break;
		case dsm_state_send_multicast:
			// Send Multicast
			SendDiscovery();
			state = dsm_state_discovering;
			dms.discovery_tmstmp = ntps.secsSince1900;
			break;
		case dsm_state_discovering:
			if(dms.d1_state && dms.d2_state)  // <-- Implementare l'attivazione di questi flag (quando si connettono nuovi dispositivi via tcp ed inviano un packetto con scritto chi sono)
			{
				// Devices found
				state = dsm_state_ready;
			}
			else
			{
				// Check if it's necessary to send the pkt again
				if(dms.discovery_tmstmp - ntps.secsSince1900 >= DISCOVERY_RETRY_TIME )
				{
					state = dsm_state_send_multicast;
				}
				else
				{
					// Keep waiting
					printf(".");
				}					
			}
			break;
		case dsm_state_ready:
			break;
		case dsm_state_error:
			break;
		default:
			break;
	}
}



