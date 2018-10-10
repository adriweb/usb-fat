#include "fat.h"
#include <stdint.h>
#include <string.h>
#include <stdlib.h>
#include <ctype.h>

/* macros to aid in reading data from byte buffers */
#define GET32(p) (*((uint32_t*)(p)))
#define GET16(p) (*((uint16_t*)(p)))
#define FAT_EOF 0x0ffffff8
#define FAT_DIRATTRIB 0x10

/* the global fat32 fs structure */
static struct fat32fs_t fat;
#define CLUSTER(cn) ((fat.cluster_begin_lba + ((uint32_t)(((cn > 1) ? (cn) : fat.root_dir_first_cluster) & 0x0fffffff) - 2) * fat.sectors_per_cluster) & 0x0fffffff)
#define SECTOR(sn) (((sn) - fat.cluster_begin_lba) / fat.sectors_per_cluster + 2)
#define FIXCLUSTERNUM(cn) ((cn > 1) ? (cn) : fat.root_dir_first_cluster)

/* global directory entry data for parameter passing and holding the current directory */
static struct fat32dirent_t ret_file;
static struct fat32dirent_t cur_dir;
static const char* fncmp;
static int ret_value;
static uint32_t ret_lcluster;

/* shared sector buffers */
/* each routine is responsible for writing its own buffer modifications to memory */
/* these buffers are provided merely for convience and to save on memory usage */
static uint8_t sect[512];
static uint32_t cur_sect;
static uint8_t* cur_line;
static uint32_t fatsect[512 / sizeof(uint32_t)];
static uint32_t cur_fatsect;

#define readsector(lba, buffer) fat_ReadSector(buffer, lba)
#define writesector(lba, buffer) fat_WriteSector(buffer, lba)

/* string conversion function */
const char* str_to_fat(const char * str)
{
	static char name[12];
	unsigned char i, j;
	
	// copy first eight characters
	for (i=0; i<11; i++) {
		if ((i > 0 && str[i] == '.' && !(i == 1 && str[0] == '.')) || str[i] == '\0')
			break;
		name[i] = (isalnum(str[i]) || str[i] == '.' || str[i] == '~') ? toupper(str[i]) : '_';
	}
	
	// pad with spaces
	for (j=i; j<11; j++) name[j] = ' ';
	
	// copy extension
	if (str[i] == '.') {
		i++;
		for (j=0; j<3; j++) {
			if (str[i+j] == '\0') break;
			name[8+j] = isalnum(str[i+j]) ? toupper(str[i+j]) : '_';
		}
	}
	
	name[11] = '\0';
	
	return name;
}

/* finds an empty FAT cluster */
uint32_t fat_findempty(void)
{
	unsigned int i;

	for (cur_fatsect = fat.fat_begin_lba; cur_fatsect < fat.fat_begin_lba + fat.sectors_per_fat; cur_fatsect++) {
		readsector(cur_fatsect, (uint8_t*)fatsect);
		for (i=0; i<16; i++)
			if (!fatsect[i])
				return ((cur_fatsect - fat.fat_begin_lba) << 7) | i;
	}

	// hang
	//lcd_printf("filesystem\nis full");
	while (1) ;

	return 0x0fffffff; // out of space
}

/* reads the next cluster from the FAT */
uint32_t fat_readnext(uint32_t cur_cluster)
{
	// compute FAT sector and index
	uint32_t s;
	unsigned int i;

	cur_cluster = FIXCLUSTERNUM(cur_cluster);
    s = ((cur_cluster >> 7) + fat.fat_begin_lba);
    i = cur_cluster & 0x7f;

	// load next FAT sector into buffer
	if (cur_fatsect != s) {
		cur_fatsect = s;
		readsector(cur_fatsect, (uint8_t*)fatsect);
	}
	
	// return next cluster
	return fatsect[i];
}

/* writes the next cluster to the FAT */
/* uses a blocking write-through philosophy for the buffer, which can be inefficient */
uint32_t fat_writenext(uint32_t cur_cluster, uint32_t new_cluster)
{
	uint32_t r;

	// load the next FAT sector into buffer
	cur_cluster = FIXCLUSTERNUM(cur_cluster);
	r = fat_readnext(cur_cluster);

	// write FAT sectors (mirrored FATs)
	fatsect[cur_cluster & 0x7f] = new_cluster;
	writesector(cur_fatsect, (uint8_t*)fatsect);
	writesector(cur_fatsect + fat.sectors_per_fat, (uint8_t*)fatsect);

	// return next cluster
	return r;
}

