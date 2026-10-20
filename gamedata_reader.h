#include <stdbool.h>

#pragma once

typedef unsigned char byte;
#define PWF_MAGIC "PWF "
#define ENTRIES_COUNT_OFF 0x0C

#define GROUP1 0x20
#define NTSC_VERSION_GROUP2 0xD608
#define PAL_VERSION_GROUP2 0xD618
#define NTSC_VER_GROUPS_END 0xD8D8
#define PAL_VER_GROUPS_END 0xD8E8
#define NTSC_PAK_FILES 7139
#define PAL_PAK_FILES 7141

//constants to identify the known files after to checking the Magic ID
#define EMP_FILE 9
#define EWDF_FILE 10
#define P2N_FILE 11
#define PS2D_FILE 12
#define DTPK_FILE 13
#define PWF_FILE 14
#define AFS_FILE 15
#define NONE_FILE 0
//--------------------------------

#define ISO_SECTOR_SIZE 2048
#define BUFFER_SIZE 8388608LL //8MB

//Constants to access the subdirectories
#define SUB_DIRS_COUNT 7
#define EWDF_DIR 0
#define P2N_DIR 1
#define PWF_DIR 2
#define PS2D_DIR 3
#define EMP_DIR 4
#define DTPK_DIR 5
#define AFS_DIR 6

typedef struct
{
	char path[260];
	int entry_off;
	int lba;
	int filesize;
	int lba_flag;
	int size_flag;
	bool is_encoded;
}
FILELIST;

void ExtractFiles(FILE *wad);
void CreateName(char *dest, const char *filename, int idx);
void RenameFile(char *dest, char *filename, const char *ext, const char *dir, int idx);
void SaveDataToLog(FILELIST *FILES, int entry_off, byte *Entry, int lba, char *dest);
void SortLBA(FILELIST *FILES, int extracted);
void CloseFiles(FILE *f1, FILELIST *FILES);
void SaveToFile(FILE *src, FILE *dest, int filesize, byte *buff);
bool CreateSubDirectories(char *path);
bool EntryIsValid(byte *entry);
int GetSize(byte *Entry);
int GetLBA(byte *Entry);
int Get_WAD_Files_Num(FILE *wad, int *GROUP2, int *GROUPS_END);
int GetFilesize(FILE *wad);
byte GetMagicID(int lba, FILE *wad);
FILELIST *AllocLIST(int size);
byte *AllocMem();

