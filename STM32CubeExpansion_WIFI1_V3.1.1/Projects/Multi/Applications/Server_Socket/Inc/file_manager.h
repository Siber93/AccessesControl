#include <stdint.h>

#define MAX_FILE_SIZE													1024
#define DEFAULT_ROW_SIZE											31
#define MAX_FILENAME_LENGTH										15
#define WRITE_TIMEOUT													120 // Refresh file time on sdcard

void FM_AddValue(uint8_t val);
void FM_Check_WTimeout();
void FM_Force_Save();