/* clears a FAT cluster chain more efficiently than repeatedly calling fat_writenext() */
void fat_clearchain(uint32_t first_cluster)
{
	uint32_t cur_cluster;

	// compute FAT sector and index of the first cluster
	uint32_t s;
	unsigned int i;
	
    cur_cluster = first_cluster;
    cur_cluster = FIXCLUSTERNUM(cur_cluster);
    s = ((cur_cluster >> 7) + fat.fat_begin_lba);
    i = cur_cluster & 0x7f;

	while (cur_cluster != 0 && cur_cluster < FAT_EOF) {
		// load next FAT sector into buffer if not already there
		if (cur_fatsect != s) {
			cur_fatsect = s;
			readsector(cur_fatsect, (uint8_t*)fatsect);
		}
		
		// update FAT sector buffer
		cur_cluster = fatsect[i];
		fatsect[i] = 0;
	
		// compute FAT sector and index
		s = ((cur_cluster >> 7) + fat.fat_begin_lba);
		i = cur_cluster & 0x7f;

		// write FAT sector buffer to memory
		if (cur_fatsect != s || cur_cluster == 0 || cur_cluster >= FAT_EOF) {
			writesector(cur_fatsect, (uint8_t*)fatsect);
			writesector(cur_fatsect + fat.sectors_per_fat, (uint8_t*)fatsect);
		}
	}

}

/* fills a fat32dirent_t from raw data */
void load_dirent(struct fat32dirent_t * de, const uint8_t * d)
{
	unsigned char i;

	de->attrib = d[0xb];

	// set type
	switch (*d) {
		case 0xe5 : de->type = DIRENT_BLANK; break;
		case 0x00 : de->type = DIRENT_END; break;
		default :
			if ((de->attrib & 0xf) == 0xf) de->type = DIRENT_EXTFILENAME;
			else if (de->attrib & 0x8) de->type = DIRENT_VOLLABEL;
			else de->type = DIRENT_FILE;
			break;
	}
	
	if (de->type != DIRENT_FILE) return;
	
	// get filename
	for (i=0; i<11; i++) {
		de->filename[i] = d[i];
	}
	de->filename[11] = '\0';
	
	// get data
	de->cluster = ((uint32_t)GET16(d + 0x14) << 16) | GET16(d + 0x1a);
	de->size = GET32(d + 0x1c);
}

/* call fat_Select before running */
uint8_t fat_Init(void)
{
	uint8_t* p;
    uint8_t numfat;

	// read partition Volume ID
	if (readsector(0, sect)) {
		//os_line("error: sector read");
		return 1;	
	}
	
	// check bytes per sector (should be 512)
	if (GET16(sect + 0xb) != 512) {
		//os_line("error: non-512b sectors");
		return 4;
	}
	
    numfat = sect[0x10];

	// check for 2 FATs
	if (numfat != 2) {
		//os_line("warning: not mirrored");
	}
	
	// sanity check
	if (fat_ValidSector(sect) != 0) {
		//os_line("error: missing Vol ID");
		return 2;
	}
	
	// read and compute important constants
	fat.sectors_per_cluster = sect[0xd];
	fat.sectors_per_fat = GET32(sect + 0x24);
	
	// start plus #_of_reserved_sectors
	fat.fat_begin_lba = GET16(sect + 0xe);

	// start plus (#_of_FATs * sectors_per_FAT)
	fat.cluster_begin_lba = fat.fat_begin_lba + (numfat * fat.sectors_per_fat);
	
	// root directory first cluster
	fat.root_dir_first_cluster = GET32(sect + 0x2c);
	
	// read in the first sector of the first FAT cluster
	cur_fatsect = fat.fat_begin_lba;
	readsector(fat.fat_begin_lba, (uint8_t*)fatsect);
	
	// set current directory to root directory
	cur_dir.cluster = fat.root_dir_first_cluster;
	
	return 0;
}

/* dummy loop file routine */
void loop_file_all(uint8_t* s, int n)
{

}

void send_nstr(char *s, int n)
{
    int i;
    for (i = 0; i < n; i++) {
        send_char(s[i]);
    }
}

#if 1 // debugging
/* prints a text sector */
void print_sect(uint8_t* s, int n)
{
	if (n > 512) n = 512;
	send_nstr((char*)s, n);
}

