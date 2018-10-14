/*
Copyright (c) 2015 Steven Arnow <s@rdw.se>
'fat.c' - This file is part of µCFAT

This software is provided 'as-is', without any express or implied
warranty. In no event will the authors be held liable for any damages
arising from the use of this software.

Permission is granted to anyone to use this software for any purpose,
including commercial applications, and to alter it and redistribute it
freely, subject to the following restrictions:

	1. The origin of this software must not be misrepresented; you must not
	claim that you wrote the original software. If you use this software
	in a product, an acknowledgment in the product documentation would be
	appreciated but is not required.

	2. Altered source versions must be plainly marked as such, and must not be
	misrepresented as being the original software.

	3. This notice may not be removed or altered from any source
	distribution.
*/

#ifndef NULL
#define	NULL	((void *) 0)
#endif

#include <stdint.h>
#include <stdbool.h>
#include <stdio.h>
#include <string.h>
#include "fat.h"

#define GET32(p)		(*((uint32_t*)(p)))
#define GET16(p)		(*((uint16_t*)(p)))
#define FAT_DIRATTRIB		0x10
#define	MAX_FD_OPEN		4

#define SET16(p, w)		((*((uint16_t*)(p))) = w)
#define SET32(p, w)		((*((uint32_t*)(p))) = w)

#define GET_ENTRY_CLUSTER(e)	(((GET16(sector_buff + ((e) * 32 + 20)) << 16) | (GET16(sector_buff + ((e) * 32 + 26)))) & (fat_state.type != FAT_TYPE_FAT32 ? 0xFFFF : ~0))

static uint32_t locate_record(const char *path, unsigned int *record_index, const char *tail);

uint8_t *sector_buff;

enum FATType {
	FAT_TYPE_FAT16,
	FAT_TYPE_FAT32,
};


struct {
	bool		valid;
	uint8_t		cluster_size;
	uint16_t	root_directory_size;
	uint32_t	clusters;
	uint32_t	fat_size;
	uint32_t	fat_pos;
	uint32_t	root_dir_pos;
	uint32_t	data_region;
	uint8_t		type;
} fat_state;

struct FATFileDescriptor {
	int32_t		key;
	uint32_t	entry_sector;
	uint32_t	entry_index;
	uint32_t	first_cluster;
	uint32_t	current_cluster;
	uint32_t	fpos;
	uint32_t	file_size;
	bool		write;
};

struct FATFileDescriptor fat_fd[MAX_FD_OPEN];
int32_t fat_key = 0;

bool end_of_chain_mark(uint32_t cluster);/* {
	if (fat_state.type == FAT_TYPE_FAT16)
		return cluster >= 0xFFF8 ? true : false;
	return cluster >= 0x0FFFFFF8 ? true : false;
}
*/
bool init_fat();/* {
	uint8_t *data = sector_buff;
	uint16_t reserved_sectors;
	uint8_t *u8;
	uint8_t tu8;
	int err, i;
	uint32_t fsinfo;

	read_sector(0, data);

	if (GET16(data + 0xb) != 512) {
		//fprintf(stderr, "Only 512 bytes per sector is supported\n");
		return -1;
	}
	fat_state.cluster_size = data[13];
	reserved_sectors = GET16(data + 14);
	fat_state.root_directory_size = GET16(data + 17);

	u8 = &data[16];
	if (*u8 != 2) {
		//fprintf(stderr, "Unsupported FAT: %i FAT:s in filesystem, only 2 supported\n", *u8);
		return -1;
	}

	if (GET16(data + 19)) {
		fat_state.clusters = GET16(data + 19) / fat_state.cluster_size;
	} else {
		fat_state.clusters = GET32(data + 32) / fat_state.cluster_size;
	}

	if (fat_state.clusters < 4085) {
		return -1;
	} else if (fat_state.clusters < 65525) {
		fat_state.type = FAT_TYPE_FAT16;
	} else {
		fat_state.type = FAT_TYPE_FAT32;
	}

	if (fat_state.type <= FAT_TYPE_FAT16)
		tu8 = data[38];
	else
		tu8 = data[66];
	if (tu8 != 0x28 && tu8 != 0x29) {
	//	fprintf(stderr, "FAT signature check failed\n");
		return -1;
	}

	if (fat_state.type <= FAT_TYPE_FAT16) {
		fat_state.fat_size = GET16(data + 22);
	} else {
		fat_state.fat_size = GET32(data + 36);
	}
	
	fat_state.fat_pos = reserved_sectors;
	fat_state.root_dir_pos = fat_state.fat_pos + fat_state.fat_size * 2;
	fat_state.data_region = fat_state.root_dir_pos + GET16(data + 17) * 32 / 512;
	if (fat_state.type == FAT_TYPE_FAT32) {
		fat_state.data_region = fat_state.root_dir_pos;
		fat_state.root_dir_pos = fat_state.fat_pos + fat_state.fat_size * 2 + fat_state.cluster_size * (GET32(data + 44) - 2);

		/* Invalidate free space counter 
		fsinfo = GET16(data + 48);
		read_sector(fsinfo, data);
		if (GET32(data + 0) == 0x41615252 && GET16(data + 510) == 0xAA55) {
			SET32(data + 488, 0xFFFFFFFF);
			write_sector(fsinfo, data);
		}
	}


	for (i = 0; i < MAX_FD_OPEN; i++)
		fat_fd[i].key = -1;
	fat_state.valid = true;

	return 0;
}
*/

