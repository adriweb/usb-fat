#include "ti84pce.inc"
	.org userMem-2
	.db tExtTok,tAsm84CeCmp

#ifndef NDEBUG
	ld      a,(iy+mathprintFlags)
	res     mathprintEnabled,(iy+mathprintFlags)
	push    af
	call    _HomeUp
	call    _ClrLCDFull
#endif
	push    iy
	call	initMsd			; attempt to initialize msd
	jr	nc,initFailed
	call	fs_mount
initFailed:
	call    usbCleanup
	pop     iy
#ifndef NDEBUG
	pop     af
	ld      (iy+mathprintFlags),a
#endif
	ret

#include "debug.inc"
#include "host.inc"
#include "usb.asm"
#include "msd.asm"

#define ld_ehl_ix(xx) ld hl,(ix+xx) \ ld e,(ix+xx+3)
#define ld_ehl_iy(xx) ld hl,(iy+xx) \ ld e,(iy+xx+3)
#define ld_abc_ix(xx) ld bc,(ix+xx) \ ld a,(ix+xx+3)
#define ld_abc_iy(xx) ld bc,(iy+xx) \ ld a,(iy+xx+3)

#define ld_ix_ehl(xx) ld (ix+xx),hl \ ld (ix+xx+3),e
#define ld_iy_ehl(xx) ld (iy+xx),hl \ ld (iy+xx+3),e
#define ld_ix_abc(xx) ld (ix+xx),bc \ ld (ix+xx+3),a
#define ld_iy_abc(xx) ld (iy+xx),bc \ ld (iy+xx+3),a

BS_VolLab32 = 71
BPB_FSInfo32 = 48
MBR_Table = 446

; mount a logical drive
;  z = success
; nz = failure
fs_mount:
	xor	a,a
	sbc	hl,hl
	ld	(xferReadLba),hl
	ld	(xferReadLba+3),a			; zero lba
	call	scsiRequestDefaultReadSector		; read the mbr at sector 0
	call	fs_check
	cp	a,2					; 2 = no fat filesytem
	jr	z,noFatRecord
	or	a,a
	jr	z,goodFatRecord				; probably fdisk format here instead if a == 1
	ld	iy,xferReadDataDefault+MBR_Table	; offset = mbr table
	ld	a,(iy+4)
	or	a,a
	jr	z,noFatRecord				; type of partition must exist
	lea	hl,iy+8					; partition offset in lba
	ld	de,fs_struct+fs_volbase
	call	util_cp32				; store as little-endian
	ld	de,xferReadLba				; scsi expects big-endian
	call	util_cp32_little_to_big
	call	scsiRequestDefaultReadSector		; get the fat record sector
	call	fs_check
	or	a,a
	jr	z,goodFatRecord				; verify valid fat
noFatRecord:
	xor	a,a
	inc	a
	ret