/* prints a dirent */
char print_dirent(struct fat32dirent_t* de)
{
	if (de->type == DIRENT_FILE) {
		send_nstr(de->filename, strlen(de->filename));
		if (IS_SUBDIR(*de)) send_nstr(" (dir)", 6);
	}
	
	return 0;
}
#endif

/* finds a dirent by name */
char find_dirent(struct fat32dirent_t* de)
{
	unsigned char i;
	char cmp = 1;
	if (de->type == DIRENT_FILE) {
		// compare names
		for (i=0; i<11; i++) {
			if (de->filename[i] != fncmp[i]) {
				// not a match
				cmp = 0;
				break;
			}
		}
		
		// store if match
		if (cmp) return 1;
	}
	
	ret_file.type = DIRENT_BLANK;
	return 0;
}

/* finds an empty dirent slot */
char find_emptyslot(struct fat32dirent_t* de)
{
	if (de->type == DIRENT_BLANK || de->type == DIRENT_END) return 1;
	else return 0;
}

/* finds the highest numbered dirent */
char find_highest(struct fat32dirent_t* de)
{
    unsigned char i;
	int num = 0;
	if (de->filename[0] >= '0' && de->filename[0] <= '9') {
		for (i=0; i<8; i++) {
			if (de->filename[i] < '0' || de->filename[i] > '9') break;
			num = num * 10 + de->filename[i] - '0';
		}
		if (num > ret_value) ret_value = num;
	}
	return 0;
}

/* calls a subroutine for each dirent in the directory */
void loop_dir(uint32_t fcluster, char (*funct)(struct fat32dirent_t*))
{
	uint32_t cluster = fcluster;
	uint32_t fsect;
	uint8_t* p;
	struct fat32dirent_t d;

    fsect = cur_sect = CLUSTER(fcluster);
    p = sect;

	readsector(cur_sect, sect);
	
	// print all directory entries
	do {
		// load next sector, following cluster chains
		if (p - sect >= 512) {
			cur_sect++;
		
			// get next cluster from fat
			if (cur_sect - fsect >= fat.sectors_per_cluster) {
				cluster = fat_readnext(cluster);
				cur_sect = fsect = CLUSTER(cluster);
			}
			
			if (cluster >= FAT_EOF) break;
			
			// read new sector
			readsector(cur_sect, sect);
			p = sect;
		}
	
		// fill dirent struct
		load_dirent(&d, p);
		
		// call provided function with each entry
		if (funct(&d)) {
			load_dirent(&ret_file, p);
			ret_file.dcluster = cluster;
			cur_line = p;
			break;
		}
		
		p += 32;
	} while (d.type != DIRENT_END);
}

/* calls a subroutine for each sector in the file */
void loop_file(uint32_t fcluster, int size, void (*funct)(uint8_t *, int))
{
	uint32_t cluster;
	uint32_t fsect;
	uint32_t n;

    cluster = fcluster;
    fsect = cur_sect = CLUSTER(fcluster);
    n = size;
	ret_lcluster = cluster;
	
	// loop through all sectors
	while (1) {
		// load next sector, following cluster chains
	
		// get next cluster from fat
		if (cur_sect - fsect >= fat.sectors_per_cluster) {
			ret_lcluster = cluster;
			cluster = fat_readnext(cluster);
			cur_sect = fsect = CLUSTER(cluster);
		}
		
		if (cluster >= FAT_EOF) break;
		
		// read new sector
		readsector(cur_sect, sect);
		
		// call provided function with each sector
		funct(sect, n);
		
		n -= 512;
		cur_sect++;
	}
}



/* higher level fs routines */
/* they do about what you would expect of them */

#if 0 // debugging
void ls(void)
{
	loop_dir(cur_dir.cluster, print_dirent);
}
#endif

void cd(const char * s)
{
	fncmp = str_to_fat(s);
	loop_dir(cur_dir.cluster, find_dirent);
	
	if (IS_SUBDIR(ret_file))
		cur_dir.cluster = ret_file.cluster;
}

/* doesn't check whether the new name already exists */
/* if that would be a problem, call exists() first      */
void rn(const char * s, const char * snew)
{
	unsigned char i;
	const char* n;
	
	// find file
	fncmp = str_to_fat(s);
	loop_dir(cur_dir.cluster, find_dirent);
	
	n = str_to_fat(snew);
	
	if (n[0] == ' ') return;
	
	// modify buffered sector memory
	if (IS_FILE(ret_file)) {
		// insert spaces
		for (i=0; i<11; i++)
			cur_line[i] = *n++;
	} else return;
	
	// write modified sector buffer to card
	writesector(cur_sect, sect);
}