static void fname_to_fatname(char *name, char *fname) {
	uint8_t i, j;

	for (i = 0; i < 8 && name[i] != '.' && name[i]; i++)
		fname[i] = name[i];
	if (i < 8 && name[i]) {
		for (j = i; j < 8; j++)
			fname[j] = ' ';
		i++;
		for (; j < 11 && name[i]; j++, i++)
			fname[j] = name[i];
		for (; j < 11; j++)
			fname[j] = ' ';
	} else if (i == 8 && name[i] == '.') {
		i++, j = 8;
		for (; j < 11 && name[i]; j++, i++)
			fname[j] = name[i];
		for (; j < 11; j++)
			fname[j] = ' ';
	} else {
		for (; i < 11; i++)
			fname[i] = ' ';
	}

	return;
}


static uint32_t cluster_to_sector(uint32_t cluster) {
	if (!cluster)
		return 0;
	return (cluster - 2) * fat_state.cluster_size + fat_state.data_region;
}

static uint32_t sector_to_cluster(uint32_t sector) {
	if (!sector)
		return 0;
	return (sector - fat_state.data_region) / fat_state.cluster_size + 2;
}


static uint32_t next_cluster(uint32_t prev_cluster) {
	uint32_t cluster_pos, cluster_sec;

	if (fat_state.type == FAT_TYPE_FAT16) {
		cluster_pos = prev_cluster << 1;
		cluster_sec = cluster_pos >> 9;
		cluster_pos &= 0x1FF;
		cluster_sec += fat_state.fat_pos;
		read_sector(cluster_sec, sector_buff);
		cluster_sec = GET16(sector_buff + cluster_pos);
	} else {
		cluster_pos = prev_cluster << 2;
		cluster_sec = cluster_pos >> 9;
		cluster_pos &= 0x1FF;
		cluster_sec += fat_state.fat_pos;
		read_sector(cluster_sec, sector_buff);
		cluster_sec = GET32(sector_buff + cluster_pos) & 0x0FFFFFFF;
	}

	if (end_of_chain_mark(cluster_sec))
		return 0;

	return cluster_sec;
}


