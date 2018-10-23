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
#include <stdbool.h>

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
uint8_t msd_ReadSector(uint8_t *data, uint32_t sector);
uint8_t msd_WriteSector(uint8_t *data, uint32_t sector);

uint8_t fat_Init(void);
uint8_t fat_ReadSector(uint8_t *data, uint32_t sector);
uint8_t fat_WriteSector(uint8_t *data, uint32_t sector);
uint8_t fat_ValidSector(uint8_t *data);

uint8_t fat_Find(fat_partition_t *result, uint8_t max);
void fat_Select(fat_partition_t *partitions, uint8_t index);

void usb_Cleanup(void);

#define	O_WRONLY		2
#define	O_RDONLY		1
#define	O_RDWR			(O_RDONLY | O_WRONLY)
extern uint8_t *sector_buff;

struct FATDirList {
	char		filename[13];
	uint8_t		attrib;
	uint8_t		padding[2];
};

bool init_fat(void);
void fat_close(int fd);
int8_t fat_open(const char *path, int flags);
uint32_t fat_fsize(int fd);
uint32_t fat_ftell(int fd);
bool fat_read_sect(int fd);
bool fat_write_sect(int fd);
bool delete_file(const char *path);
bool create_file(char *path, char *name, uint8_t attrib);
uint8_t fat_get_stat(const char *path);
void fat_set_stat(const char *path, uint8_t stat);
void fat_set_fsize(const char *path, uint32_t size);
int fat_dirlist(const char *path, struct FATDirList *list, int size, int skip);

/* These should be provided by you */
/* Returns < 0 on error */
#define write_sector(sector, data) fat_WriteSector(data, sector)
#define read_sector(sector, data) fat_ReadSector(data, sector)
//int write_sector_call(uint32_t sector, uint8_t *data);
//int read_sector_call(uint32_t sector, uint8_t *data);

#endif