void del(const char * s)
{
	// find file
	fncmp = str_to_fat(s);
	loop_dir(cur_dir.cluster, find_dirent);
	
	if (!IS_FILE(ret_file) || IS_SUBDIR(ret_file)) return;

	// clear FAT chain
	if (ret_file.cluster != 0)
		fat_clearchain(ret_file.cluster);
	
	// erase directory entry
	*cur_line = 0xe5;
	writesector(cur_sect, sect);
}

#if 1 // debugging
void cat(const char * s)
{
	// find file
	fncmp = str_to_fat(s);
	loop_dir(cur_dir.cluster, find_dirent);
	
	// print if a printable file
	if (IS_FILE(ret_file) && !IS_SUBDIR(ret_file))
		loop_file(ret_file.cluster, ret_file.size, print_sect);
}
#endif

char exists(const char * s)
{
	fncmp = str_to_fat(s);

	loop_dir(cur_dir.cluster, find_dirent);
	
	return IS_FILE(ret_file);
}

/* doesn't check whether the file already exists */
void touch(const char * s)
{
	const char* n;
	uint32_t oldcluster, cluster, fsect;
	unsigned char i;

	// find the next open spot
    n = str_to_fat(s);
	loop_dir(cur_dir.cluster, find_emptyslot);
	cluster = ret_file.dcluster;
	fsect = CLUSTER(ret_file.dcluster);

	// write filename
	for (i=0; i<11; i++)
		cur_line[i] = *n++;
	
	// write filesize and attrib
	GET32(cur_line + 0x1c) = 0;
	cur_line[0x0b] = 0x00;
	
	// write EOF as first cluster
	GET16(cur_line + 0x14) = FAT_EOF>>16;
	GET16(cur_line + 0x1a) = 0xffff;

	// next dirent
	cur_line += 32;
	
	// check if adding to end of directory
	if (ret_file.type == DIRENT_END) {
		// check if end of sector
		if (cur_line - sect >= 512) {
			// write current sector
			writesector(cur_sect, sect);
			// go to new sector
			cur_sect++;

			// get next cluster from fat
			if (cur_sect - fsect >= fat.sectors_per_cluster) {
				// link to blank cluster
				oldcluster = cluster;
				cluster = fat_findempty();
				fat_writenext(oldcluster, cluster);
				fat_writenext(cluster, FAT_EOF);
			}
			
			// read new sector
			cur_sect = fsect = CLUSTER(cluster);
			readsector(cur_sect, sect);
			cur_line = sect;
		}

		// write end of dir
		cur_line[0] = 0x00;
	}

	// whatever just happened, the sector needs to be synced
	writesector(cur_sect, sect);

}

/* creates a directory in the current working directory */
/* returns 0 on success, true on failure*/
char mkdir(const char * dirname)
{
    uint32_t tmp_fat, bdir_cluster;

	// fail if the filename is in use, otherwise create it
	if (exists(dirname)) return -1;
	touch(dirname);

	fncmp = str_to_fat(dirname);
	loop_dir(cur_dir.cluster, find_dirent);
	if (!IS_FILE(ret_file)) return -2;

	// get an empty cluster
    tmp_fat = fat_findempty();
	fat_writenext(tmp_fat, FAT_EOF);
	
	// set directory bits and point to cluster
	GET16(cur_line + 0x14) = tmp_fat>>16;
	GET16(cur_line + 0x1a) = tmp_fat;
	cur_line[0x0b] = FAT_DIRATTRIB;
	writesector(cur_sect, sect);

	// remember the current directory for creating ".." and getting back
    bdir_cluster = cur_dir.cluster;

	// create "."
	cur_sect = CLUSTER(tmp_fat);
	sect[0] = 0x00;
	writesector(cur_sect, sect);
	
	cur_dir.cluster = tmp_fat;
	touch(".");
	cur_line = sect;

	// set directory bits and point to self
	GET16(cur_line + 0x14) = tmp_fat>>16;
	GET16(cur_line + 0x1a) = tmp_fat;
	cur_line[0x0b] = FAT_DIRATTRIB;
	
	writesector(cur_sect, sect);

	// create ".."
	touch("..");
	cur_line = sect + 32;
	
	// set directory bits and point to cluster
	GET16(cur_line + 0x14) = bdir_cluster>>16;
	GET16(cur_line + 0x1a) = bdir_cluster;
	cur_line[0x0b] = FAT_DIRATTRIB;
	
	cur_line = sect + 64;
	cur_line[0] = 0x00;

	writesector(cur_sect, sect);

	// return to original directory
	cur_dir.cluster = bdir_cluster;

	return 0;
}