static void dealloc_fat(uint32_t fat_entry) {
	uint32_t sector, index;
	unsigned int shift_sec, shift_index;
	if (fat_state.type == FAT_TYPE_FAT16) {
		shift_sec = 8;
		shift_index = 1;
	} else {
		shift_sec = 7;
		shift_index = 2;
	}

	for (;;) {
		if (end_of_chain_mark(fat_entry) || !fat_entry)
			return;
		sector = (fat_entry >> shift_sec) + fat_state.fat_pos;
		read_sector(sector, sector_buff);
		valid_sector:

		index = (fat_entry << shift_index) & 0x1FF;
		fat_entry = GET16(sector_buff + index);
		if (shift_index == 1) {
			fat_entry = GET16(sector_buff + index);
			SET16(sector_buff + index, 0);
		} else {
			fat_entry = GET32(sector_buff + index);
			SET32(sector_buff + index, 0);
		}
		if (end_of_chain_mark(fat_entry) || ((fat_entry >> shift_sec) + fat_state.fat_pos != sector)) {
			write_sector(sector, sector_buff);
			write_sector(sector + fat_state.fat_size, sector_buff);
		} else
			goto valid_sector;
	}
}


static uint32_t locate_record(const char *path, unsigned int *record_index, const char *tail) {
	char component[13], fatname[11];
	unsigned int i = 0;
	uint32_t cur_sector;
	bool recurse = false, file = false;

	cur_sector = fat_state.root_dir_pos;

	found_component:
	while (*path == '/')
		path++;
	if (!*path) {
		if (tail) {
			path = tail;
			if (recurse)
				goto recurse;
		} else if (!recurse)
			return 0;
		else {
			*record_index = i;
			return cur_sector;
		}
	} else if (recurse) {
		recurse:
		cur_sector = GET_ENTRY_CLUSTER(i);
		cur_sector = cluster_to_sector(cur_sector);
	}
	memcpy(component, path, 13);
	for (i = 0; *path && *path != '/'; i++)
		path++;
	if (*path)
		path++;
	if (i < 13)
		component[i] = 0;
	if (file && *path)	/* A non-directory mid-path */
		return 0;
	fname_to_fatname(component, fatname);
	for (;;) {
		read_sector(cur_sector, sector_buff);

		for (i = 0; i < 16; i++) {
			if ((sector_buff[i * 32 + 11] & 0xF) == 0xF)	/* Long filename entry */
				continue;
			if (!sector_buff[i * 32])	/* End of list */
				return 0;
			if (sector_buff[i * 32] == 0xE5)
				/* Deleted record, skip */
				continue;
			/* This is a valid file */

			if (!memcmp(fatname, &sector_buff[i * 32], 11)) {
				recurse = true;
				file = (sector_buff[i * 32 + 11] & 0x10) ? false : true;
				goto found_component;
			}
		}
		
		if (!recurse) {	/* Don't cross cluster boundary for root directory */
			if (fat_state.type == FAT_TYPE_FAT16) {
				cur_sector++;	/* Lets just cross our fingers and hope we don't overrun */
				continue;
			}
		}

		if (cur_sector / fat_state.cluster_size != (cur_sector + 1) / fat_state.cluster_size) {
			cur_sector = cluster_to_sector(next_cluster(sector_to_cluster(cur_sector)));
			if (!cur_sector)
				return 0;
		} else
			cur_sector++;
	}
}


