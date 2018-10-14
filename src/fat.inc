_cluster_to_sector:
	pop	de
	pop	hl
	dec	sp
	pop	af
	dec	sp
	dec	sp
	push	hl
	push	de
	ld	de,-2
	add	hl,de
	adc	a,d
	ld	c,a
	ld	a,(_fat_state + 1)
	jr	c,enter
	sbc	hl,hl
	ld	e,l
	ret
loop:
	add	hl,hl
	rl	c
enter:
	rrca
	jr	nc,loop
	ld	de,(_fat_state + 20)
	ld	a,(_fat_state + 20 + 3)
	add	hl,de
	adc	a,c
	ld	e,a
	ret

_fname_to_fatname:
	ld	iy,0
	add	iy,sp
	ld	de,(iy + 3)	; de = name
	ld	hl,(iy + 6)	; hl = fname
	ld	b,0		; for (i = 0; i < 8 && name[i] != '.' && name[i]; i++)
.loop1:
	ld	a,b
	cp	a,8
	jr	nc,.done1
	ld	a,(de)
	cp	a,46		; '.'
	jr	z,.done1
	ld	a,(de)
	or	a,a
	jr	z,.done1
	ld	(hl),a
	inc	de		; i++
	inc	hl
	inc	b
	jr	.loop1
.done1:
	ld	a,b		; if (i < 8 && name[i])
	cp	a,8
	jr	nc,.elseif
	ld	a,(de)
	or	a,a
	jr	z,.elseif

	ld	a,8		; for (j = i; j < 8; j++)
.loop2:
	cp	a,b
	jr	z,.fillremaining
	ld	(hl),32		; fname[j] = ' ';
	inc	hl
	inc	b
	jr	.loop2
.fillremaining:
	inc	de		; i++;

.loop3456:			; for (; j < 11 && name[i]; j++, i++)
	ld	a,b
	cp	a,11
	ret	nc
	ld	a,(de)
	or	a,a
	jr	z,.other
	inc	de
.store:
	ld	(hl),a		; fname[j] = name[i];
	inc	hl
	inc	b
	jr	.loop3456
.other:
	ld	a,32		; ' '
	jr	.store

.elseif:
	ld	a,b		; else if (i == 8 && name[i] == '.')
	cp	a,8
	jr	nz,.spacefill
	ld	a,(de)
	cp	a,46		; '.'
	jr	nz,.spacefill
	jr	.fillremaining

.spacefill:
	ld	a,11
.spacefillloop:			; for (; j < 11; j++)
	cp	a,b
	ret	z
	ld	(hl),32		; fname[j] = ' '
	inc	hl
	inc	b
	jr	.spacefillloop

_next_cluster:
	ld	hl,3
	ld	e,h
	inc.s	bc
	ld	b,h
	add	hl,sp
	ld	a,(_fat_state + 1)
	or	a,a
	ld	a,(hl)
	inc	hl
	ld	hl,(hl)
	jr	z,.getshift
	add	a,a
	adc	hl,hl
	rl	e
.getshift:
	add	a,a
	ld	c,a
	rl	b
	push	bc
	ld	de,(_fat_state + 20)
	add	hl,de
	ld	a,(_fat_state + 20 + 3)
	add	a,e
	ld	e,a
	call	read_sector
	pop	de
	ld	hl,(_sector_buff)
	add	hl,de
	ld	e,(hl)
	inc	hl
	ld	d,(hl)
	ld	a,(_fat_state + 1)
	or	a,a
	jr	z,.getsec
	dec	hl
	ld	de,(hl)
	inc	hl
	inc	hl
	inc	hl
	ld	a,(hl)
	and	$0F
.getsec:
	ex	de,hl
	ld	e,a
	push	de
	push	hl
	call	_end_of_chain_mark
	pop	hl
	pop	de
	ret	c
	sbc	hl,hl
	ld	e,l
	ret



_end_of_chain_mark:
	pop	de
	pop	hl
	dec	sp
	pop	af
	dec	sp
	dec	sp
	push	hl
	push	de
	ld	de,8
	ld	a,(_fat_state + 24)
	or	a,a
	jr	nz,.fat32
	add.s	hl,de
	sbc	a,a
	ret
.fat32:
	add	hl,de
	adc	a,$f0
	sbc	a,a
	ret

_init_fat:
	or	a,a
	sbc	hl,hl
	ld	e,l
	call	read_sector		; read_sector(0, sector_buff);

	ld	iy,(_sector_buff)
	ld	hl,(iy + 11)
	ld	a,h
	sub	a,512 shr 8
	or	a,l			; if (get16(data + 11) != 512)
	jr	z,.goodsectsz
	xor	a,a			; return 0;
	ret