int dir_highestnumbered(void)
{
	ret_value = 0;
	loop_dir(cur_dir.cluster, find_highest);
	return ret_value;
}

/* routines for writing to empty files created with touch() */

char write_start(const char * s, struct fatwrite_t * fwrite)
{
    unsigned char i;
	fncmp = str_to_fat(s);

	loop_dir(cur_dir.cluster, find_dirent);
	if (!IS_FILE(ret_file) || IS_SUBDIR(ret_file) || ret_file.size > 0)
		return 0;

	// save name
	for (i=0; i<11; i++)
		fwrite->name[i] = fncmp[i];

	// save important data
	fwrite->sect_i = 0;
	fwrite->sector_offset = 0;
	fwrite->size = 0;

	fwrite->cur_cluster = fwrite->f_cluster = fat_findempty();
	fwrite->dir = cur_dir.cluster;
	// lay claim to the cluster
	fat_writenext(fwrite->cur_cluster, FAT_EOF);
	
	return 1;
}

char write_append(const char * s, struct fatwrite_t * fwrite)
{
    unsigned char i;
	fncmp = str_to_fat(s);

	loop_dir(cur_dir.cluster, find_dirent);
	if (!IS_FILE(ret_file) || IS_SUBDIR(ret_file) || ret_file.size == 0)
		return 0;

	// save name
	for (i=0; i<11; i++)
		fwrite->name[i] = fncmp[i];

	// find end
	fwrite->f_cluster = ret_file.cluster;
	loop_file(ret_file.cluster, ret_file.size, loop_file_all);

	// save important data
	fwrite->sect_i = (ret_file.size%512) ? (ret_file.size%512) : 512;
	fwrite->sector_offset = ((ret_file.size-1)/512)%fat.sectors_per_cluster;
	fwrite->size = ret_file.size;

	fwrite->cur_cluster = ret_lcluster;
	fwrite->dir = cur_dir.cluster;

	// read in buffer
	readsector(CLUSTER(fwrite->cur_cluster) + fwrite->sector_offset, fwrite->buf);
	
	return 1;
}

void write_add(struct fatwrite_t * fwrite, const char * buf, int count)
{
	int i;
	uint32_t oldcluster;

	for (i=0; i<count; i++) {
		// filled sector, write out
		if (fwrite->sect_i >= 512) {
			//write_end(fwrite);
			writesector(CLUSTER(fwrite->cur_cluster) + fwrite->sector_offset, fwrite->buf);
			// new cluster
			fwrite->sector_offset++;
			if (fwrite->sector_offset >= fat.sectors_per_cluster) {
				// find new cluster
				fwrite->sector_offset = 0;
				oldcluster = fwrite->cur_cluster;
				fwrite->cur_cluster = fat_findempty();
				// link and reserve new cluster
				fat_writenext(oldcluster, fwrite->cur_cluster);
				fat_writenext(fwrite->cur_cluster, FAT_EOF);
			}

			// reset buffer index for the new sector
			fwrite->sect_i = 0;
		}
		
		// copy next byte to buffer
		fwrite->buf[fwrite->sect_i++] = buf[i];
		fwrite->size++;
	}
}

void write_end(struct fatwrite_t * fwrite)
{
	// write out current buffer and ensure fat chain terminates with an EOF
	writesector(CLUSTER(fwrite->cur_cluster) + fwrite->sector_offset, fwrite->buf);
	fat_writenext(fwrite->cur_cluster, FAT_EOF);
	
	// find file in dir
	fncmp = fwrite->name;
	loop_dir(fwrite->dir, find_dirent);

	// sanity check
	if (!IS_FILE(ret_file)) return;

	// write filesize
	GET32(cur_line + 0x1c) = fwrite->size;
	
	// write first cluster
	GET16(cur_line + 0x14) = fwrite->f_cluster >> 16;
	GET16(cur_line + 0x1a) = fwrite->f_cluster;

	// write out updated dir entry
	writesector(cur_sect, sect);
}
