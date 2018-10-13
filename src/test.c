#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <string.h>
#include <ctype.h>
#include "fat.h"

FILE *fp;

#define WR_SIZE 0x400000
uint8_t buffer[512];

int write_sector_call(uint32_t sector, uint8_t *data) {
	fseek(fp, sector * 512, SEEK_SET);
	if (ftell(fp) != sector * 512)
		return -1;
	(void)fwrite(data, 512, 1, fp);
	if (ftell(fp) != (sector + 1) * 512)
		return -1;
	return 0;
}


int read_sector_call(uint32_t sector, uint8_t *data) {
	fseek(fp, sector * 512, SEEK_SET);
	if (ftell(fp) != sector * 512)
		return -1;
	(void)fread(data, 512, 1, fp);
	if (ftell(fp) != (sector + 1) * 512)
		return -1;
	return 0;
}

int main(int argc, char **argv) {
	int fd;
	uint8_t buff[512];
	size_t size;
	
	if(argc < 2) {
		fprintf(stderr, "Usage: test /path/to/image.img\n");
		return 1;
	}
	
	sector_buff = buff;
	fp = fopen(argv[1], "r+");
	init_fat();

	printf("open file...\n");
	fflush(stdout);

	create_file(NULL, "BADFILE", 0);

	fd = fat_open("BADFILE", O_WRONLY);
	if (fd >= 0) {

		printf("writing file...\n");
		fflush(stdout);

		for (size = 0; size < WR_SIZE; size += 512) {
			memcpy(sector_buff, buffer, 512);
			fat_write_sect(fd);
		}

		fat_close(fd);
		fat_set_fsize("BADFILE", WR_SIZE);
		printf("done.\n");
	}

	return 0;
}
