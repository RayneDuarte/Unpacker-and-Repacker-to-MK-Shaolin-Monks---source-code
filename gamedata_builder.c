#include <stdio.h>
#include <stdlib.h>
#include <windows.h>
#include <dirent.h>
#include <unistd.h>
#include <stdbool.h>
#include <conio.h>
#include "gamedata_reader.h"
#include "gamedata_builder.h"

void BuildGAMEDATA(FILE *wad)
{
	FILE *log = fopen("GAMEDATA-LSN.TXT", "r");
	if (!log)
	{
		printf("The log file called 'GAMEDATA-LSN.TXT' of the extracted files from old GAMEDATA.WAD was not found!\n");
		return;	
	}
	
	FILE *newpack = fopen("NEW_GAMEDATA.WAD", "wb");
	if (!newpack)
	{
		fclose(log);
		printf("Unable to create the NEW_GAMEDATA.WAD to disk\n");
		return;
	}

	int total, GROUP2, GROUPS_END;
	total = Get_WAD_Files_Num(wad, &GROUP2, &GROUPS_END);
	if (!total)
		return CloseResources(NULL, NULL, NULL, log, newpack);
	
	FILELIST *FILES = AllocLIST(total);
	if (!FILES)
		return CloseResources(FILES, NULL, NULL, log, newpack);
	int i = 0;

	unsigned char *table = malloc(sizeof(*table)* ARCHIVE_TABLE_SIZE);
	if (!table)
	{
		printf("Insufficient memory to load the files table from GAMEDATA.WAD\n");
		return CloseResources(FILES, table, NULL, log, newpack);
	}

	rewind(wad);
	fread(table, 1, ARCHIVE_TABLE_SIZE, wad);
	fwrite(table, 1, ARCHIVE_TABLE_SIZE, newpack);

	unsigned char *buff = AllocMem();
	if (!buff) return CloseResources(FILES, table, buff, log, newpack);

	unsigned char sentinel_block[2048];
	memset(sentinel_block, 0, 2048);

	while (1)
	{
		int entry_off, lba_flag, size_flag, file_lba;
		char path[260];
		
		int ret = fscanf(log, "|%d||%X||%X||%X|%259[^\n]\n", &entry_off, &lba_flag, &size_flag, &file_lba, path);
		if (ret != 5) break;
		if (i >= total)
		{
			printf("The log has more entries than expected\n");
			break;
		}
		
		FILE *f = fopen(path, "rb");
		if (!f)
		{
			printf("The file '%s' is missing\n", path);
			printf("The rebuilding has been aborted\n");
			free(buff);
			return CloseResources(FILES, table, buff, log, newpack);
		}

		FILES[i].entry_off = entry_off;
		FILES[i].lba_flag = lba_flag;
		FILES[i].size_flag = size_flag;
		FILES[i].filesize = GetFilesize(f);
		FILES[i].lba = ftell(newpack);
		
		if (FILES[i].filesize > 16)
			FILES[i].is_encoded = Is_Encoded(f);
		else
			FILES[i].is_encoded = false;

		bool rejected;
		if (FILES[i].is_encoded)
		{
			int packed_size = FILES[i].filesize / 4;
			
			if (packed_size * 4 == FILES[i].filesize &&
					packed_size <= MAX_SIZE_WITH_ENC)
			{
				if ((FILES[i].lba >> 11) <= MAX_LBA_WITH_ENC)
					rejected = false;
				else
					rejected = true;
			}
			else
				rejected = true;
		}
		else if (FILES[i].filesize <= MAX_SIZE_WITHOUT_ENC)
			rejected = false;
		else
			rejected = true;
		
		if (rejected)
		{
			if (FILES[i].is_encoded)
			{
				printf("This compressed file '%s' is too large!\n", path);
				printf("Compressed files must be less than or equal to 4194300 bytes (4.1943MB).\n");
				printf("Compressed files must also be multiples of 4\n");
				return CloseResources(FILES, table, buff, log, newpack);
			}
			else
			{
				printf("This uncompressed file '%s' is too large!\n");
				printf("Uncompressed files must be less than or equal to 16777215 bytes (15.9MB).\n");
				return CloseResources(FILES, table, buff, log, newpack);
			}
		}
		
		SaveToFile(f, newpack, FILES[i].filesize, buff);
		fclose(f);
		Padding(newpack);
		
		if (i == 1089)
		{ //write the sentinel block
			sentinel_block[0] = 0x0E;
			fwrite(sentinel_block, 1, 2048, newpack);	
		}
		
		if (((ftell(newpack) / 2048) % 2048) == 0)
    		fwrite(sentinel_block, 1, 2048, newpack);
		
		printf("%s\n", path);
		i++;
	}
	UpdateTOC(FILES, table, GROUPS_END);
	rewind(newpack);
	fwrite(table, 1, ARCHIVE_TABLE_SIZE, newpack);
	CloseResources(FILES, table, buff, log, newpack);
	printf("\nBuild complete\n%d inserted files\n", i);
}

void UpdateTOC(FILELIST *FILES, unsigned char *table, int GROUPS_END)
{
	unsigned char *groups = table + 32;
	int i = 32, j = 0;
	while (i < GROUPS_END)
	{
		int size;
		memcpy(&size, groups + 4, sizeof(int));
		
		if (size != 0)
		{
			if (FILES[j].is_encoded)
			{
				if (FILES[j].size_flag != groups[7])
				{ //utiliza a que foi escolhida manualmente no arquivo GAMEDATA_LSN.TXT
					groups[7] = (unsigned char)FILES[j].size_flag;	
				}
				else if (GetSize(groups) < FILES[j].filesize)
				{
					groups[7] += 1;
				}
								
				FILES[j].lba /= ISO_SECTOR_SIZE;
				unsigned char *e = groups + 2;
				*e = FILES[j].lba_flag;
				Packbits(e - 2, FILES[j].lba);
	
				FILES[j].filesize /= 4;
				Packbits(e + 1, FILES[j].filesize);
			}
			else
			{
				Writebits(groups, FILES[j].lba);
				Writebits(groups + 4, FILES[j].filesize);
			}
			j++;
		}
		else Writebits(groups, FILES[j].lba);
		i += 8;
		groups += 8;
	}
}

void Packbits(unsigned char *e, int v)
{	//write v as a 20-bit value
	e[0] = v & 0xff;
	e[1] = (v >> 8) & 0xff;
	e[2] = (e[2] & 0xf0) | ((v >> 16) & 0x0f);
}

void Writebits(unsigned char *e, int v)
{	//write v as a 32-bit value
	memcpy(e, &v, 4);
}

bool Is_Encoded(FILE *f)
{
	char magic[4];
	if (ftell(f) > 0) rewind(f);
	fread(magic, 1, 4, f);
	rewind(f);
	
	if (!memcmp(magic, EWDF_MAGIC, 4)) return true;
	return false;
}

void CloseResources(FILELIST *FILES, unsigned char *table, unsigned char *buff, FILE *log, FILE *newpack)
{
	if (FILES) free(FILES);
	if (log) fclose(log);
	if (newpack) fclose(newpack);
	if (table) free(table);
	if (buff) free(buff);
	return;
}

void Padding(FILE *newpack)
{
	int pos = ftell(newpack);
	int pad = ISO_SECTOR_SIZE - (pos % ISO_SECTOR_SIZE);
	
	if (pad != ISO_SECTOR_SIZE)
	{
	    char zero[2048] = {0};
	    fwrite(zero, 1, pad, newpack);
	}
}

