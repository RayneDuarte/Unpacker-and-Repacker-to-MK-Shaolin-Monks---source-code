#include "gamedata_reader.h"
static const char EWDF_MAGIC[4]={"EWDF"};
#define MAX_SIZE_WITHOUT_ENC 0xffffff //~16MB 16777215
#define MAX_SIZE_WITH_ENC 0xFFFFF //~1MB 1048575
#define MAX_LBA_WITH_ENC 0xFFFFF
#define ARCHIVE_TABLE_SIZE 0xE000
#define LBA_IS_TOO_LARGE -1

void BuildGAMEDATA(FILE *wad);
void CloseResources(FILELIST *FILES, unsigned char *table, unsigned char *buff, FILE *log, FILE *newpack);
void UpdateTOC(FILELIST *FILES, unsigned char *table, int GROUPS_END);
bool Is_Encoded(FILE *f);
void Padding(FILE *newpack);
void Packbits(unsigned char *e, int v);
void Writebits(unsigned char *e, int v);
