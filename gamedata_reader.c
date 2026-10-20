#include <stdio.h>
#include <stdlib.h>
#include <windows.h>
#include <dirent.h>
#include <unistd.h>
#include <stdbool.h>
#include <conio.h>
#include "gamedata_reader.h"

/*Constantes para identificar os formatos de arquivos
mais comuns apartir do Magic ID*/

const char EWDF_ID[5]={"EWDF"};
const char P2N_ID[5]={"P2N"};
const char PS2D_ID[5]={"PS2D"};
const char DTPK_ID[10]={"ps2_DTPK"};
const char PWF_ID[5]={"PWF "};
const char EMP_ID[5]={"2EMP"};
const char AFS_ID[5]={"AFS"};

//Diretório principal onde serão salvos todos os arquivos extraídos
const char *MAIN_DIR={"GAMEDATA"};

//Diretórios para formatos mais comuns de arquivos no container
const char SUB_DIRS[7][30]=
{
	"EWDF\\",
	"P2N\\",
	"PWF\\",
	"PS2D\\",
	"2EMP\\",
	"DTPK\\",
	"AFS\\"
};

int Pack_Size;
int group1_extractor, group2_extractor;

void ExtractFiles(FILE *wad)
{
	char id[4];
	fread(id, 1, 4, wad);
	if (memcmp(id, PWF_MAGIC, 4) != 0)
	{
		printf("The archive has no the Magic ID '%s'\n", PWF_MAGIC);
		return;
	}

	CreateDirectory(MAIN_DIR, NULL);
	DIR *dir = opendir(MAIN_DIR);
	if (!dir)
	{
		printf("Failed to create the output directory '%s'\n", MAIN_DIR);
		return;
	}

	char path[30];
	strcpy(path, MAIN_DIR);
	strcat(path, "\\");
	if (!CreateSubDirectories(path))
	{
		printf("Failed when trying to create one or more subdirectories to save the extracted files!\n");
		return;
	}

	Pack_Size = GetFilesize(wad);

	int total, t = 0, extracted = 0, no_data = 0;
	int GROUP2, GROUPS_END;
	group1_extractor = 0;
	group2_extractor = 0;

	total = Get_WAD_Files_Num(wad, &GROUP2, &GROUPS_END);
	if (!total) return;

	FILELIST *FILES = AllocLIST(total);
	if (!FILES) return;
	
	FILE *main_log = fopen("GAMEDATA-LSN.TXT", "w");
	if (!main_log)
	{
		free(FILES);
		printf("Failed to create the GAMEDATA-LSN.TXT log file\n");
		return;
	}

	unsigned char *buff = AllocMem();
	if (!buff)
	{
		free(FILES);
		return;
	}

	while (1)
	{
		unsigned char entry[8];
		fread(entry, 1, 8, wad);
		int next_entry_off = ftell(wad);
		int lba, filesize;
		
		if (next_entry_off > GROUPS_END) break;
		
		if (!EntryIsValid(entry))
		{
			printf("The entry has no data!\nSkipping...\n");
			no_data++;
		}
		else
		{
			lba = GetLBA(entry);	
			
			byte format = NONE_FILE;
			filesize = GetSize(entry);
			if (filesize)
				format = GetMagicID(lba, wad);

			char filename[256], dest[256];
			strcpy(dest, path);

			fseek(wad, lba, SEEK_SET);
			unsigned char buffer[64];
			fread(buffer, 1, 64, wad);

			if (format == EWDF_FILE)
			{
				strcat(dest, SUB_DIRS[EWDF_DIR]);
				if (!buffer[16])
					CreateName(filename, "Unnamed file ", t);
				else
				{
					int i = 16;
					char index[10];
					
					itoa(t + 1, index, 10);
					strcpy(filename, "0000");
					strcat(filename, index);
					strcat(filename, "_");
						
					char *f = filename + strlen(filename);
					*f++ = buffer[i++];

					for (; buffer[i] != 0 && i < 64; i++)
					{
						if (buffer[i] == '/') buffer[i] = '_';
							*f++ = buffer[i];
					}
					*f = 0;
				}
			}
			else if (format == P2N_FILE)
				RenameFile(dest, filename, ".P2N", SUB_DIRS[P2N_DIR], t);
			else if (format == PWF_FILE)
				RenameFile(dest, filename, ".PWF", SUB_DIRS[PWF_DIR], t);
			else if (format == EMP_FILE)
				RenameFile(dest, filename, ".2EMP", SUB_DIRS[EMP_DIR], t);
			else if (format == PS2D_FILE)
				RenameFile(dest, filename, ".PS2D", SUB_DIRS[PS2D_DIR], t);
			else if (format == DTPK_FILE)
				RenameFile(dest, filename, ".DTPK", SUB_DIRS[DTPK_DIR], t);
			else if (format == AFS_FILE)
				RenameFile(dest, filename, ".AFS", SUB_DIRS[AFS_DIR], t);
			else
				CreateName(filename, "Unnamed file ", t);
	
			strcat(dest, filename);
			fseek(wad, lba, SEEK_SET);
				
			FILE *save = fopen(dest, "wb");
			if (!save)
			{
				printf("Failed to create a file at '%s'\n", dest);
				free(buff);
				return CloseFiles(main_log, FILES);
			}
				
			SaveToFile(wad, save, filesize, buff);
			fclose(save);
			SaveDataToLog(&FILES[extracted], next_entry_off, entry, lba, dest);
			extracted++;
			printf("%s\n", dest);
			if (next_entry_off - 8 < GROUP2) group1_extractor++;
			else group2_extractor++;

			fseek(wad, next_entry_off, SEEK_SET);
		}
		t++;
	}

	SortLBA(FILES, extracted);
	for (int i = 0; i < extracted; i++)
		fprintf(main_log, "|%d||%02X||%02X||%X|%s\n", FILES[i].entry_off, FILES[i].lba_flag, FILES[i].size_flag, FILES[i].lba, FILES[i].path);

	CloseFiles(main_log, FILES);
	free(buff);
	
	printf("\n%d/%d extracted files\n", extracted, total);
	printf("Entries without valid data: %d (LBA == 0 || FILESIZE == 0)\n", no_data);
	if (total == NTSC_PAK_FILES)
	{
		printf("Extracted files from GROUP1 (0x20-0xD607): %d\n", group1_extractor);
		printf("Extracted files from GROUP2 (0xD608-0xD8D8): %d\n", group2_extractor);
	}
	else
	{
		printf("Extracted files from GROUP1 (0x20-0xD617): %d\n", group1_extractor);
		printf("Extracted files from GROUP2 (0xD618-0xD8E8): %d\n", group2_extractor);
	}
}

