_fat_Select:
	ld	iy, 0
	add	iy, sp
	ld	e,(iy + 6)
	ld	d,8
	mlt	de
	ld	hl,(iy + 3)
	add	hl,de
	ld	de,partition_lba
	ld	bc,4
	ldir
;	ld	hl,partition_lba
;	call	debugHexBlockHL
;	db	4
	ret

partition_lba:
	db	0,0,0,0

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
	ld	hl,(xferCBWDataDefault)
	ld	de,($90 shl 16) or ($58 shl 8) or ($eb shl 0)
	or	a,a
	sbc	hl,de
   	add	hl,de				; check if boot sector
	jr	z,fat32_only			; this should only happen on the first one
	call	find_fat32
	ld	a, 0
fat32_num := $ - 1
	ret

find_fat32:
	call	scsiRequestDefaultRead		; read sector
	call	check_sector_magic
	ret	nz
	ld	hl,-64
	add	hl,sp
	ld	sp,hl
	ex	de,hl
	ld	hl,xferCBWDataDefault + 446 + 4
	ld	bc,64
	ldir					; copy the current partition table to the stack
	xor	a,a
	sbc	hl,hl
	add	hl,sp
	ld	a,4
.loop:
	push	af
	push	hl
	ld	a,(hl)
;	call	debugHexA
	cp	a,$0c				; fat32 partition?
	call	z,fat32_found
	cp	a,$0f				; extended partition?
	call	z,ebr_found
	cp	a,$05				; extended partition? (chs)
	call	z,ebr_found
	pop	hl
	ld	bc,16
	add	hl,bc

;	push	hl
;	call	debugStr
;	db	'next loop',0
;	call	debugNewLine
;	ld	a,200
;	call	_DelayTenTimesAms
;	pop	hl

	pop	af
	dec	a
	jr	nz,.loop
;	call	debugStr
;	db	'exited loop.',0
;	call	debugNewLine
	ld	hl,64
	add	hl,sp
	ld	sp,hl
	ret

fat32_only:
	call	check_sector_magic
	ld	a,0
	ret	nz
	inc	a
	ld	(fat32_num),a
	ld	hl,(fat32_ptrs)
	push	hl
	pop	de
	ld	(hl),0
	ld	bc,7
	inc	de
	ldir
	ret

fat32_found:
	push	af
;	call	debugStr
;	db	'found fat',0
;	call	debugNewLine
	ld	a,(fat32_num)
	cp	a,0
fat32_max := $ - 1
	jr	z,.found_max
	ld	bc,4				; hl -> end of lba
	add	hl,bc
	push	hl
	ld	c,8
	ld	de,0
fat32_ptrs := $ - 3
	ldir
	ld	(fat32_ptrs),de
	pop	hl
	ld	de,scsiRead10Lba + 3
	call	reverse_copy
;	call	scsiRequestDefaultRead		; read sector
;	ld	hl,xferCBWDataDefault
;	call	debugHexBlockHL
;	db	16
;	call	debugNewLine

.found_max:
	ld	hl,fat32_num
	inc	(hl)
	pop	af
	ret

ebr_found:
	push	af
;	call	debugStr
;	db	'found ebr',0
;	call	debugNewLine
	ld	bc,4				; hl -> end of lba
	add	hl,bc
	ld	de,scsiRead10Lba + 3
	call	reverse_copy
	call	find_fat32			; recursively locate fat32 partitions
	pop	af
	ret

reverse_copy:
	ld	b,4
.loop:
	ld	a,(hl)
	ld	(de),a
	inc	hl
	dec	de
	djnz	.loop
	ret

check_sector_magic:
	ld	hl,xferCBWDataDefault + 510	; offset = signature
	ld	a,(hl)
	cp	a,$55
	ret	nz
	inc	hl
	ld	a,(hl)
	cp	a,$aa
	ret

