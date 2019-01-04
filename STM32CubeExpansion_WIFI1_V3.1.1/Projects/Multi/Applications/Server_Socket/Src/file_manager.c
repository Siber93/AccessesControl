#include "file_manager.h"
#include "wifi_interface.h"
#include "ntp.h"
#include <string.h>


uint16_t page_index = 0; 
uint8_t page[MAX_FILE_SIZE];
uint8_t file_name[15];
uint8_t dirty = 1;
// Counter to decide when write file to sdcard
uint32_t write_counter = 0;


void CopyTimeToStr(uint8_t* str)
{
	uint32_t num = ntps.secsSince1900-NTP_SEVENTY_YEARS;
	for(int i=9; i>=0; i--)
	{
		str[i] = 0x30 + num % 10;
		num = num / 10;
	}
}
/*
*			Max number 99, and leave what there was before on the first char if the number is <10
*/
void CopyValueToStr(uint8_t* str, uint8_t num)
{
	str[0] = ' ';
	str[1] = ' ';
	for(int i=1; i>=0; i--)
	{
		str[i] = 0x30 + num % 10;
		num = num / 10;
		if(num == 0)
			break;
	}
}


void FM_Init(void)
{
	// Init File with all sapces
	//memset( page, ' ', DEFAULT_ROW_SIZE );
	// Init brackets
	page[0] = '[';
	page_index = 1;
	page[1] = ']';
	
	// init file name
	memset(file_name, '0', 15);
	file_name[10] = '.';
	file_name[11] = 'j';
	file_name[12] = 's';
	file_name[13] = 'o';
	file_name[14] = 'n';

	CopyTimeToStr(file_name);
	
	// Create the new file
	wifi_file_create((char*)file_name,2,(char*)page);
	
	// Create file name string to appen to db_index.txt
	char index_file_name[] = "db_index.txt";
	char new_file_name[strlen((char*)file_name)+1];
	strcpy(new_file_name+1,(char*)file_name);
	new_file_name[0] = ',';
	
	// Add it to the list
	wifi_file_create((char*)index_file_name,strlen((char*)new_file_name),(char*)new_file_name);
}



void FM_AddValue(uint8_t val)
{	
	if(dirty)
	{
		FM_Init();
		dirty = 0;
	}
	else
	{
		// substitute last "]" char with ','
		page[page_index-1] = ',';
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
	page_index = page_index + 10;
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
	page[page_index++] = ']';
	
	// Check if we have reached the max_file_size length and we have to create new file
	if(page_index+DEFAULT_ROW_SIZE >= MAX_FILE_SIZE)
	{
		// Page finished on the next record
		dirty = 1;
		
	}	
}


void FM_Check_WTimeout()
{
	
	// Check if it's time to refresh the counter for write on sd card or dirty flag is set
	if(page_index > 0 
		&& (write_counter + WRITE_TIMEOUT < ntps.secsSince1900 
				|| dirty))
	{
		// Refresh timer
		write_counter = ntps.secsSince1900;
		// Delete the file
		wifi_file_delete((char*)file_name);
		// Create it again
		wifi_file_create((char*)file_name,page_index,(char*)page);
	}
}