void RenameFile(char *dest, char *filename, const char *ext, const char *dir, int idx)
{
	strcat(dest, dir);
	CreateName(filename, "Unnamed file", idx);
	strcat(filename, ext);
}

void SortLBA(FILELIST *FILES, int extracted)
{
	for (int i = 0; i < extracted; i++)
	{
		for (int j = 0; j < extracted - 1; j++)
		{
			if (FILES[j].lba > FILES[j + 1].lba)
			{
				FILELIST tmp = FILES[j + 1];
				FILES[j + 1] = FILES[j];
				FILES[j] = tmp;
			}
		}
	}
}

void CloseFiles(FILE *f1, FILELIST *FILES)
{
	if (f1) fclose(f1);
	if (FILES) free(FILES);
}

void SaveToFile(FILE *src, FILE *dest, int filesize, byte *buff)
{
	while (filesize)
	{
		if (filesize >= BUFFER_SIZE)
		{
			fread(buff, 1, BUFFER_SIZE, src);
			fwrite(buff, 1, BUFFER_SIZE, dest);
			filesize -= BUFFER_SIZE;
		}
		else
		{
			fread(buff, 1, filesize, src);
			fwrite(buff, 1, filesize, dest);
			filesize = 0;
		}
	}	
}

void SaveDataToLog(FILELIST *FILES, int entry_off, byte *Entry, int lba, char *dest)
{
	FILES->lba_flag = 0;
	FILES->size_flag = 0;

	if (Entry[7] >= 0x80)
	{
		FILES->lba_flag = Entry[2];
		FILES->size_flag = Entry[7];
	}
	
	FILES->lba = lba;
	FILES->entry_off = entry_off - 8;
	strcpy(FILES->path, dest);
}

bool CreateSubDirectories(char *path)
{
	int i = 0;
	while (i < SUB_DIRS_COUNT)
	{
		char dir[50];
		strcpy(dir, path);
		strcat(dir, SUB_DIRS[i]);
		CreateDirectory(dir, NULL);
	
		DIR *check = opendir(dir);
		if (!check) return false;
		closedir(check);
		i++;
	}
	return true;
}

