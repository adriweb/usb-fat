xferCBWDataDefault  := usbbuf

initMsd:
	call	initDevice
	call	findFirstMsdInterface
	call	c,findScsiEndpoints
	ret	nc
	call	msdReset
	call	msdMaxLun
	call	msdSetLun
	call	scsiInit
	scf
	ret

scsiInit:
	ld	a,(ep_in)
	ld	(reqClrInStall + wIndex),a
	ld	a,(ep_out)
	ld	(reqClrOutStall + wIndex),a
	ld	hl,scsiInquiry
	call	scsiDefaultRequest
	ld	hl,scsiTestUnitReady
	call	scsiDefaultRequest
	ld	hl,scsiReqestSense
	call	scsiDefaultRequest
	ld	hl,scsiTestUnitReady
	call	scsiDefaultRequest
	ld	hl,scsiReadCapacity
	ld	de,lb_addr
	call	scsiRequest		; store the logical block address/size
	ld	hl,scsiModeSense6	; get the things
	call	scsiDefaultRequest
	ld	hl,scsiTestUnitReady	; test if ready again
	call	scsiDefaultRequest
	ld	hl,(block_size)
	ld	de,512
	or	a,a
	sbc	hl,de
	jp	nz,debugAbort		; ensure sector size is 512 bytes
	ret

_msd_KeepAlive:
	ld	hl,scsiTestUnitReady
	call	scsiDefaultRequest
	ld	a,50
	jp	_DelayTenTimesAms

_msd_ReadSector:
	call	__frameset0

	ld	a,(ix+9)
	ld	(scsiRead10Lba + 3),a
	ld	a,(ix+10)
	ld	(scsiRead10Lba + 2),a
	ld	a,(ix+11)
	ld	(scsiRead10Lba + 1),a
	ld	a,(ix+12)
	ld	(scsiRead10Lba + 0),a

	ld	de,(ix+6)
	call	scsiRequestRead

	xor	a,a
	sbc	hl,hl
	ld	sp,ix
	pop	ix
	ret

_msd_WriteSector:
	call	__frameset0

	ld	a,(ix+9)
	ld	(scsiWrite10Lba + 3),a
	ld	a,(ix+10)
	ld	(scsiWrite10Lba + 2),a
	ld	a,(ix+11)
	ld	(scsiWrite10Lba + 1),a
	ld	a,(ix+12)
	ld	(scsiWrite10Lba + 0),a

	ld	de,(ix+6)
	call	scsiRequestWrite

	xor	a,a
	sbc	hl,hl
	ld	sp,ix
	pop	ix
	ret

; scsi format:
; <[1] in/out>,<[3] i/o length>,<[1] cdb length>,<[n] cdb>

scsiInquiry:
	db	$80,$24,$00,$00,$06,$12,$00,$00,$00,$24,$00

scsiTestUnitReady:
	db	$00,$00,$00,$00,$06,$00,$00,$00,$00,$00,$00

scsiModeSense6:
	db	$80,$fc,$00,$00,$06,$1a,$00,$3f,$00,$fc,$00

scsiReqestSense:
	db	$80,$12,$00,$00,$06,$03,$00,$00,$00,$12,$00

scsiReadCapacity:
	db	$80,$08,$00,$00,$0a,$25,$00,$00,$00,$00,$00,$00,$00,$00,$00

scsiRead10:
	db	$80,$00,$02,$00,$0a,$28,$00
scsiRead10Lba:
	db	$00,$00,$00,$00
scsiRead10GroupNum:
	db	$00
scsiRead10Length:
	db	$00,$01
scsiRead10Ctrl:
	db	$00

scsiWrite10:
	db	$00,$00,$02,$00,$0a,$2a,$00
scsiWrite10Lba:
	db	$00,$00,$00,$00
scsiWrite10GroupNum:
	db	$00
scsiWrite10Length:
	db	$00,$01
scsiWrite10Ctrl:
	db	$00

; Input:
;  scsiRead10Lba = logical block address
;  de -> buffer
; Output:
;  buffer loaded
scsiRequestDefaultRead:
	ld	de,xferCBWDataDefault
scsiRequestRead:
	ld	hl,scsiRead10
	jr	scsiRequest

; Input:
;  scsiWriteiRead10Lba = logical block address
;  de -> buffer
; Output:
;  buffer loaded
scsiRequestDefaultWrite:
	ld	de,xferCBWDataDefault
