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

#define WR_SIZE 0x400000

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
    unsigned int size;
    uint8_t num;
    fat_partition_t fat_partitions[MAX_PARTITIONS];
    char buf[256];
    int fd;

    sector_buff = sect;

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
    sprintf(buf, "total fat partitions: %u", num);
    os_line(buf);

    /* just use the first partition */
    fat_Select(fat_partitions, 0);

    os_line("using fat partition 1.");

    if (init_fat() != 0) {
        os_line("invalid fat partition.");
        return;
    }
    
    delete_file(wrtest);
    create_file(0, wrtest, 0);

    fd = fat_open(wrtest, O_WRONLY);
    if (fd >= 0) {

	os_line("writing file...");

	for (size = 0; size < WR_SIZE; size += 512) {
		memcpy(sector_buff, (void*)size, 512);
		fat_write_sect(fd);
		if( !(size % (512 * 200)) )
		{
			sprintf(buf, "addr: %u", size);
			os_line(buf);
		}
	}
        fat_set_fsize(wrtest, WR_SIZE);

        fat_close(fd);
    }
}

void main(void) {
    open_fat_file();
    usb_Cleanup();

    /* wait for key */
    while(!os_GetCSC());
}