static uint32_t alloc_cluster(uint32_t entry_sector, uint32_t entry_index, uint32_t old_cluster) {
	uint32_t i, j;
	uint32_t cluster = 0, avail_cluster;
	int fat_size, ent_per_sec, eocm, mask, shift;
	
	if (fat_state.type == FAT_TYPE_FAT16) {
		fat_size = 2;
		ent_per_sec = 256;
		eocm = 0xFFFF;
		mask = 0xFF;
		shift = 1;
	} else {
		fat_size = 4;
		ent_per_sec = 128;
		eocm = 0xFFFFFFF;
		mask = 0x7F;
		shift = 2;
	}

	for (i = 0; i < fat_state.fat_size; i++) {
		read_sector(fat_state.fat_pos + i, sector_buff);

		for (j = 0; j < 512; j += fat_size) {
			avail_cluster = (fat_size == 2) ? GET16(sector_buff + j) : GET32(sector_buff + j);
			if (!avail_cluster) {
				cluster = i * ent_per_sec + j / fat_size;
				if (fat_size == 2)
					SET16(sector_buff + j, eocm);
				else
					SET32(sector_buff + j, eocm);
				write_sector(fat_state.fat_pos + i, sector_buff);
				write_sector(fat_state.fat_pos + fat_state.fat_size + i, sector_buff);
				if (old_cluster) {
					read_sector(fat_state.fat_pos + old_cluster / ent_per_sec, sector_buff);
					if (fat_size == 2)
						SET16(sector_buff + ((old_cluster & mask) << shift), cluster);
					else
						SET32(sector_buff + ((old_cluster & mask) << shift), cluster);
					write_sector(fat_state.fat_pos + old_cluster / ent_per_sec, sector_buff);
					write_sector(fat_state.fat_pos + fat_state.fat_size + old_cluster / ent_per_sec, sector_buff);
				}

				goto allocated;
			}
		}
	}
	
	allocated:
	if (!old_cluster) {
		read_sector(entry_sector, sector_buff);
		SET16(sector_buff + (entry_index * 32 + 20), cluster >> 16);
		SET16(sector_buff + (entry_index * 32 + 26), cluster);
		write_sector(entry_sector, sector_buff);
	}

	return cluster;
}


static uint32_t do_alloc_entry(uint32_t entry_sector, uint32_t entry_index, uint32_t prev_cluster) {
	uint32_t sector;
	sector = cluster_to_sector(alloc_cluster(entry_sector, entry_index, prev_cluster));
	sector_buff[0] = 0xE5;
	sector_buff[11] = 0;
	sector_buff[32] = 0;
	sector_buff[32 + 11] = 0;
	write_sector(sector, sector_buff);
	return sector;
}


static uint32_t alloc_entry(uint32_t parent_entry_sector, uint32_t parent_entry_index, uint32_t first_cluster, unsigned int *index) {
	unsigned int i, j;
	uint32_t sector, old_cluster, old_sector = 0;

	sector = cluster_to_sector(first_cluster);
	if (!sector) {
		*index = 0;
		return do_alloc_entry(parent_entry_sector, parent_entry_index, first_cluster);
	}

	for (;;) {
		for (i = 0; i < fat_state.cluster_size; i++) {
			read_sector(sector + i, sector_buff);
			for (j = 0; j < 16; j++) {
				if (old_sector) {
					sector_buff[j * 32 + 11] = 0;
					sector_buff[j * 32] = 0;
					write_sector(sector, sector_buff);
					return old_sector;
				}

				if (sector_buff[j * 32] == 0xE5) {
					*index = j;
					return sector;
				} else if (!sector_buff[j * 32]) {
					*index = j;
					old_sector = sector;
				}
			}
		}

		old_cluster = first_cluster;
		first_cluster = next_cluster(first_cluster);
		if (!first_cluster) {
			*index = 0;
			return do_alloc_entry(parent_entry_sector, parent_entry_index, old_cluster);
		}
	}
}


static uint32_t alloc_entry_root(unsigned int *index) {
	unsigned int i, j;
	uint32_t sector, old_sector = fat_state.root_dir_pos;
	bool alloc = false;

	if (fat_state.type == FAT_TYPE_FAT32) {
		return alloc_entry(0, 0, sector_to_cluster(old_sector), index);
	}

	sector = old_sector;
	for (i = 0; i < fat_state.root_directory_size / 16; i++) {
		read_sector(sector, sector_buff);

		for (j = 0; j < 16; j++) {
			if (!alloc) {
				if (sector_buff[j * 32] == 0xE5) {
					*index = j;
					return sector;
				} else if (!sector_buff[j * 32]) {
					*index = j;
					alloc = true;
					old_sector = sector;
				}
			} else {
				sector_buff[j * 32] = 0;
				sector_buff[j * 32 + 11] = 0;
				write_sector(sector, sector_buff);
				return old_sector;
			}
		}

		sector++;
	}

	return 0;
}


void fat_close(int fd) {
	int i;
	
	for (i = 0; i < MAX_FD_OPEN; i++)
		if (fat_fd[i].key == fd) {
			fat_fd[i].key = -1;
			/* TODO: Write access/modify/stuff */
			return;
		}
}