scsiRequestWrite:
	ld	hl,scsiWrite10
	jr	scsiRequest

; Input:
;  hl = ptr to xfer
;  de = ptr to storage
; Output;
;   z = success
;  nz = failure
scsiDefaultRequest:
	ld	de,xferCBWDataDefault
scsiRequest:
	push	ix
	push	iy
	call	msdCommandTransport
	call	msdDataTransport
	call	msdStatusTransport
	pop	iy
	pop	ix
	ret

; ---

msdPerformResetRecovery:
	call	msdReset		; perform reset recovery
	call	msdClrInStall
;	jr	msdClrOutStall
msdClrOutStall:
	ld	de,reqClrOutStall
	jr	msdClrStall
msdClrInStall:
	ld	de,reqClrInStall
msdClrStall:
	ld	hl,usbbuf
	ld	iy,(device)
	jp	controlDefaultTransfer

; hl = data to transfer
; de = buffer to recieve
msdCommandTransport:
	ld	(xferData),hl
	ld	(xferCBWData),de
.ready
	ld	iy,packetCBW
	ld	a,(hl)
	ld	(msdDataEp),a
	ld	(iy+12),a		; direction flag
	push	hl
	inc	hl
	ld	bc,(hl)			; get the length -- only use 3/4 bytes
	ld	(iy+8),bc		; i/o length
	push	bc
	inc	hl
	inc	hl
	inc	hl			; move past length
	ld	bc,0
	ld	c,(hl)			; cdb length
	inc	c
	lea	de,iy+14		; cdb location
	ldir
	ld	hl,(iy+4)		; increment tag
	inc	hl
	ld	(iy+4),hl
	push	iy			; iy -> packetCBW
	call	msdDeviceEpOut
	pop	de			; de -> packetCBW
	ld	bc,packetCBWLen
	call	bulkTransfer
	call	qhReap
	pop	bc
	pop	hl
	or	a,a
	ret	z			; check the command was accepted
	add	a,a			; check if stall
	call	nz,debugAbort
.resendCbw:
	call	msdPerformResetRecovery
	ld	hl,0
xferData := $ - 3
	jr	msdCommandTransport.ready

; bc = length of transfer
msdDataTransport:
	ld	a,0
msdDataEp := $ - 1
	sbc	hl,hl
	adc	hl,bc
	ret	z			; no bulk if 0 length req
	ld	de,0
xferCBWData := $ - 3
	or	a,a
	jr	z,msdDataOut
msdDataIn:
	call	msdDeviceEpIn
	call	bulkTransfer
	call	qhReap
	or	a,a
	ret	z
	add	a,a
	call	nz,debugAbort
	jq	msdClrInStall

msdDataOut:
	call	msdDeviceEpOut
	call	bulkTransfer
	call	qhReap
	or	a,a
	ret	z
	add	a,a
	call	nz,debugAbort
	jq	msdClrOutStall

msdStatusTransport:
	call	msdDeviceEpIn
	ld	de,packetCSW
	ld	bc,packetCSWLen
	call	bulkTransfer
	call	qhReap
	or	a,a
	jr	z,msdCheckValidCsw
msdStatusStall:
	call	msdClrInStall		; clear stall
	call	msdDeviceEpIn
	ld	de,packetCSW
	ld	bc,packetCSWLen
	call	bulkTransfer
	call	qhReap			; attempt to read csw again
	or	a,a
	jr	nz,msdPhaseError
msdCheckValidCsw:
	ld	a,(packetCSW+12)
	or	a,a			; check for good status of transfer
	ret	z
	cp	a,2			; check for phase error (we can recover)
	call	nz,debugAbort		; handle command fail later
msdPhaseError:
	jp	msdCommandTransport.resendCbw

; Input:
;  hl = ptr in config
;   a = type
; Output:
;  nc = not found
;   c = found
getDescriptor:
	push	de
	push	bc
	ld	de,0
msd_config_end := $ - 3
	ld	bc,0
findDescriptor:
	or	a,a
	sbc	hl,de
	add	hl,de
	jr	nc,notFoundDescriptor
	push	hl
	pop	iy
	ld	c,(hl)			; length of descriptor
	cp	a,(iy + bDescriptorType)
	jr	z,foundDescriptor
	add	hl,bc
	jr	findDescriptor
foundDescriptor:
	scf
