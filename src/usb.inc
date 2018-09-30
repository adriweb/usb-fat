initDevice:
	call    usbInit				; init usb
	call    devFirst
	ld	(device),iy			; use first device
	ld      hl,usbbuf			; get dev desc
	ld      de,reqDevDesc18
	call    setup
	ld      hl,usbbuf+18			; get langids
	ld      de,reqGetStrDesc
	call    setup
	ld      hl,usbbuf+18+2
	ld      de,reqGetStrDesc+4
	ldi
	ldi					; use first langid
	ld      hl,(usbbuf+8)			; get vendor
	ld      hl,(usbbuf+10)			; get product
	ld      hl,(usbbuf+12)			; get release
	ld      a,(usbbuf+14)			; get manufacturer
	ld      a,(usbbuf+15)			; get product
	ld      a,(usbbuf+16)			; get serial number
	ld	iy,(device)			; get configuration descriptor
	xor     a,a
	call    getConfigDescTotalLength	; total length of config
	ld      (reqConfDesc+wLength),hl
	push    hl
	ld      hl,usbbuf
	ld      de,reqConfDesc
	call    setup
	pop	bc
	ld      hl,usbbuf
	ld	(config_start),hl
	add	hl,bc
	ld	(config_end),hl
	ld	iy,(device)			; select config
	ld      ix,usbbuf
	call	devSelectConfig
	ret

setup:
	ld	iy,(device)
	call	controlDefaultTransfer
	jp	qhWait

; Input:
;  a = index
debugStrDesc:
;if ~defined NDEBUG
	ld	hl, debugNewLine
	push	hl
;end if
	or      a,a
	jr      nz,.none
	call	debugStr
	db	'<NONE>',0
	ret
.none:
	ld      (reqGetStrDesc+2),a
	call	debugChar
	db	34
	call	.name
	call	debugChar
	db	34
	ret
.name:
	ld      hl,usbbuf+18
	ld      de,reqGetStrDesc
	push    hl
	call    setup
	pop     hl
	ld      b,(hl)
	srl     b
.loop:
	dec     b
	ret     z
	inc     hl
	inc     hl
	ld      de,(hl)
	ld      a,d
	or      a,a
	jr      nz,.first
	ld      a,e
	cp      a,$20
	jr      c,.first
	cp      a,$7f
	jr      nc,.first
	cp      a,$5b
	jr      nz,.draw
	ld      a,$c1
	jr      .draw
.first:
	ld      a,$d0
.draw:
	call	debugCharA
	jr      .loop

reqDevDesc18:
	db     $80,6,0,1,0,0,18,0
reqConfDesc:
	db     $80,6,1,2,0,0,0,0,0
reqGetStrDesc:
	db     $80,6,0,3,0,0,0,1
reqClearHalt:
	db	2,1,0,0,0,0,0,0

device:
	db	0,0,0
config_start:
	db	0,0,0
config_end:
	db	0,0,0

