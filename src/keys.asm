    .assume adl=1
    .def _key_Scan
    .def _key_Any

;-------------------------------------------------------------------------------
_key_Scan:
	di
	ld	hl,$f50200		; DI_Mode = $f5xx00
	ld	(hl),h
	xor	a,a
key_scan:
	cp	a,(hl)
	jr	nz,key_scan
	ret

;-------------------------------------------------------------------------------
_key_Any:
	di
	ld	hl,$f50200		; DI_Mode = $f5xx00
	ld	(hl),h
	xor	a,a
key_loop:
	cp	a,(hl)
	jr	nz,key_loop
	ld	l,$12			; kbdG1 = $f5xx12
	or	a,(hl)
	inc	hl
	inc	hl
	or	a,(hl)
	inc	hl
	inc	hl
	or	a,(hl)
	inc	hl
	inc	hl
	or	a,(hl)
	inc	hl
	inc	hl
	or	a,(hl)
	inc	hl
	inc	hl
	or	a,(hl)
	inc	hl
	inc	hl
	or	a,(hl)
	ret