int fat_open(const char *path, int flags) {
	unsigned int i, index;
	uint32_t sector;

	if (!fat_state.valid)
		return -1;

	for (i = 0; i < MAX_FD_OPEN; i++)
		if (fat_fd[i].key < 0)
			break;
	if (i == MAX_FD_OPEN)
		return -1;
	
	if (!(sector = locate_record(path, &index, NULL)))
		return -1;
	read_sector(sector, sector_buff);

	if (sector_buff[index * 32 + 11] & 0x10)
		/* Don't allow opening a directory */
		return -1;
	fat_fd[i].write = flags & O_WRONLY ? true : false;
	fat_fd[i].entry_sector = sector;
	fat_fd[i].entry_index = index;
	fat_fd[i].first_cluster = GET_ENTRY_CLUSTER(index);
	if (fat_fd[i].first_cluster == 0) {
		if (fat_fd[i].write)
			fat_fd[i].first_cluster = alloc_cluster(fat_fd[i].entry_sector, fat_fd[i].entry_index, 0);
		fat_fd[i].file_size = 0;
	} else {
		fat_fd[i].file_size = GET32(sector_buff + (index * 32 + 28));
	}

	fat_fd[i].current_cluster = fat_fd[i].first_cluster;
	fat_fd[i].fpos = 0;
	fat_fd[i].key = fat_key++;
	return fat_fd[i].key;
}


uint32_t fat_fsize(int fd) {
	int i;
	
	if (fd < 0)
		return 0;
	for (i = 0; i < MAX_FD_OPEN; i++)
		if (fat_fd[i].key == fd)
			break;
	if (i == MAX_FD_OPEN)
		return 0;
	return fat_fd[i].file_size;
}


uint32_t fat_ftell(int fd) {
	int i;

	if (fd < 0)
		return 0;
	for (i = 0; i < MAX_FD_OPEN; i++)
		if (fat_fd[i].key == fd)
			break;
	if (i == MAX_FD_OPEN)
		return 0;
	return fat_fd[i].fpos;
}


bool fat_read_sect(int fd) {
	unsigned int i;
	uint32_t old_cluster, sector;
	struct FATFileDescriptor *desc;

	if (fd < 0)
		return false;
	for (i = 0; i < MAX_FD_OPEN; i++)
		if (fat_fd[i].key == fd)
			break;
        desc = &fat_fd[i];
	if (!desc->current_cluster)
		return false;
	if (desc->fpos == desc->file_size)
		return false;
	sector = cluster_to_sector(desc->current_cluster) + ((desc->fpos >> 9) % fat_state.cluster_size);
	
	desc->fpos += 512;
	if (desc->file_size < desc->fpos)
		desc->fpos = desc->file_size & (~0x1FF);
	if (!(desc->fpos % (fat_state.cluster_size * 512))) {
		old_cluster = desc->current_cluster;
		desc->current_cluster = next_cluster(desc->current_cluster);
		if (!desc->current_cluster && desc->write)
			desc->current_cluster = alloc_cluster(desc->entry_sector, desc->entry_index, old_cluster);
	}
	
	read_sector(sector, sector_buff);

	return true;
}


bool fat_write_sect(int fd) {
	unsigned int i;
	uint32_t old_cluster, sector;
	struct FATFileDescriptor *desc;

	if (fd < 0)
		return false;
	for (i = 0; i < MAX_FD_OPEN; i++)
		if (fat_fd[i].key == fd)
			break;
        desc = &fat_fd[i];
	if (!desc->write)
		return false;
	if (!desc->current_cluster)
		return false;
	sector = cluster_to_sector(desc->current_cluster);
	sector += (desc->fpos >> 9) % fat_state.cluster_size;
	write_sector(sector, sector_buff);
	desc->fpos += 512;
	if (!((desc->fpos >> 9) % fat_state.cluster_size)) {
		old_cluster = desc->current_cluster;
		desc->current_cluster = next_cluster(desc->current_cluster);
		if (!desc->current_cluster && desc->write)
			desc->current_cluster = alloc_cluster(desc->entry_sector, desc->entry_index, old_cluster);
	}

	read_sector(desc->entry_sector, sector_buff);
	SET32(sector_buff + (desc->entry_index * 32 + 28), desc->fpos);
	write_sector(desc->entry_sector, sector_buff);
	return true;
}


