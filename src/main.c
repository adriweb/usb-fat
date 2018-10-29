#include "fat.h"

#include <stdio.h>
#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>
#include <tice.h>
#include <keypadc.h>
#include <string.h>

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

const char *rdtest = "RDTEST.TXT";
const char *wrtest = "WRTEST.TXT";

#define WR_SIZE 8192

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
    uint8_t sect[512];
    unsigned int size, ticks;
    uint8_t num;
    fat_partition_t fat_partitions[MAX_PARTITIONS];
    char buf[256];
    int8_t fd;

    sector_buff = sect;

    os_ClrHome();

    os_line("insert msd...");

    asm("ld iy,D00080h");
    asm("res 1,(iy + 0Dh)"); // no text buffer
    asm("res 3,(iy + 4Ah)"); // use first buffer (negated by next)
    asm("set 5,(iy + 4Ch)"); // only display

    /* initialize mass storage device */
    if (!msd_Init()) {
        os_line("msd init failed.");
        return;
    }

    os_line("locating filesystem");

    /* find avaliable fat32 filesystems */
    num = fat_Find(fat_partitions, MAX_PARTITIONS);
    if (num == 0) {
        os_line("no fat partitions.");
        return;
    }

    /* log number of partitions */
    sprintf(buf, "total fat partitions: %u", num);
    os_line(buf);

    /* just use the first partition */
    fat_Select(fat_partitions, 0);

    os_line("using fat partition 1.");

    if (init_fat() != true) {
        os_line("invalid fat partition.");
        return;
    }

    delete_file(wrtest);
    create_file(0, wrtest, 0);

    fd = fat_open(rdtest, O_RDONLY);
    if (fd >= 0) {

        os_line("reading file...");

        timer_Control = TIMER1_DISABLE;
        timer_1_ReloadValue = timer_1_Counter = 0;
        timer_Control = TIMER1_ENABLE | TIMER1_32K | TIMER1_UP;

        for (size = 0; size < WR_SIZE; size += 512) {
            fat_read_sect(fd);
        }

        ticks = (unsigned int)timer_1_Counter;
        sprintf(buf, "ticks: %u", ticks);
        os_line(buf);
    }

    fd = fat_open(wrtest, O_WRONLY);
    if (fd >= 0) {

        os_line("writing file...");

        timer_Control = TIMER1_DISABLE;
        timer_1_ReloadValue = timer_1_Counter = 0;
        timer_Control = TIMER1_ENABLE | TIMER1_32K | TIMER1_UP;

        for (size = 0; size < WR_SIZE; size += 512) {
            fat_write_sect(fd);
        }

        ticks = (unsigned int)timer_1_Counter;
        sprintf(buf, "ticks: %u", ticks);
        os_line(buf);

        fat_close(fd);
        fat_set_fsize(wrtest, WR_SIZE);
    }

    fd = fat_open(wrtest, O_RDONLY);
    if (fd >= 0) {
        sprintf(buf, "fat_fsize: %u", (unsigned int)fat_fsize(fd));
        os_line(buf);
        sprintf(buf, "fat_ftell: %u", (unsigned int)fat_ftell(fd));
        os_line(buf);
    }
}

void main(void) {
    open_fat_file();
    usb_Cleanup();

    /* wait for key */
    while(!os_GetCSC());
}