goodFatRecord:
	ld	ix,xferReadDataDefault			; sector data
	ld	iy,fs_struct				; pointer to fat structure

	lea	hl,ix + BS_VolLab32			; get the volume label
	ld	de,OP6
	push	de
	call	_Mov11b
	xor	a,a
	ld	(OP6+11),a
	pop	hl
	debugCall(debugHLStr)				; looks good
	debugCall(debugNewLine)

	ld	a,(ix+42)
	or	a,a
	jr	nz,noFatRecord
	ld	a,(ix+43)
	or	a,a
	jr	nz,noFatRecord				; ensure only support for fat32 v0.0

	ld_ehl_ix(44)
	ld_iy_ehl(fs_dirbase)				; root directory start cluster
	debugCallHexBlock(fs_struct+fs_dirbase, 4)
	debugCall(debugNewLine)				; looks good

	ld_ehl_ix(36)					; euhl = number of sectors per fat
	ld_iy_ehl(fs_fatsize)				; fs_fasize = number of sectors per fat
	debugCallHexBlock(fs_struct+fs_fatsize, 4)
	debugCall(debugNewLine)				; looks good

	xor	a,a
	ld	bc,0
	ld	c,(ix+16)				; aubc = number of fats (2 usually)
	call	__lmulu					; euhl = fs_fasize * aubc
	ld_iy_ehl(fs_fasize)				; fs_fasize = number of sectors per fat
	debugCallHexBlock(fs_struct+fs_fasize, 4)
	debugCall(debugNewLine)				; looks good

	xor	a,a
	ld	bc,0
	ld	c,(ix+14)
	ld	b,(ix+15)				; nrsv: number of reserved sectors
	ld	(nrsv),bc
	call	__ladd					; sysect: nrsv + fasize
	ld	a,e
	push	hl
	pop	bc
	push	bc
	push	af
	ld_ehl_ix(32)					; tsect: total number of sectors on the volume
	call	__lsub					; tsect - sysect
	xor	a,a
	ld	bc,0
	ld	c,(ix+13)
	ld	(iy+fs_csize),c
	call	__ldivu					; nclst: (tsect - sysect) / fs_csize;
	ld	c,2
	call	__ladd					; num_fatent: nclst + 2
	ld_iy_ehl(fs_num_fatent)
	debugCallHexBlock(fs_struct+fs_num_fatent, 4)
	debugCall(debugNewLine)				; looks good

	ld_ehl_iy(fs_volbase)
	debugCallHexBlock(fs_struct+fs_volbase, 4)
	debugCall(debugNewLine)				; looks good

	ld	bc,0
nrsv =$-3
	xor	a,a
	call	__ladd
	ld_iy_ehl(fs_fatbase)				; fatbase: volbase + nrsv
	debugCallHexBlock(fs_struct+fs_fatbase, 4)
	debugCall(debugNewLine)				; looks good

	ld_ehl_iy(fs_volbase)
	pop	af
	pop	bc					; aubc = sysect
	call	__ladd
	ld_iy_ehl(fs_database)				; database: volbase + sysect
	debugCallHexBlock(fs_struct+fs_database, 4)
	debugCall(debugNewLine)				; looks good

	ld	a,(ix + BPB_FSInfo32)			; get filesystem information if available
	cp	a,1
	jr	nz,nofileinfo

	ld_ehl_iy(fs_volbase)
	inc	hl					; fs_volbase + 1 -- probably unsafe to do this

	call	fs_move_window

nofileinfo:

	xor	a,a
	ld	(fs_struct+fs_flag),a
	ret

; move to different sector
; inputs:
;  euhl: sector
; outputs:
;
fs_move_window:
	ret


	; static FRESULT move_window (	/* Returns FR_OK or FR_DISK_ERR */
	; 	FATFS* fs,			/* Filesystem object */
	; 	DWORD sector		/* Sector number to make appearance in the fs->win[] */
	; )
	; {
	; 	FRESULT res = FR_OK;
        ;
        ;
	; 	if (sector != fs->winsect) {	/* Window offset changed? */
	; #if !FF_FS_READONLY
	; 		res = sync_window(fs);		/* Write-back changes */
	; #endif
	; 		if (res == FR_OK) {			/* Fill sector window with new data */
	; 			if (disk_read(fs->pdrv, fs->win, sector, 1) != RES_OK) {
	; 				sector = 0xFFFFFFFF;	/* Invalidate window if read data is not valid */
	; 				res = FR_DISK_ERR;
	; 			}
	; 			fs->winsect = sector;
	; 		}
	; 	}
	; 	return res;
	; }


; builds a name based on the file input segment from 8.3 format
create_name:
	ret

; check for valid fat record
fs_check:
	ld	de,$55 | ($aa << 8)		; magic = 0xAA55
	ld	bc,510				; offset = signature
	call	fs_offset_magic
	ld	a,2
	ret	nz
	ld	de,$46 | ($41 << 8)		; magic = FAT32
	ld	bc,82				; offset = fat32 check
	call	fs_offset_magic
	ld	a,1
	ret	nz
	dec	a
	ret