.goodsectsz:
	ld	a,(iy + 13)
	ld	(_fat_state + 1),a	; fat_state.cluster_size = data[13];

	ld	bc,(iy + 17)
	ld	hl,_fat_state + 2
	ld	(hl),c
	inc	hl
	ld	(hl),b			; fat_state.root_directory_size = get16(data + 17);

	ld	a,(iy + 16)
	cp	a,2			; if (data[16] != 2)
	jr	z,.goodfatnum
	xor	a,a			; return 0;
	ret

.goodfatnum:
	or	a,a			; if (get16(data + 19))
	sbc	hl,hl
	ld	e,l
	ld	l,(iy + 19)
	ld	h,(iy + 20)
	ld	a,(_fat_state + 1)
	ld	bc,0
	ld	c,a
	add	hl,de
	xor	a,a
	sbc	hl,de
	jr	z,.getclusters

	call	__idivs			; fat_state.clusters = get16(data + 19) / fat_state.cluster_size;
	xor	a,a
	ld	(_fat_state + 4),hl
	ld	e,a
	ld	(_fat_state + 7),a
	jr	.gotclusters
.getclusters:				; else
	ld	hl,(iy + 32)		; fat_state.clusters = get32(data + 32) / fat_state.cluster_size;
	ld	e,(iy + 35)
	call	__ldivu
	ld	(_fat_state + 4),hl
	ld	a,e
	ld	(_fat_state + 7),a
.gotclusters:				; if (fat_state.clusters < 4085)
	xor	a,a
	ld	bc,4085
	call	__lcmpu
	jr	nc,.fatvalid
	xor	a,a			; return 0;
	ret
.fatvalid:					; else if (fat_state.clusters < 65525)
	ld	a,(_fat_state + 7)
	ld	e,a
	ld	hl,(_fat_state + 4)
	xor	a,a
	ld	bc,65525
	call	__lcmpu
	jr	nc,.fat32
	xor	a,a			; fat_state.type = fat_type_fat16;
	jr	.fat16
.fat32:
	ld	a,1			; fat_state.type = fat_type_fat32;
.fat16:
	ld	(_fat_state + 24),a
	or	a,a			; if (fat_state.type <= fat_type_fat16)
	push	af
	jr	nz,.getu8
	ld	a,(iy + 38)		; tu8 = data[38];
	jr	.gotu8
.getu8:
	ld	a,(iy + 66)		; tu8 = data[66];
.gotu8:
	cp	a,$28			; if (tu8 != 0x28 && tu8 != 0x29)
	jr	z,.goodu8
	cp	a,$29
	jr	z,.goodu8
	pop	af
	xor	a,a			; return 0;
	ret
.goodu8:
	pop	af			; if (fat_state.type <= fat_type_fat16)
	jr	nz,l_18

	xor	a,a
	sbc	hl,hl
	ld	l,(iy + 22)
	ld	h,(iy + 23)
	ld	(_fat_state + 8),hl	; fat_state.fat_size = get16(data + 22);
	ld	(_fat_state + 11),a
	jr	l_20
l_18:
	ld	hl,(iy + 36)		; fat_state.fat_size = get32(data + 36);
	ld	a,(iy + 39)
	ld	(_fat_state + 8),hl
	ld	(_fat_state + 11),a
