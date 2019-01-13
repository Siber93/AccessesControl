#include <stdint.h>

#define DISCOVERY_PKT_SIZE											18
#define DISCOVERY_PORT													123
#define COMMAND_PORT														124
#define DISCOVERY_RETRY_TIME										10

#define DISCOVERY_SEQ1_BYTE											(char)0xFE
#define DISCOVERY_SEQ2_BYTE											(char)0xFF
#define MAX_DEVICE_NUMBER												10

#define MAX_PIR_DELTA_TIME											60*20 // 20 mins



typedef enum {
  dsm_state_reset = 0,
  dsm_state_ready,
  dsm_state_send_multicast,
	dsm_state_discovering,
  dsm_state_error,
  dsm_undefine_state       = 0xFF,
} dsm_state_t;


struct dm_stru
{
  char* 					addr;
	uint8_t					addr_len;
  uint8_t   			d_state[MAX_DEVICE_NUMBER];
	uint8_t   			discovery_socket;
	uint8_t   			command_socket;
	uint32_t				discovery_tmstmp;
	uint16_t				people;
	uint8_t					print_flag;
};

extern struct dm_stru dms;




uint8_t DM_CheckCommand(unsigned char* data, uint8_t len);
void DM_ParseCommand(uint8_t* data, uint8_t len);
void DM_Kernel(dsm_state_t* state);
void DM_Init(char* ip, int len);
void DM_Force_Save();
