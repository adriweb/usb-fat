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

const char *filename = "FILETEST.TXT";

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

    asm("ld iy,$d00080");
    asm("res 1,(iy + $0d)"); // no text buffer
    asm("res 3,(iy + $4a)"); // use first buffer (negated by next)
    asm("set 5,(iy + $4c)"); // only display

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

    /* attempt to delete it */
    if (exists(filename)) {
        char buf[10];
        unsigned int ticks;
        timer_Control = TIMER1_DISABLE;
        timer_1_ReloadValue = timer_1_Counter = 0;
        timer_Control = TIMER1_ENABLE | TIMER1_32K | TIMER1_UP;

        cat(filename);

        ticks = (unsigned int)timer_1_Counter;
        sprintf(buf, "ticks: %u", ticks);
        os_line(buf);
    }
}

void main(void) {
    open_fat_file();
    usb_Cleanup();

    /* wait for key */
    while(!os_GetCSC());
}

