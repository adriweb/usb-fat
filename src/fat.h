/**
 * @file
 * @brief Contains USB FAT and MSD related routines
 *
 *
 * @authors Matt "MateoConLechuga" Waltz
 * @authors Jacob "jacobly" Young
 */

#ifndef H_FAT_MSD
#define H_FAT_MSD

#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

void os_line(const char *str);
void send_char(char n);

typedef struct {
    uint32_t lba;
    uint32_t size;
} fat_partition_t;

uint8_t msd_Init(void);
void msd_KeepAlive(void);
uint8_t msd_ReadSector(uint8_t *data, uint32_t blocknum);
uint8_t msd_WriteSector(uint8_t *data, uint32_t blocknum);

uint8_t fat_Init(void);
uint8_t fat_ReadSector(uint8_t *data, uint32_t blocknum);
uint8_t fat_WriteSector(uint8_t *data, uint32_t blocknum);
uint8_t fat_ValidSector(uint8_t *data);

uint8_t fat_Find(fat_partition_t *result, uint8_t max);
void fat_Select(fat_partition_t *partitions, uint8_t index);

void usb_Cleanup(void);

struct fat32fs_t
{
	uint32_t fat_begin_lba;
	uint32_t cluster_begin_lba;
	uint32_t root_dir_first_cluster;
	uint32_t sectors_per_fat;
	uint8_t sectors_per_cluster;
};

enum {DIRENT_FILE, DIRENT_EXTFILENAME, DIRENT_VOLLABEL, DIRENT_BLANK, DIRENT_END};

struct fat32dirent_t
{
	char filename[12];
	uint8_t attrib;
	uint32_t cluster;
	uint32_t size;
	uint32_t dcluster;

	char type;
};

struct fatwrite_t
{
	int sect_i;
	int sector_offset;
	uint32_t size;
	uint32_t f_cluster;
	uint32_t cur_cluster;
	uint32_t dir;
	uint8_t buf[512];
	char name[11];
};

/* the user level functions */

void ls(void);
void cd(const char * s);
void rn(const char * s, const char * snew);
void del(const char * s);
void cat(const char * s);
char exists(const char * s);
void touch(const char * s);
char mkdir(const char * dirname);
int dir_highestnumbered(void);

/* routines for writing to files */

char write_start(const char * s, struct fatwrite_t * fwrite);
void write_add(struct fatwrite_t * fwrite, const char * buf, int count);
void write_end(struct fatwrite_t * fwrite);
char write_append(const char * s, struct fatwrite_t * fwrite);

/* the workhorse functions */

#define IS_SUBDIR(dirent) (((dirent).attrib & 0x10) && ((dirent).type == DIRENT_FILE))
#define IS_FILE(dirent) ((dirent).type == DIRENT_FILE)

const char* str_to_fat(const char * str);
void loop_dir(uint32_t fcluster, char (*funct)(struct fat32dirent_t*));
void loop_file(uint32_t fcluster, int size, void (*funct)(uint8_t *, int));
uint32_t fat_findempty(void);
uint32_t fat_readnext(uint32_t cur_cluster);
uint32_t fat_writenext(uint32_t cur_cluster, uint32_t new_cluster);
void fat_clearchain(uint32_t first_cluster);
char print_dirent(struct fat32dirent_t* de);
void print_sect(uint8_t* s, int n);
char find_dirent(struct fat32dirent_t* de);
char find_emptyslot(struct fat32dirent_t* de);

#ifdef __cplusplus
}
#endif

#endif