/* Anta att vi aldrig försöker ta bort roten */
static bool folder_empty(uint32_t cluster) {
	unsigned int i, j;
	uint32_t sector;

	for (;;) {
		sector = cluster_to_sector(cluster);
		if (!sector)
			return true;
		for (i = 0; i < fat_state.cluster_size; i++) {
			read_sector(sector, sector_buff);
			for (j = 0; j < 16; j++) {
                                unsigned int j32 = j * 32;
                                uint8_t val;
				if (sector_buff[j32 + 11] == 0xF)
					continue;
                                val = sector_buff[j32];
				if (val == 0)
					return true;
				if (val != 0xE5 && val && val != '.' && val != ' ')
					return false;
			}
			sector++;
		}
		cluster = next_cluster(cluster);
	}

	return true;
}


bool delete_file(const char *path) {
	uint32_t i;
	unsigned int index, index32;
	uint32_t sector, cluster;
	if (!fat_state.valid)
		return false;
	sector = locate_record(path, &index, NULL);
	if (!sector)
		return false;
	for (i = 0; i < MAX_FD_OPEN; i++)
		if (fat_fd[i].entry_sector == sector && (int) fat_fd[i].entry_index == index && fat_fd[i].key >= 0)
			return false;
	read_sector(sector, sector_buff);
	index32 = index * 32;
	if (sector_buff[index32 + 11] & 0x10) {
		cluster = GET_ENTRY_CLUSTER(index);
		if (!folder_empty(cluster))
			return false;
		read_sector(sector, sector_buff);
		sector_buff[index32 + 11] = 0;
		sector_buff[index32] = 0xE5;
		write_sector(sector, sector_buff);
		dealloc_fat(cluster);
		return true;
	} else {
		sector_buff[index32 + 11] = 0;
		sector_buff[index32] = 0xE5;
		cluster = GET_ENTRY_CLUSTER(index);
		write_sector(sector, sector_buff);
		dealloc_fat(cluster);
		return true;
	}
}


bool create_file(char *path, char *name, uint8_t attrib) {
	uint32_t sector, pcluster, cluster;
	unsigned int index, pindex, index32, i;
	if (!path) {
		if (locate_record(name, &i, NULL))
			return false;
	} else {
		if (locate_record(path, &i, name))
			return false;
	}

	if (path && *path == '/' && !*(path + 1))
		path = NULL;
	if (!path) {
		sector = alloc_entry_root(&index);
		pcluster = 0;
	} else {
		sector = locate_record(path, &pindex, NULL);
		read_sector(sector, sector_buff);
		pcluster = GET_ENTRY_CLUSTER(pindex);
		if (!sector)
			return false;
		read_sector(sector, sector_buff);
		sector = alloc_entry(sector, pindex, GET_ENTRY_CLUSTER(pindex), &index);
	}

	if (!sector)
		return false;
	read_sector(sector, sector_buff);
	index32 = index * 32;
	for (i = 0; i < 32; i++)
		sector_buff[index32 + i] = 0;
	fname_to_fatname(name, (char *) &sector_buff[index32]);
	sector_buff[index32 + 11] = attrib;
	write_sector(sector, sector_buff);
	
	if (attrib & 0x10) {
		pindex = index;
		sector = alloc_entry(sector, pindex, 0, &index);
		read_sector(sector, sector_buff);
                index32 = index * 32;
		for (i = 0; i < 11; i++)
			sector_buff[index32 + i] = ' ';
		sector_buff[index32] = '.';
		cluster = sector_to_cluster(sector);
		SET16(sector_buff + (index32 + 20), cluster >> 16);
		SET16(sector_buff + (index32 + 26), cluster & 0xFFFF);
		sector_buff[index32 + 11] = 0x10;
		write_sector(sector, sector_buff);
		
		sector = alloc_entry(sector, pindex, cluster, &index);
		read_sector(sector, sector_buff);
                index32 = index * 32;
		for (i = 0; i < 11; i++)
			sector_buff[index32 + i] = ' ';
		sector_buff[index32] = '.';
		sector_buff[index32 + 1] = '.';
		SET16(sector_buff + (index32 + 20), pcluster >> 16);
		SET16(sector_buff + (index32 + 26), pcluster & 0xFFFF);
		sector_buff[index32 + 11] = 0x10;
		write_sector(sector, sector_buff);
	}

	return true;
}


