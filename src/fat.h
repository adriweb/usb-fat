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

typedef struct {
    uint32_t lba;
    uint32_t size;
} fat_partition_t;

uint8_t msd_Init(void);
void msd_KeepAlive(void);
uint8_t msd_ReadSector(uint8_t *data, uint32_t blocknum);
uint8_t msd_WriteSector(uint8_t *data, uint32_t blocknum);

uint8_t fat_ReadSector(uint8_t *data, uint32_t blocknum);
uint8_t fat_WriteSector(uint8_t *data, uint32_t blocknum);

uint8_t fat_Find(fat_partition_t *result, uint8_t max);
void fat_Select(fat_partition_t *partitions, uint8_t index);

void usb_Cleanup(void);

#ifdef __cplusplus
}
#endif

#endif
