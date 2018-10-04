_fat_find:
	ld	iy, 0
	add	iy, sp
	ld	hl, (iy + 3)
	ld	a, (iy + 6)
msd_mbr_parse:
	ld	(fat32_ptrs), hl
	ld	(fat32_max), a
	xor	a, a
	ld	(fat32_num), a
	sbc	hl, hl
	ld	(scsiRead10Lba), hl
	ld	(scsiRead10Lba + 3), a
	call	find_fat32
	call	debugStr
	db	'returning...',0
	call	debugNewLine
	ld	a, 0
fat32_num := $ - 1
	ret

find_fat32:
	call	debugStr
	db	'entered find',0
	call	debugNewLine
	call	scsiRequestDefaultRead		; read sector
	call	check_sector_magic
	ret	nz
	call	debugStr
	db	'passed magic',0
	call	debugNewLine
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
	call	debugHexA
	cp	a,$0c				; fat32 partition?
	call	z,fat32_found
	cp	a,$0f				; extended partition?
	call	z,ebr_found
	pop	hl
	ld	bc,16
	add	hl,bc

	push	hl
	call	debugStr
	db	'next loop',0
	call	debugNewLine
	ld	a,100
	call	_DelayTenTimesAms
	pop	hl

	pop	af
	dec	a
	jr	nz,.loop
	call	debugStr
	db	'attempting to finish!',0
	call	debugNewLine
	ld	hl,64
	add	hl,sp
	ld	sp,hl
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
	ld	bc,8				; hl -> end of lba
	add	hl,bc
	ld	de,0
fat32_ptrs := $ - 3
	call	reverse_copy
	ld	bc,8
	add	hl,bc				; hl -> end of size
	call	reverse_copy
	ld	(fat32_ptrs),de
.found_max:
	ld	hl,fat32_num
	inc	(hl)
	pop	af
	ret

ebr_found:
	push	af
	call	debugStr
	db	'found ebr',0
	call	debugNewLine
	ld	de,scsiRead10Lba
	ld	bc,4
	add	hl,bc
	call	debugHexBlockHL
	db	8
	ldi
	ldi
	ldi
	ldi
	call	find_fat32			; recursively locate fat32 partitions
	pop	af
	ret

reverse_copy:
	ld	b,4
.loop:
	ld	a,(hl)
	ld	(de),a
	dec	hl
	inc	de
	djnz	.loop
	ret

check_sector_magic:
	ld	hl,xferCBWDataDefault + 510	; offset = signature
	ld	a,(hl)
	call	debugHexA
	cp	a,$55
	ret	nz
	inc	hl
	ld	a,(hl)
	call	debugHexA
	cp	a,$aa
	ret

