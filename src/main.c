#include "fat.h"

#include <stdio.h>
#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>
#include <tice.h>
#include <keypadc.h>

#define MAX_PARTITIONS 10

void send_char(char n)
{
    char str[2];
    str[0] = n;
    str[1] = 0;
    os_PutStrFull(str);
}

void key_Scan(void);
unsigned char key_Any(void);

/* cannot use getcsc in usb */
void wait_user(void) {
    while(!key_Any()) {
        msd_KeepAlive();
    }
}

void os_line(const char *str) {
    os_PutStrFull(str);
    os_NewLine();
}

void open_fat_file(void) {
    unsigned int size;
    uint8_t num;
    fat_partition_t fat_partitions[MAX_PARTITIONS];
    char buffer[256];

    os_ClrHome();

    os_line("insert msd...");

    /* initialize mass storage device */
    if (!msd_Init()) {
        os_line("msd init failed.");
        return;
    }

    /* find avaliable fat32 filesystems */
    num = fat_Find(fat_partitions, MAX_PARTITIONS);
    if (num == 0) {
        os_line("no fat partitions.");
        return;
    }

    /* log number of partitions */
    sprintf(buffer, "total fat partitions: %u", num);
    os_line(buffer);

    /* just use the first partition */
    fat_Select(fat_partitions, 0);

    os_line("using fat partition 1.");

    if (fat_Init() != 0) {
        os_line("invalid fat partition.");
        return;
    }

    cat("FILETEST.TXT");

    /* attempt to delete it */
    del("FILETEST.TXT");
}

void main(void) {
    open_fat_file();
    usb_Cleanup();

    /* wait for key */
    while(!os_GetCSC());
}

