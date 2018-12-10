#include <stdint.h>


#define DISCOVERY_PKT_SIZE											18
#define DISCOVERY_ADDRESS												"224.0.0.200"
#define DISCOVERY_PORT													123
#define COMMAND_PORT														124
#define DISCOVERY_RETRY_TIME										3

#define DISCOVERY_SEQ1_BYTE											(char)0xFE
#define DISCOVERY_SEQ2_BYTE											(char)0xFF

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
  uint8_t   			d1_state;
	uint8_t   			d2_state;
	uint8_t   			discovery_socket;
	uint8_t   			command_socket;
	uint32_t				discovery_tmstmp;
};

extern struct dm_stru dms;