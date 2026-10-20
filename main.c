#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <locale.h>
#include <conio.h>
#include "gamedata_reader.h"
#include "gamedata_builder.h"

static const char *in_filename ={"GAMEDATA.WAD"};

int main(int argc, char *argv[])
{
	setlocale(LC_ALL, "Portuguese");
/*	int v = 0x7C8D0;
	unsigned char b[2];
	b[0] = 0x81;
	unsigned char c = b[0] >> 4;
	printf("%X\n", (b[0] & 0xf0) | ((v >> 16) & 0x0F));
	printf("%X\n", ((v >> 16) & 0xFF));

	unsigned char data[4]={0x00, 0x90, 0xd5, 0x27};
	unsigned int base = (data[2] << 16) | (data[1] << 8) | data[0];
	size_t lba_array[25];

	lba_array[0] = base;
	lba_array[1] = ((data[2] & 0x3F) << 16) | (data[1] << 8) | data[0];
	lba_array[2] = ((data[2] & 0x7F) << 16) | (data[1] << 8) | data[0];
	lba_array[3] = (data[1] << 8) | data[0];
	lba_array[4] = base << 1;
	lba_array[5] = base << 2;
	lba_array[6] = ((data[3] << 16) | (data[1] << 8) | data[0]);
	lba_array[7] = (data[0] << 16) | (data[1] << 8) | data[2];	
	lba_array[8] = ((data[2] & 0x3F) << 16 | data[1] << 8 | data[0]) << 1;
	lba_array[9] = ((data[2] & 0x3F) << 16 | data[1] << 8 | data[0]) << 2;
	lba_array[10] = (data[1] << 16) | (data[2] << 8) | data[3];
	lba_array[11] = ((data[1] << 16) | (data[2] << 8) | data[3]) * 2048;
	lba_array[12] = ((data[3] & 0x3F) << 16) | (data[2] << 8) | data[1];
	lba_array[13] = ((data[3] & 0x1F) << 16);
	lba_array[14] =	((data[3] & 0x3F) << 16) | (data[2] << 8) |	data[1];
	lba_array[15] =	lba_array[14] << 1;
	lba_array[16] =	lba_array[14] << 2;
	lba_array[17] =	lba_array[14] << 3;
	lba_array[18] = (data[3] << 16) | (data[1] << 8) | data[2];
	lba_array[19] = ((data[3] & 0x3F) << 17) | (data[2] << 9) | (data[1] << 1);
	lba_array[20] = ((data[3] << 16) | (data[2] << 8) | data[1]) & 0x3FFFFF;
	lba_array[20] *= 2048;
*/
	printf("Unpacker/Packer for Mortal Kombat Shaolin Monks (PS2) by Raynê Games\n");
	printf("Tool designed to extract the contents/rebuild the GAMEDATA.WAD archive\n");
	printf("v1.2\n");
	
	if (argc != 2)
	{
		printf("\nNone argument!\nUse -e to extract or -r to rebuild\n");
		return 1;
	}
	if (strcmp(argv[1], "-e") != 0 && strcmp(argv[1], "-r") != 0)
	{
		printf("Invalid argument!\nMust be -e to extract or -r to revuild\n");
		return 1;
	}

	FILE *wad = fopen(in_filename, "rb");
	if (!wad)
	{
		printf("Unable to find the '%s'\n", in_filename);
		return 1;	
	}
	if (!strcmp(argv[1], "-e"))
		ExtractFiles(wad);
	else BuildGAMEDATA(wad);
		
	fclose(wad);
	return 0;
}

