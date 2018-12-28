#include <string.h>
#include <stdlib.h>
#include "device_manager.h"
#include "wifi_interface.h"
#include "ntp.h"
#include "file_manager.h"

// Addre for discovery, by default it is the broadcast
char* DISCOVERY_ADDRESS;

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
	memset(dms.d_state,0,MAX_DEVICE_NUMBER);
	dms.people = 0;
	
	if(DISCOVERY_ADDRESS != NULL)
	{
		free(DISCOVERY_ADDRESS);
	}
	
	// Generate broadcast address
	uint8_t i = 0;
	for(i = dms.addr_len-1; i >= 0; i--)
	{
		if(dms.addr[i]=='.')
		{
			break;
		}				
	}
	
	uint8_t l = i+1;
	DISCOVERY_ADDRESS = (char*)malloc (l+3);
	for(i = 0; i < l; i++)
	{
		DISCOVERY_ADDRESS[i] = dms.addr[i];
	}
	DISCOVERY_ADDRESS[l] = '2';
	DISCOVERY_ADDRESS[l+1] = '5';
	DISCOVERY_ADDRESS[l+2] = '5';
	
	
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
	
	// Send UDP Packet
  status = wifi_socket_client_write(dms.discovery_socket,3 + dms.addr_len, buffer);
  if(status!=WiFi_MODULE_SUCCESS)
  {
    printf("\r\n >>Error on sending the pkt");
    return;
  }	
}

/*
 *		Send UDP Broadcast request
*/
void DM_Kernel(dsm_state_t* dsm_st)
{
	switch(*dsm_st)
	{
		case dsm_state_reset:
			printf("\r\n >>Opening Command socket...");
			// Open socket for commands
			status = wifi_socket_server_open(COMMAND_PORT, (uint8_t *)protocolC, &dms.command_socket);
			if(status!=WiFi_MODULE_SUCCESS)
			{
				printf("\r\n >>Error in CMD socket opening");
				return;
			}			
			
			printf("\r\n >>Opening Discovery socket...");
			// Open the socket for discovery
			status =	wifi_socket_client_open((uint8_t *)DISCOVERY_ADDRESS, DISCOVERY_PORT, (uint8_t *)protocolD, &dms.discovery_socket);
			if(status!=WiFi_MODULE_SUCCESS)
			{
				printf("\r\n >>Error in DSC socket opening");
				
				return;
			}
			
			// Prepare to send multicast discovery			
			*dsm_st = dsm_state_send_multicast;
			break;
		case dsm_state_send_multicast:
			printf("\r\n >>Send Discovery");
			// Send Multicast
			SendDiscovery();
			*dsm_st = dsm_state_discovering;
			dms.discovery_tmstmp = ntps.secsSince1900;
			printf("\r\n >>Waiting for the answer\r\n");
			break;
		case dsm_state_discovering:			
			// Check if it's necessary to send the pkt again
			if(dms.discovery_tmstmp - ntps.secsSince1900 >= DISCOVERY_RETRY_TIME )
			{
				*dsm_st = dsm_state_send_multicast;
			}
			else
			{
				// Keep waiting
				HAL_Delay(1000);
				printf(".");
			}	
			break;
		case dsm_state_ready:
			// Not used yet. Maybe settable after retries limit reached.
			break;
		case dsm_state_error:
			break;
		default:
			break;
	}
}

/*
 * Check if the pkt is from our protocol
*/
uint8_t DM_CheckCommand(uint8_t* data, uint8_t len)
{
	return data[0] == 0xFE && data[0] == 0xFF ? 1 : 0;
}


/*
 * Check if the pkt is from our protocol
*/
void DM_ParseCommand(uint8_t* data, uint8_t len)
{
	switch(data[2])
	{
		case 0x00:
			break;
		case 0x01:
			// Discovery Ack
			printf("\r\n >>D_ACK Received, New Device connected");
			for(int i = 0; i < MAX_DEVICE_NUMBER; i++)
			{
					if(i == data[3])
					{
						dms.d_state[i] = 1;
						printf("--> %d",i);
						break;
					}
			}
			break;
		case 0x02:
			// Notify
			// parse notify type
			switch(data[4])
			{
				case 0x01:
					// Entry
					dms.people += data[5];
					printf("\r\n >>[N] %lu - %d Entries - %lu",(unsigned long)(ntps.secsSince1900 - NTP_SEVENTY_YEARS), data[5],(unsigned long)dms.people);
					break;
				case 0x02:
					// Leaving
					dms.people -= data[5];
					printf("\r\n >>[N] %lu - %d Leavings - %lu",(unsigned long)(ntps.secsSince1900 - NTP_SEVENTY_YEARS), data[5],(unsigned long)dms.people);
					break;
				default:
					// Error
					printf("\r\n >>[N] %lu - Error, Unknown PKT",(unsigned long)(ntps.secsSince1900 - NTP_SEVENTY_YEARS));
					break;
			}		
			// Print log			
			FM_AddValue(dms.people);
			break;
		default:
			// Not Supported
			printf("\r\n >>[UKNW_CMD]");
			break;
	}
}