void CreateName(char *dest, const char *filename, int idx)
{
	char index[10];
	itoa(idx + 1, index, 10);
	strcpy(dest, "0000");
	strcat(dest, index);
	strcat(dest, "_");
	strcat(dest, filename);
}

bool EntryIsValid(byte *entry)
{
	int size;
	memcpy(&size, entry + 4, 4);
	if (!size) return false;
	return true;
}

int GetSize(byte *Entry)
{
	int u32_size;
	memcpy(&u32_size, Entry + 4, 4);
	int u20_size = ((Entry[5] & 0x0f) << 16) | ((Entry[4] << 8) | Entry[3]);
	u20_size *= 4;
	
	int size;

	if (Entry[7] >= 0x80)
		size = u20_size;
	else if (Entry[7] == 0)
		size = u32_size;
	else
		size = 0;
	
	return size;
}

int GetLBA(byte *Entry)
{
	if (Entry[3] >= 1 && Entry[3] <= 0x24 && Entry[7] == 0)
	{
		return ((Entry[3] << 24) | (Entry[2] << 16) | (Entry[1] << 8) | (Entry[0]));
		//32-bit lba absolute
	}
	
	byte hn = Entry[2] >> 4;
	byte ln = Entry[2] & 0x0f;
	int base1 = (Entry[2] << 16) | (Entry[1] << 8) | Entry[0];
	int base2 = ((Entry[2] & 0x3F) << 16) | (Entry[1] << 8) | Entry[0];
	int lba;

	if (Entry[3])
	{
		if (hn == 4 || hn == 8 || hn == 12)
			lba = base2;
		else
			lba = base1;
	}
	else
		lba = base1;

	if ((lba & 0x7FF) != 0)
	{
		lba <<= 11;
		if ((lba & 0x7FF) == 0 && lba < Pack_Size)
			return lba;
		else
			lba = 0;
	}
	else if (lba > Pack_Size)
		lba = 0;

	return lba;
}

byte GetMagicID(int lba, FILE *wad)
{
	fseek(wad, lba, SEEK_SET);
	char id[10];
	fread(id, 1, 8, wad);

	if (!memcmp(id, EWDF_ID, 4)) return EWDF_FILE;
	if (!memcmp(id, P2N_ID, 3)) return P2N_FILE;
	if (!memcmp(id, PS2D_ID, 4)) return PS2D_FILE;
	if (!memcmp(id, DTPK_ID, 8)) return DTPK_FILE;
	if (!memcmp(id, EMP_ID, 4)) return EMP_FILE;
	if (!memcmp(id, PWF_ID, 4)) return PWF_FILE;
	if (!memcmp(id, AFS_ID, 4)) return AFS_FILE;
	return NONE_FILE;
}

int Get_WAD_Files_Num(FILE *wad, int *GROUP2, int *GROUPS_END)
{
	int total;
	fseek(wad, ENTRIES_COUNT_OFF, SEEK_SET);
	fread(&total, 1, 4, wad);
	fseek(wad, GROUP1, SEEK_SET);

	if (total == NTSC_PAK_FILES)
	{
		*GROUPS_END = NTSC_VER_GROUPS_END;
		*GROUP2 = NTSC_VERSION_GROUP2;
	}
	else if (total == PAL_PAK_FILES)
	{
		*GROUPS_END = PAL_VER_GROUPS_END;
		*GROUP2 = PAL_VERSION_GROUP2;
	}
	else
	{
		printf("The GAMEDATA.WAD version is invalid!\n");
		total = 0;
	}
	return total;
}

FILELIST *AllocLIST(int size)
{
	FILELIST *FILES = malloc(sizeof(*FILES)* size);
	if (!FILES)
	{
		printf("Out of memory to store the files infos\n");
		return NULL;
	}
	return FILES;
}

unsigned char *AllocMem()
{
	unsigned char *buff;
	buff = malloc(sizeof(*buff)* BUFFER_SIZE);
	if (!buff)
		printf("Insufficient memory to start the extraction/Rebuilding\n");
	return buff;
}

int GetFilesize(FILE *wad)
{
	fseek(wad, 0, SEEK_END);
	int size = ftell(wad);
	rewind(wad);
	return size;
}