; bc = offset
; de = check
fs_offset_magic:
	ld	hl,xferReadDataDefault
	add	hl,bc
	ld	a,(hl)				; ensure record signature == 0xAA55
	cp	a,e
	ret	nz
	inc	hl
	ld	a,(hl)
	cp	a,d				; not a boot record?
	ret

; move up to the parent directory
;  z = success
; nz = fail
dir_rewind:
	ret

; Input:
;  euhl = cluster
; Output:
;  euhl = sector
util_clust2sect:
	push	af
	ld	iy,fs_struct
	xor	a,a
	ld	bc,2
	call	__lsub				; clust -= 2
	xor	a,a				; need to check (clst >= (fs_num_fatent - 2) == invalid at some point
	ld	bc,0
	ld	c,(iy+fs_csize)
	call	__lmulu
	ld_abc_iy(fs_database)
	call	__ladd				; clust * fs_csize + fs_database
	pop	af
	ret


; Input:
;  iy -> directory entry
util_get_clust:
	ld	bc,(iy+21-2)			; ld bcu,(iy+21)
	ld	a,(iy+20)
	ld	c,(iy+26)
	ld	b,(iy+27)			; clst = (dir+20) << 16 | (dir+26);
	ret

util_cp32:
	push	bc
	push	de
	push	hl
	ldi
	ldi
	ldi
	ldi
	pop	hl
	pop	de
	pop	bc
	ret

util_cp32_little_to_big:
	inc	de
	inc	de
	inc	de
	ld	b,3
_:	ld	a,(hl)
	inc	hl
	ld	(de),a
	dec	de
	djnz	-_
	ret

util_zero:
	.db	0,0,0,0

; ---
; filesystem handling
; ---

; filesystem object structure
fs_struct:
fs_type =$-fs_struct
	.db	0	; fat type
fs_flag =$-fs_struct
	.db	0	; file status flags
fs_csize =$-fs_struct
	.db	0,0	; number of sectors per cluster + padding
fs_fasize =$-fs_struct
	.db	0,0,0,0	; number of sectors per fat
fs_fatsize =$-fs_struct
	.db	0,0,0,0	; number of sectors per fat
fs_volbase =$-fs_struct
	.db	0,0,0,0	; location of fat base
fs_num_fatent =$-fs_struct
	.db	0,0,0,0	; number of fat entries (clusters + 2)
fs_fatbase =$-fs_struct
	.db	0,0,0,0	; fat start sector
fs_dirbase =$-fs_struct
	.db	0,0,0,0	; root directory start sector (clusters)
fs_database =$-fs_struct
	.db	0,0,0,0	; data start sector
fs_fptr =$-fs_struct
	.db	0,0,0,0	; file read/write pointer
fs_fsize =$-fs_struct
	.db	0,0,0,0	; file size
fs_org_clust =$-fs_struct
	.db	0,0,0,0	; file start cluster
fs_curr_clust =$-fs_struct
	.db	0,0,0,0	; file current cluster
fs_dsect =$-fs_struct
	.db	0,0,0,0	; file current data sector
fs_last_clst =$-fs_struct
	.db	0,0,0,0	; last allocated cluster
fs_free_clst =$-fs_struct
	.db	0,0,0,0	; number of free clusters
fs_fsi_flag =$-fs_struct
	.db	0	; file system information flag
fs_winsect:
	.db	0,0,0,0 ; current visible sector

; ---
; directory handling
; ---

dir_index:
	.dl	0	; current read/write index number
dir_sclust:
	.db	0,0,0,0	; table start cluster
dir_clust:
	.db	0,0,0,0	; current cluster
dir_sect:
	.db	0,0,0,0	; currect sector
dir_fn:
	.db	0,0,0,0,0,0,0,0	; (i/o) { file[8], ext[3], status[1] }
	.db	0,0,0
	.db	0
dir_buf:
	.db	0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0
	.db	0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0
