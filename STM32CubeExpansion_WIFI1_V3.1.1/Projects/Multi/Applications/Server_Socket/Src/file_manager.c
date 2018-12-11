#include "file_manager.h"
#include "wifi_interface.h"
#include "ntp.h"
#include <string.h>


uint16_t page_index = 0; 
uint8_t page[MAX_FILE_SIZE];
uint8_t file_name[15];
uint8_t dirty = 1;


void CopyTimeToStr(uint8_t* str)
{
	uint32_t num = ntps.secsSince1900-NTP_SEVENTY_YEARS;
	for(int i=9; i>=0; i--)
	{
		str[i] = 30 + num % 10;
		num = num / 10;
	}
}
/*
*			Max number 99, and leave what there was before on the first char if the number is <10
*/
void CopyValueToStr(uint8_t* str, uint8_t num)
{
	for(int i=2; i>=0; i--)
	{
		str[i] = 30 + num % 10;
		num = num / 10;
		if(num == 0)
			break;
	}
}


void FM_Init(void)
{
	// Init File with all sapces
	memset( page, ' ', DEFAULT_ROW_SIZE );
	// Init brackets
	page[0] = '[';
	page_index = 1;
	page[MAX_FILE_SIZE-1] = ']';
	
	// init file name
	memset(file_name, '0', 15);
	file_name[10] = '.';
	file_name[11] = 'j';
	file_name[12] = 's';
	file_name[13] = 'o';
	file_name[14] = 'n';

	CopyTimeToStr(file_name);
	
	// Create the file
	wifi_file_create((char*)file_name,MAX_FILE_SIZE,(char*)page);
}



void FM_AddValue(uint8_t val)
{	
	if(dirty)
	{
		FM_Init();
		dirty = 0;
	}
	
	// Add the row to the array
	page[page_index++] = '{';
	page[page_index++] = '"';
	page[page_index++] = 't';
	page[page_index++] = 'i';
	page[page_index++] = 'm';
	page[page_index++] = 'e';
	page[page_index++] = '"';
	page[page_index++] = ':';
	CopyTimeToStr(page+page_index);
	page_index = page_index + 9;
	page[page_index++] = ',';
	page[page_index++] = '"';
	page[page_index++] = 'v';
	page[page_index++] = 'a';
	page[page_index++] = 'l';
	page[page_index++] = 'u';
	page[page_index++] = 'e';
	page[page_index++] = '"';
	page[page_index++] = ':';
	CopyValueToStr(page+page_index,val);
	page_index = page_index + 2;
	page[page_index++] = '}';
	page[page_index++] = ',';
	
	if(page_index+DEFAULT_ROW_SIZE >= MAX_FILE_SIZE)
	{
		// Page finished on the next record
		dirty = 1;
		
	}
	// Delete the file
	wifi_file_delete((char*)file_name);
	// Create it again
	wifi_file_create((char*)file_name,MAX_FILE_SIZE,(char*)page);
}





