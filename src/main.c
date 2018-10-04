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

unsigned char fat_find(fat_partition_t *result, unsigned char max);

void main(void) {
    fat_partition_t fat_partition[10];
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
    os_line("getting fat32 systems");
    num = fat_find(fat_partition, sizeof(fat_partition)/sizeof(fat_partition_t));

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
