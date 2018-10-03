_fat_find:
	pop	de
	pop	hl
	pop	af
	push	af
	push	hl
	push	de
msd_mbr_parse:
	ld	(fat32_ptrs),hl
	ld	(fat32_max),a
	xor	a,a
	ld	(fat32_num),a
	sbc	hl,hl
	ld	(scsiRead10Lba),hl
	ld	(scsiRead10Lba + 3),a
	call	find_fat32
	ld	a,0
fat32_num := $ - 1
	ret

find_fat32:
	call	scsiRequestDefaultRead		; read sector
	call	check_sector_magic
	ret	nz
	ld	hl,-48
	add	hl,sp
	ld	sp,hl
	ex	de,hl
	ld	hl,xferCBWDataDefault + 446 + 4
	ld	bc,48
	ldir					; copy the current partition table to the stack
	xor	a,a
	sbc	hl,hl
	add	hl,sp
	ld	a,4
.loop:
	push	af
	push	hl
	ld	a,(hl)
	cp	a,$0c				; fat32 partition?
	call	z,fat32_found
	cp	a,$0f				; extended partition?
	call	z,ebr_found
	pop	hl
	ld	bc,16
	add	hl,bc
	pop	af
	dec	a
	jr	nz,.loop
	ld	hl,48
	add	hl,sp
	ld	sp,hl
	ret

fat32_found:
	push	af
	ld	a,(fat32_num)
	cp	a,0
fat32_max := $ - 1
	jr	z,.found_max
	ld	bc,8				; hl -> end of lba
	add	hl,bc
	ld	de,0
fat32_ptrs := $ - 1
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
	ld	bc,4
	add	hl,bc
	ld	de,scsiRead10Lba
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
	cp	a,$55
	ret	nz
	inc	hl
	ld	a,(hl)
	cp	a,$aa
	ret

