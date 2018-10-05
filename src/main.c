#include "fat.h"
#include "thinfat32.h"

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>
#include <tice.h>
#include <keypadc.h>

#define MAX_PARTITIONS 10

const char *file = "FILETEST.TXT";

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
    TFFile *fp;
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

    if (tf_init() != 0) {
        os_line("invalid fat partition.");
        return;
    }

    os_line("using fat partition 1.");

    if ((fp = tf_fopen(file, "r")) == NULL) {
        os_line("can't open file.");
        return;
    }

    size = fp->size;

    sprintf(buffer, "file size: %u bytes", size);
    os_line(buffer);

    if (size > sizeof buffer) {
        size = sizeof buffer;
    }

    tf_fread((uint8_t*)&buffer, size - 1, fp);
    buffer[size - 1] = '\0';
    os_line("file contents:");
    os_line(buffer);
    tf_fclose(fp);
}

void main(void) {
    open_fat_file();
    usb_Cleanup();

    /* wait for key */
    while(!os_GetCSC());
}