uint8_t fat_get_stat(const char *path) {
	uint32_t sector;
	unsigned int index;
	if (!(sector = locate_record(path, &index, NULL)))
		return 0xFF;
	read_sector(sector, sector_buff);
	return sector_buff[(index << 5) + 11];
}


void fat_set_stat(const char *path, uint8_t stat) {
	uint32_t sector;
	unsigned int index;
	if (!(sector = locate_record(path, &index, NULL)))
		return;
	read_sector(sector, sector_buff);
	sector_buff[(index << 5) + 11] = stat;
	write_sector(sector, sector_buff);
	return;
}


void fat_set_fsize(const char *path, uint32_t size) {
	uint32_t sector;
	unsigned int index;
	if (!(sector = locate_record(path, &index, NULL)))
		return;
	read_sector(sector, sector_buff);
	SET32(sector_buff + (index * 32 + 28), size);
	write_sector(sector, sector_buff);
	return;
}


int fat_dirlist(const char *path, struct FATDirList *list, int size, int skip) {
	uint32_t sector, cluster, found;
	unsigned int index, i, j, k, l;
	bool root = false;

	if (!path);
	else {
		while (*path == '/') path++;
		if (!*path)
			path = 0;
	}
	if (!path) {
		sector = fat_state.root_dir_pos;
		root = true;
	} else {
		if (!(sector = locate_record(path, &index, NULL)))
			return -1;
		read_sector(sector, sector_buff);
		if (!(sector_buff[index * 32 + 11] & 0x10))
			return -1;
		cluster = GET_ENTRY_CLUSTER(index);
		if (fat_state.type != FAT_TYPE_FAT32)
			cluster &= 0xFFFF;
		sector = cluster_to_sector(cluster);
	}

	found = 0;
	for (;;) {
		cluster = sector_to_cluster(sector);
		for (i = 0; i < fat_state.cluster_size; i++) {
			read_sector(sector + i, sector_buff);
			for (j = 0; j < 16; j++) {
				if (found >= (uint32_t) size)
					return found;
				if (sector_buff[j * 32 + 11] == 0xF)
					continue;
				if (sector_buff[j * 32] == 0xE5)
					continue;
				if (!sector_buff[j * 32])
					return found;
				if (skip) {
					skip--;
					continue;
				}

				for (k = 0; k < 8; k++) {
					if (sector_buff[j * 32 + k] == ' ')
						break;
					list[found].filename[k] = sector_buff[j * 32 + k];
				}
				
				if (sector_buff[j * 32 + 8] != ' ') {
					list[found].filename[k] = '.';
					k++;
				}

				for (l = 0; l < 3; k++, l++) {
					if (sector_buff[j * 32 + 8 + l] == ' ')
						break;
					list[found].filename[k] = sector_buff[j * 32 + 8 + l];
				}
				
				list[found].filename[k] = 0;

				list[found].attrib = sector_buff[j * 32 + 11];
				found++;
			}
			
		}
		
		if (root && fat_state.type == FAT_TYPE_FAT16) {
			sector += fat_state.cluster_size;
		} else {
			if (!(cluster = next_cluster(cluster)))
				return found;
			sector = cluster_to_sector(cluster);	
		}
	}
}


bool fat_valid() {
	return fat_state.valid;
}