l_20:
	xor	a,a			; fat_state.fat_pos = reserved_sectors;
	sbc	hl,hl
	ld	l,(iy + 14)
	ld	h,(iy + 15)		; reserved_sectors = get16(data + 14);
	ld	(_fat_state + 12),hl
	ld	(_fat_state + 15),a

	ld	a,(_fat_state + 11)	; fat_state.root_dir_pos = fat_state.fat_pos + fat_state.fat_size * 2;
	ld	hl,(_fat_state + 8)
	ld	e,a
	xor	a,a
	ld	bc,2
	call	__lmulu
	ld	bc,(_fat_state + 12)
	ld	a,(_fat_state + 15)
	call	__ladd
	ld	a,e
	ld	(_fat_state + 16),hl
	ld	(_fat_state + 19),a	; fat_state.root_dir_pos

	ld	bc,0
	ld	a,(_fat_state + 2)
	ld	c,a
	ld	a,(_fat_state + 3)	; fat_state.root_directory_size
	ld	b,a

	srl	b			; fat_state.data_region = fat_state.root_dir_pos + fat_state.root_directory_size * 32 / 512;
	rr	c
	srl	b
	rr	c
	srl	b
	rr	c
	srl	b
	rr	c
	
	xor	a,a
	add	hl,bc			; __ladd
	adc	a,e
	ld	e,a
	ld	(_fat_state + 20),hl
	ld	a,e
	ld	(_fat_state + 23),a

	ld	a,(_fat_state + 24)	; if (fat_state.type == fat_type_fat32)
	cp	a,1
	jr	nz,l_28
	ld	bc,(_fat_state + 16)	; fat_state.data_region = fat_state.root_dir_pos;
	ld	a,(_fat_state + 19)
	ld	(_fat_state + 20),bc
	ld	(_fat_state + 23),a

	ld	hl,(iy + 44)		; fat_state.root_dir_pos += + fat_state.cluster_size * (get32(data + 44) - 2);
	ld	e,(iy + 47)
	xor	a,a
	ld	bc,2
	call	__lsub
	ld	bc,0
	ld	a,(_fat_state + 1)
	ld	c,a
	call	__lmulu
	ld	bc,(_fat_state + 16)
	ld	a,(_fat_state + 19)
	call	__ladd
	ld	(_fat_state + 16),hl
	ld	a,e
	ld	(_fat_state + 19),a

	or	a,a			; fsinfo = get16(data + 48);
	sbc	hl,hl
	ld	e,l
	ld	l,(iy + 48)
	ld	h,(iy + 49)		; /* invalidate free space counter */
	ld	(.fsinfo),hl
	call	read_sector		; read_sector(fsinfo, data);

	ld	hl,(iy + 0)		; if (get32(data + 0) == 0x41615252 && get16(data + 510) == 0xaa55)
	ld	e,(iy + 3)
	ld	bc,$615252
	ld	a,$41
	call	__lcmpu
	jr	nz,l_28
	ld	bc,510
	lea	hl,iy
	add	hl,bc
	call	test_sector
	jr	nz,l_28

	ld	bc,488			; set32(data + 488, 0xffffffff);
	lea	hl,iy
	add	hl,bc
	ld	a,255
	ld	(hl),a
	inc	hl
	ld	(hl),a
	inc	hl
	ld	(hl),a
	inc	hl
	ld	(hl),a

	ld	e,0
	ld	hl,0
.fsinfo := $ - 3
	call	write_sector		; write_sector(fsinfo, data);
l_28:
	ld	b,3			; for (i = 0; i < max_fd_open; i++)
	ld	hl,_fat_fd
	ld	de,26
l_25:
	ld	(hl),255		; fat_fd[i].key = -1;
	add	hl,de
	djnz	l_25
l_27:

	ld	a,1
	ld	(_fat_state + 0),a	; fat_state.valid = true;
	ret				; return 1;

; euhl = sector lba
read_sector:
	ld	bc,scsiRead10Lba
	call	add_fat_partition_lba
	ld	de,(_sector_buff)
	jp	scsiRequestRead

; euhl = sector lba
write_sector:
	ld	bc,scsiWrite10Lba
	call	add_fat_partition_lba
	ld	de,(_sector_buff)
	jp	scsiRequestWrite

_fat_ReadSector:
	ld	iy, 0
	add	iy, sp
	ld	de, (iy + 3)
	push	de
	ld	hl, (iy + 6)
	ld	e, (iy + 9)
	ld	bc, scsiRead10Lba
	call	add_fat_partition_lba
;	call	debugStr
;	db	'rd ', 0
;	ld	hl, scsiRead10Lba
;	call	debugHexBlockHL
;	db	4
;	call    debugNewLine
	pop	de
	call	scsiRequestRead
	xor	a, a
	ret

_fat_WriteSector:
	ld	iy, 0
	add	iy, sp
	ld	de,(iy + 3)
	push	de
	ld	hl, (iy + 6)
	ld	e, (iy + 9)
	ld	bc, scsiWrite10Lba
	call	add_fat_partition_lba
;	call	debugStr
;	db	'wr ', 0
;	ld	hl, scsiWrite10Lba
;	call	debugHexBlockHL
;	db	4
;	call    debugNewLine
	pop	de
	call	scsiRequestWrite
	xor	a, a
	ret

_fat_Select:
	ld	iy, 0
	add	iy, sp
	ld	e, (iy + 6)
	ld	d, 8
	mlt	de
	ld	hl, (iy + 3)
	add	hl, de
	ld	de, fat_partition_lba
	ld	bc, 4
	ldir
;	ld	hl, fat_partition_lba
;	call	debugHexBlockHL
;	db	4
;	call    debugNewLine
	ret

