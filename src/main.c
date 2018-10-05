#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>
#include <tice.h>
#include <keypadc.h>
#include "thinfat32.h"

unsigned char msd_Init(void);
void usb_Cleanup(void);
void msd_KeepAlive(void);

void key_Scan(void);
unsigned char key_Any(void);

void os_line(const char *str);
void wait_user(void);

unsigned char sector[512];

unsigned char msd_ReadSector(uint8_t *data, uint32_t blocknum);
unsigned char msd_WriteSector(uint8_t *data, uint32_t blocknum);

typedef struct {
	uint32_t lba;
	uint32_t size;
} fat_partition_t;

unsigned char fat_Find(fat_partition_t *result, unsigned char max);
void fat_Select(fat_partition_t *partitions, unsigned char index);

uint32_t chstolba(uint32_t c)
{
// LBA = (C × HPC + H) × SPT + (S - 1)
// C, H and S are the cylinder number, the head number, and the sector number
// LBA is the logical block address
// HPC is the maximum number of heads per cylinder (reported by disk drive, typically 16 for 28-bit LBA)
// SPT is the maximum number of sectors per track (reported by disk drive, typically 63 for 28-bit LBA)

	return c;
}

#define MAX_PARTITIONS 10

void main(void) {
    fat_partition_t fat_partitions[MAX_PARTITIONS];
    char buffer[100];
    const char *file = "apples.txt";
    int err;
    uint8_t num;

    os_ClrHome();

    os_line("insert msd...");

    /* initialize mass storage device */
    if (!msd_Init()) {
	return;
    }

    /* find avaliable fat32 filesystems */
    num = fat_Find(fat_partitions, MAX_PARTITIONS);
    fat_Select(fat_partitions, 0);

    sprintf(buffer, "num: %u", num);
    os_line(buffer);

    usb_Cleanup();

    /* wait for key */
    while(!os_GetCSC());
}

/* cannot use getcsc in usb */
void wait_user(void) {
    while(!key_Any()) {
        msd_KeepAlive();
    }
}

void os_line(const char *str) {
    os_PutStrLine(str);
    os_NewLine();
}