notFoundDescriptor:
	pop	bc
	pop	de
	ret

; Input:
;  hl = interface ptr
; Output:
;  nc = not found
;   c = found
findScsiEndpoints:
	ld	bc,6 or ($50 shl 8)	; check scsi bot
	call	checkSubclassProtocol
	jr	nz,endpointFail
	push	hl
	pop	iy
	ld	b,(iy+4)
	ld	c,0
	ld	de,0
findScsiEndpointsLoop:
	ld	a,5			; endpoint descriptor type
	call	getDescriptor
	ret	nc
	ld	a,(iy+2)
	bit	7,a
	jr	z,endpointOut
	bit	0,c
	jr	nz,endpointSkip
	ld	(ep_in),a
;	call	debugStr
;	db	'in: ',0
;	call	debugHexA
;	call	debugNewLine
	set	0,c
	jr	endpointSkip
endpointOut:
	bit	1,c
	jr	nz,endpointSkip
	ld	(ep_out),a
;	call	debugStr
;	db	'out: ',0
;	call	debugHexA
;	call	debugNewLine
	set	1,c
endpointSkip:
	ld	e,(iy+0)
	add	hl,de
	djnz	findScsiEndpointsLoop
	ld	a,c
	cp	a,2
	jr	z,endpointFail
;	call	debugStr
;	db	'valid endpoints',0
;	call	debugNewLine
	scf
	ret
endpointFail:
	scf
	ccf
	ret

; Input:
;  none
; Output:
;  hl = interface ptr
;  nc = not found
;   c = found
findFirstMsdInterface:
	ld	hl,0
msd_config_start := $ - 3
findMsdInterface:
	ld	a,4			; interface descriptor type
	call	getDescriptor
	ret	nc
	ld	a,(iy+0)		; bLength
	cp	a,9
	jr	c,findMsdInterface	; ensure interface length >= 9
	ld	a,(iy+5)
	cp	a,8			; bInterfaceClass == 8
	jr	nz,findMsdInterface
;	call	debugStr
;	db	'interface: ',0
	ld	a,(iy+2)
	ld	(packetMSDReset+4),a	; store the interface number
	ld	(packetMSDMaxLUN+4),a	; store the interface number
;	call	debugHexA
;	call	debugNewLine
	scf
	ret

; Input:
;   c = subclass
;   b = protocol
checkSubclassProtocol:
	ld	a,(iy+6)
	cp	a,c			; scsi subclass
	ret	nz
	ld	a,(iy+7)
	cp	a,b			; bot protocol
	ret

msdSetLun:
	ld	a,(curr_lun)
	ld	(packetCBW+13),a
	ret
msdMaxLun:
	ld	hl,max_lun
	ld	de,packetMSDMaxLUN
	jp	setup
msdReset:
	or	a,a
	sbc	hl,hl
	ld	(packetCBW + 4),hl
	ld	(packetCBW + 5),hl
	ld	de,packetMSDReset
	jp	setup

; Output:
;  flags are unaffected
msdDeviceEpOut:
ep_out := $ + 1
	ld	a,0
	jr	msdDeviceEpIn.set
msdDeviceEpIn:
ep_in := $ + 1
	ld	a,0
.set:
	ld	iy,(device)
	ret

max_lun:
	db	0
curr_lun:
	db	0

lb_addr:
	db	0,0,0,0
	db	0	; technically part of block size, but meh
block_size:
	db	0,0,0

packetMSDReset:
	db	$21,$FF,$00,$00,$00,$00,$00,$00
packetMSDMaxLUN:
	db	$A1,$FE,$00,$00,$00,$00,$01,$00

packetCSW:
	db	$00,$00,$00,$00	; signature
	db	$00,$00,$00,$00	; tag
	db	$00,$00,$00,$00	; residue
	db	$00		; status
packetCSWLen := $ - packetCSW

packetCBW:
	db	$55,$53,$42,$43	; signature
	db	$00,$00,$00,$00	; tag
	db	$00,$00,$00,$00	; length
	db	$00		; flags
	db	$00		; lun
packetCBWCBD:
	db	$00		; command block length
	db	$00		; command block opcode
	db	$00,$00,$00	; command block
	db	$00,$00,$00,$00	; command block
	db	$00,$00,$00,$00	; command block
	db	$00,$00,$00,$00	; command block
packetCBWLen := $ - packetCBW