_fat_Find:
	ld	iy, 0
	add	iy, sp
	ld	hl, (iy + 3)			; fat_partition_t
	ld	a, (iy + 6)			; maximum
msd_mbr_parse:
	ld	(fat32_ptrs), hl
	ld	(fat32_max), a
	xor	a, a
	ld	(fat32_num), a
	sbc	hl, hl
	ld	(scsiRead10Lba), hl
	ld	(scsiRead10Lba + 3), a
	call	scsiRequestDefaultRead		; read sector
	ld	hl, (xferCBWDataDefault)
	ld	de, ($90 shl 16) or ($58 shl 8) or ($eb shl 0)
	or	a, a
	sbc	hl, de
   	add	hl, de				; check if boot sector
	jr	z, fat32_only			; this should only happen on the first one
	call	find_fat32
	ld	a, 0
fat32_num := $ - 1
	ret

add_fat_partition_lba:
	push	bc
	ld	bc, (fat_partition_lba)
	ld	a, (fat_partition_lba + 3)
	add	hl, bc			; __ladd
	adc	a, e			; big endian
	pop	de
	ld	(de), a
	dec	sp
	push	hl
	inc	sp
	pop	af			; hlu
	inc	de
	ld	(de), a
	ld	a,h
	inc	de
	ld	(de), a
	ld	a,l
	inc	de
	ld	(de), a
	ret

find_fat32:
	call	scsiRequestDefaultRead		; read sector
	call	check_sector_magic
	ret	nz
	ld	hl, -64
	add	hl, sp
	ld	sp, hl
	ex	de, hl
	ld	hl, xferCBWDataDefault + 446 + 4
	ld	bc, 64
	ldir					; copy the current partition table to the stack
	xor	a, a
	sbc	hl, hl
	add	hl, sp
	ld	a, 4
.loop:
	push	af
	push	hl
	ld	a, (hl)
;	call	debugHexA
	cp	a, $0b				; fat32 partition?
	call	z, fat32_found
	cp	a, $0c				; fat32 partition?
	call	z, fat32_found
	cp	a, $0f				; extended partition?
	call	z, ebr_found
	cp	a,$05				; extended partition? (chs)
	call	z, ebr_found
	pop	hl
	ld	bc, 16
	add	hl, bc

;	push	hl
;	call	debugStr
;	db	'next loop', 0
;	call	debugNewLine
;	ld	a, 200
;	call	_DelayTenTimesAms
;	pop	hl

	pop	af
	dec	a
	jr	nz, .loop
;	call	debugStr
;	db	'exited loop.', 0
;	call	debugNewLine
	ld	hl, 64
	add	hl, sp
	ld	sp, hl
	ret

fat32_only:
	call	check_sector_magic
	ld	a, 0
	ret	nz
	inc	a
	ld	(fat32_num), a
	ld	hl, (fat32_ptrs)
	push	hl
	pop	de
	ld	(hl), 0
	ld	bc, 7
	inc	de
	ldir
	ret

fat32_found:
	push	af
;	call	debugStr
;	db	'found fat', 0
;	call	debugNewLine
	ld	a, (fat32_num)
	cp	a, 0
fat32_max := $ - 1
	jr	z, .found_max
	ld	bc, 4				; hl -> end of lba
	add	hl, bc
	push	hl
	ld	c, 8
	ld	de, 0
fat32_ptrs := $ - 3
	ldir
	ld	(fat32_ptrs), de
	pop	hl
	ld	de, scsiRead10Lba + 3
	call	reverse_copy
;	call	scsiRequestDefaultRead		; read sector
;	ld	hl,xferCBWDataDefault
;	call	debugHexBlockHL
;	db	16
;	call	debugNewLine

.found_max:
	ld	hl, fat32_num
	inc	(hl)
	pop	af
	ret

ebr_found:
	push	af
;	call	debugStr
;	db	'found ebr', 0
;	call	debugNewLine
	ld	bc, 4				; hl -> end of lba
	add	hl, bc
	ld	de, scsiRead10Lba + 3
	call	reverse_copy
	call	find_fat32			; recursively locate fat32 partitions
	pop	af
	ret

reverse_copy:
	ld	b, 4
.loop:
	ld	a, (hl)
	ld	(de), a
	inc	hl
	dec	de
	djnz	.loop
	ret

check_sector_magic:
	ld	hl, xferCBWDataDefault + 510	; offset = signature
test_sector:
	ld	a, (hl)
	cp	a, $55
	ret	nz
	inc	hl
	ld	a, (hl)
	cp	a, $aa
	ret

fat_partition_lba:
	db	0,0,0,0

