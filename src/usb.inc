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
;	ld      hl,(usbbuf+8)			; get vendor
;	ld      hl,(usbbuf+10)			; get product
;	ld      hl,(usbbuf+12)			; get release
;	ld      a,(usbbuf+14)			; get manufacturer
;	ld      a,(usbbuf+15)			; get product
;	ld      a,(usbbuf+16)			; get serial number
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
	ld	(msd_config_start),hl
	add	hl,bc
	ld	(msd_config_end),hl		; used by the msd driver
	ld	iy,(device)			; select config
	ld      ix,usbbuf
	call	devSelectConfig
	ret

_usb_Detached:
	call	debugStr
	db	'usb detached',0
	call	debugNewLine
	jp	debugAbort

setup:
	ld	iy,0
device := $ - 3
	call	controlDefaultTransfer
	ld	b,0
	jp	qhRetry

reqDevDesc18:
	db	$80,6,0,1,0,0,18,0
reqConfDesc:
	db	$80,6,0,2,0,0,0,0,0
reqGetStrDesc:
	db	$80,6,0,3,0,0,0,1
reqClrInStall:
	db	$02,1,0,0,0,0,0,0
reqClrOutStall:
	db	$02,1,0,0,0,0,0,0

