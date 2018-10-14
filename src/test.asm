
	.assume adl=1
	.def	_msd_Init
	.def	_msd_ReadSector
	.def	_msd_WriteSector
	.def	_usb_Cleanup
	.def	_msd_KeepAlive
	.def	_fat_Find
	.def	_fat_Select
	.def	_fat_ReadSector
	.def	_fat_WriteSector

	.def	_fname_to_fatname
	.def	_cluster_to_sector
	.def	_init_fat
	.def	_end_of_chain_mark
	.ref	_fat_state
	.ref	_sector_buff
	.ref	_fat_fd

saveSScreen		:= $0D0EA1F ; 21945 bytes

usbbuf			:= saveSScreen

__longjmp                  := 0000098h
__memchr                   := 000009Ch
__memcmp                   := 00000A0h
__memcpy                   := 00000A4h
__memmove                  := 00000A8h
__memset                   := 00000ACh
__memclear                 := 00000B0h
__setjmp                   := 00000B8h
__strcat                   := 00000C0h
__strchr                   := 00000C4h
__strcmp                   := 00000C8h
__strcpy                   := 00000CCh
__strcspn                  := 00000D0h
__strlen                   := 00000D4h
__strncat                  := 00000D8h
__strncmp                  := 00000DCh
__strncpy                  := 00000E0h
__strpbrk                  := 00000E4h
__strrchr                  := 00000E8h
__strspn                   := 00000ECh
__strstr                   := 00000F0h
__bldiy                    := 00000FCh
__bshl                     := 0000100h
__bshru                    := 0000104h
__bstiy                    := 0000108h
__bstix                    := 000010Ch
__case                     := 0000110h
__case16                   := 0000114h
__case16D                  := 0000118h
__case24                   := 000011Ch
__case24D                  := 0000120h
__case8                    := 0000124h
__case8D                   := 0000128h
__frameset                 := 000012Ch
__frameset0                := 0000130h
__iand                     := 0000134h
__icmpzero                 := 0000138h
__idivs                    := 000013Ch
__idivu                    := 0000140h
__idvrmu                   := 0000144h
__ildix                    := 0000148h
__ildiy                    := 000014Ch
__imul_b                   := 0000150h
__imulu                    := 0000154h
__imuls                    := 0000158h
__indcall                  := 000015Ch
__ineg                     := 0000160h
__inot                     := 0000164h
__ior                      := 0000168h
__irems                    := 000016Ch
__iremu                    := 0000170h
__ishl                     := 0000174h
__ishl_b                   := 0000178h
__ishrs                    := 000017Ch
__ishrs_b                  := 0000180h
__ishru                    := 0000184h
__ishru_b                  := 0000188h
__istix                    := 000018Ch
__istiy                    := 0000190h
__itol                     := 0000194h
__ixor                     := 0000198h
__ladd                     := 000019Ch
__ladd_b                   := 00001A0h
__land                     := 00001A4h
__lcmps                    := 00001A8h
__lcmpu                    := 00001ACh
__lcmpzero                 := 00001B0h
__ldivs                    := 00001B4h
__ldivu                    := 00001B8h
__ldvrmu                   := 00001BCh
__lldix                    := 00001C0h
__lldiy                    := 00001C4h
__lmuls                    := 00001C8h
__lmulu                    := 00001CCh
__lneg                     := 00001D0h
__lnot                     := 00001D4h
__lor                      := 00001D8h
__lrems                    := 00001DCh
__lremu                    := 00001E0h
__lshl                     := 00001E4h
__lshrs                    := 00001E8h
__lshru                    := 00001ECh
__lstix                    := 00001F0h
__lstiy                    := 00001F4h
__lsub                     := 00001F8h
__lxor                     := 00001FCh
__sand                     := 0000200h
__scmpzero                 := 0000204h
__sdivs                    := 0000208h
__sdivu                    := 000020Ch
__seqcase                  := 0000210h
__seqcaseD                 := 0000214h
__setflag                  := 0000218h
__sldix                    := 000021Ch
__sldiy                    := 0000220h
__smuls                    := 0000224h
__smulu                    := 0000228h
__sneg                     := 000022Ch
__snot                     := 0000230h
__sor                      := 0000234h
__srems                    := 0000238h
__sremu                    := 000023Ch
__sshl                     := 0000240h
__sshl_b                   := 0000244h
__sshrs                    := 0000248h
__sshrs_b                  := 000024Ch
__sshru                    := 0000250h
__sshru_b                  := 0000254h
__sstix                    := 0000258h
__sstiy                    := 000025Ch
__stoi                     := 0000260h
__stoiu                    := 0000264h
__sxor                     := 0000268h
__fppack                   := 000026Ch
__fadd                     := 0000270h
__fcmp                     := 0000274h
__fdiv                     := 0000278h
__ftol                     := 000027Ch
__ultof                    := 0000280h
__ltof                     := 0000284h
__fmul                     := 0000288h
__fneg                     := 000028Ch
__fsub                     := 0000290h
__frbtof                   := 000029Ch
__frftob                   := 00002A0h
__frftoub                  := 00002A4h
__frftoi                   := 00002A8h
__frftoui                  := 00002ACh
__frftos                   := 00002B0h
__frftous                  := 00002B4h
__fritof                   := 00002B8h
__fruitof                  := 00002BCh
__frstof                   := 00002C0h
__frubtof                  := 00002C4h
__frustof                  := 00002C8h

_PutC			:= $0207B8
_PutS			:= $0207C0
_DispHL_s		:= $0207BC
_DispHL			:= $021EE0
_NewLine		:= $0207F0
_usb_IsBusPowered	:= $0003CC
_usb_BusPowered		:= $0003E4
_usb_SelfPowered	:= $0003E8
_Delay10ms		:= $0003B4
_DelayTenTimesAms	:= $0003B8
_MemClear		:= $0210DC
_HomeUp			:= $020828
_ClrLCDFull		:= $020808

flags			:= $D00080
mpTmrCtrl		:= $0F20030
bTmr3Enable		:= 6

mathprintFlags		:= $0044
mathprintEnabled	:= 5

pUsbRange		:= $03000
mpUsbRange		:= $0E20000
usbCapLen		:= $0000
pUsbCapLen		:= pUsbRange + usbCapLen
mpUsbCapLen		:= mpUsbRange + usbCapLen
usbHciVer		:= 0002h
pUsbHciVer		:= pUsbRange + usbHciVer
mpUsbHciVer		:= mpUsbRange + usbHciVer
usbHcsParams		:= 0004h
pUsbHcsParams		:= pUsbRange + usbHcsParams
mpUsbHcsParams		:= mpUsbRange + usbHcsParams
usbHccParams		:= 0008h
pUsbHccParams		:= pUsbRange + usbHccParams
mpUsbHccParams		:= mpUsbRange + usbHccParams
usbCmd			:= 0010h
pUsbCmd			:= pUsbRange + usbCmd
mpUsbCmd		:= mpUsbRange + usbCmd
usbSts			:= 0014h
pUsbSts			:= pUsbRange + usbSts
mpUsbSts		:= mpUsbRange + usbSts
usbInt			:= 0018h
pUsbInt			:= pUsbRange + usbInt
mpUsbInt		:= mpUsbRange + usbInt
usbFrIdx		:= 001Ch
pUsbFrIdx		:= pUsbRange + usbFrIdx
mpUsbFrIdx		:= mpUsbRange + usbFrIdx
usbPeriodicListBase	:= 0024h
pUsbPeriodicListBase	:= pUsbRange + usbPeriodicListBase
mpUsbPeriodicListBase	:= mpUsbRange + usbPeriodicListBase
usbAsyncListAddr	:= 0028h
pUsbAsyncListAddr	:= pUsbRange + usbAsyncListAddr
mpUsbAsyncListAddr	:= mpUsbRange + usbAsyncListAddr
usbPortSC		:= 0030h
pUsbPortSC		:= pUsbRange + usbPortSC
mpUsbPortSC		:= mpUsbRange + usbPortSC
usbMisc			:= 0040h
pUsbMisc		:= pUsbRange + usbMisc
mpUsbMisc		:= mpUsbRange + usbMisc
usbOtgCsr		:= 0080h
pUsbOtgCsr		:= pUsbRange + usbOtgCsr
mpUsbOtgCsr		:= mpUsbRange + usbOtgCsr
usbOtgIsr		:= 0084h
pUsbOtgIsr		:= pUsbRange + usbOtgIsr
mpUsbOtgIsr		:= mpUsbRange + usbOtgIsr
usbOtgIer		:= 0088h
pUsbOtgIer		:= pUsbRange + usbOtgIer
mpUsbOtgIer		:= mpUsbRange + usbOtgIer
usbIsr			:= 00C0h
pUsbIsr			:= pUsbRange + usbIsr
mpUsbIsr		:= mpUsbRange + usbIsr
usbImr			:= 00C4h
pUsbImr			:= pUsbRange + usbImr
mpUsbImr		:= mpUsbRange + usbImr
usbDevCtrl		:= 0100h
pUsbDevCtrl		:= pUsbRange + usbDevCtrl
mpUsbDevCtrl		:= mpUsbRange + usbDevCtrl
usbDevAddr		:= 0104h
pUsbDevAddr		:= pUsbRange + usbDevAddr
mpUsbDevAddr		:= mpUsbRange + usbDevAddr
usbDevTest		:= 0108h
pUsbDevTest		:= pUsbRange + usbDevTest
mpUsbDevTest		:= mpUsbRange + usbDevTest
usbSofFnr		:= 010Ch
pUsbSofFnr		:= pUsbRange + usbSofFnr
mpUsbSofFnr		:= mpUsbRange + usbSofFnr
usbSofMtr		:= 0110h
pUsbSofMtr		:= pUsbRange + usbSofMtr
mpUsbSofMtr		:= mpUsbRange + usbSofMtr
usbPhyTmsr		:= 0114h
pUsbPhyTmsr		:= pUsbRange + usbPhyTmsr
mpUsbPhyTmsr		:= mpUsbRange + usbPhyTmsr
usbCxsr			:= 011Ch
pUsbCxsr		:= pUsbRange + usbCxsr
mpUsbCxsr		:= mpUsbRange + usbCxsr
usbCxFifo		:= 0120h
pUsbCxFifo		:= pUsbRange + usbCxFifo
mpUsbCxFifo		:= mpUsbRange + usbCxFifo
usbIdle			:= 0124h
pUsbIdle		:= pUsbRange + usbIdle
mpUsbIdle		:= mpUsbRange + usbIdle
usbGimr			:= 0130h
pUsbGimr		:= pUsbRange + usbGimr
mpUsbGimr		:= mpUsbRange + usbGimr
usbCxImr		:= 0134h
pUsbCxImr		:= pUsbRange + usbCxImr
mpUsbCxImr		:= mpUsbRange + usbCxImr
usbFifoRxImr		:= 0138h
pUsbFifoRxImr		:= pUsbRange + usbFifoRxImr
mpUsbFifoRxImr		:= mpUsbRange + usbFifoRxImr
usbFifoTxImr		:= 013Ah
pUsbFifoTxImr		:= pUsbRange + usbFifoTxImr
mpUsbFifoTxImr		:= mpUsbRange + usbFifoTxImr
usbDevImr		:= 013Ch
pUsbDevImr		:= pUsbRange + usbDevImr
mpUsbDevImr		:= mpUsbRange + usbDevImr
usbGisr			:= 0140h
pUsbGisr		:= pUsbRange + usbGisr
mpUsbGisr		:= mpUsbRange + usbGisr
usbCxIsr		:= 0144h
pUsbCxIsr		:= pUsbRange + usbCxIsr
mpUsbCxIsr		:= mpUsbRange + usbCxIsr
usbFifoRxIsr		:= 0148h
pUsbFifoRxIsr		:= pUsbRange + usbFifoRxIsr
mpUsbFifoRxIsr		:= mpUsbRange + usbFifoRxIsr
usbFifoTxIsr		:= 014Ah
pUsbFifoTxIsr		:= pUsbRange + usbFifoTxIsr
mpUsbFifoTxIsr		:= mpUsbRange + usbFifoTxIsr
usbDevIsr		:= 014Ch
pUsbDevIsr		:= pUsbRange + usbDevIsr
mpUsbDevIsr		:= mpUsbRange + usbDevIsr
usbRxZlp		:= 0150h
pUsbRxZlp		:= pUsbRange + usbRxZlp
mpUsbRxZlp		:= mpUsbRange + usbRxZlp
usbTxZlp		:= 0154h
pUsbTxZlp		:= pUsbRange + usbTxZlp
mpUsbTxZlp		:= mpUsbRange + usbTxZlp
usbIsoEasr		:= 0158h
pUsbIsoEasr		:= pUsbRange + usbIsoEasr
mpUsbIsoEasr		:= mpUsbRange + usbIsoEasr
usbIep1			:= 0160h
pUsbIep1		:= pUsbRange + usbIep1
mpUsbIep1		:= mpUsbRange + usbIep1
usbIep2			:= 0164h
pUsbIep2		:= pUsbRange + usbIep2
mpUsbIep2		:= mpUsbRange + usbIep2
usbIep3			:= 0168h
pUsbIep3		:= pUsbRange + usbIep3
mpUsbIep3		:= mpUsbRange + usbIep3
usbIep4			:= 016Ch
pUsbIep4		:= pUsbRange + usbIep4
mpUsbIep4		:= mpUsbRange + usbIep4
usbIep5			:= 0170h
pUsbIep5		:= pUsbRange + usbIep5
mpUsbIep5		:= mpUsbRange + usbIep5
usbIep6			:= 0174h
pUsbIep6		:= pUsbRange + usbIep6
mpUsbIep6		:= mpUsbRange + usbIep6
usbIep7			:= 0178h
pUsbIep7		:= pUsbRange + usbIep7
mpUsbIep7		:= mpUsbRange + usbIep7
usbIep8			:= 017Ch
pUsbIep8		:= pUsbRange + usbIep8
mpUsbIep8		:= mpUsbRange + usbIep8
usbOep1			:= 0180h
pUsbOep1		:= pUsbRange + usbOep1
mpUsbOep1		:= mpUsbRange + usbOep1
usbOep2			:= 0184h
pUsbOep2		:= pUsbRange + usbOep2
mpUsbOep2		:= mpUsbRange + usbOep2
usbOep3			:= 0188h
pUsbOep3		:= pUsbRange + usbOep3
mpUsbOep3		:= mpUsbRange + usbOep3
usbOep4			:= 018Ch
pUsbOep4		:= pUsbRange + usbOep4
mpUsbOep4		:= mpUsbRange + usbOep4
usbOep5			:= 0190h
pUsbOep5		:= pUsbRange + usbOep5
mpUsbOep5		:= mpUsbRange + usbOep5
usbOep6			:= 0194h
pUsbOep6		:= pUsbRange + usbOep6
mpUsbOep6		:= mpUsbRange + usbOep6
usbOep7			:= 0198h
pUsbOep7		:= pUsbRange + usbOep7
mpUsbOep7		:= mpUsbRange + usbOep7
usbOep8			:= 019Ch
pUsbOep8		:= pUsbRange + usbOep8
mpUsbOep8		:= mpUsbRange + usbOep8
usbEp1Map		:= 01A0h
pUsbEp1Map		:= pUsbRange + usbEp1Map
mpUsbEp1Map		:= mpUsbRange + usbEp1Map
usbEp2Map		:= 01A1h
pUsbEp2Map		:= pUsbRange + usbEp2Map
mpUsbEp2Map		:= mpUsbRange + usbEp2Map
usbEp3Map		:= 01A2h
pUsbEp3Map		:= pUsbRange + usbEp3Map
mpUsbEp3Map		:= mpUsbRange + usbEp3Map
usbEp4Map		:= 01A3h
pUsbEp4Map		:= pUsbRange + usbEp4Map
mpUsbEp4Map		:= mpUsbRange + usbEp4Map
usbEp5Map		:= 01A4h
pUsbEp5Map		:= pUsbRange + usbEp5Map
mpUsbEp5Map		:= mpUsbRange + usbEp5Map
usbEp6Map		:= 01A5h
pUsbEp6Map		:= pUsbRange + usbEp6Map
mpUsbEp6Map		:= mpUsbRange + usbEp6Map
usbEp7Map		:= 01A6h
pUsbEp7Map		:= pUsbRange + usbEp7Map
mpUsbEp7Map		:= mpUsbRange + usbEp7Map
usbEp8Map		:= 01A7h
pUsbEp8Map		:= pUsbRange + usbEp8Map
mpUsbEp8Map		:= mpUsbRange + usbEp8Map
usbFifo0Map		:= 01A8h
pUsbFifo0Map		:= pUsbRange + usbFifo0Map
mpUsbFifo0Map		:= mpUsbRange + usbFifo0Map
usbFifo1Map		:= 01A9h
pUsbFifo1Map		:= pUsbRange + usbFifo1Map
mpUsbFifo1Map		:= mpUsbRange + usbFifo1Map
usbFifo2Map		:= 01AAh
pUsbFifo2Map		:= pUsbRange + usbFifo2Map
mpUsbFifo2Map		:= mpUsbRange + usbFifo2Map
usbFifo3Map		:= 01ABh
pUsbFifo3Map		:= pUsbRange + usbFifo3Map
mpUsbFifo3Map		:= mpUsbRange + usbFifo3Map
usbFifo0Cfg		:= 01ACh
pUsbFifo0Cfg		:= pUsbRange + usbFifo0Cfg
mpUsbFifo0Cfg		:= mpUsbRange + usbFifo0Cfg
usbFifo1Cfg		:= 01ADh
pUsbFifo1Cfg		:= pUsbRange + usbFifo1Cfg
mpUsbFifo1Cfg		:= mpUsbRange + usbFifo1Cfg
usbFifo2Cfg		:= 01AEh
pUsbFifo2Cfg		:= pUsbRange + usbFifo2Cfg
mpUsbFifo2Cfg		:= mpUsbRange + usbFifo2Cfg
usbFifo3Cfg		:= 01AFh
pUsbFifo3Cfg		:= pUsbRange + usbFifo3Cfg
mpUsbFifo3Cfg		:= mpUsbRange + usbFifo3Cfg
usbFifo0Csr		:= 01B0h
pUsbFifo0Csr		:= pUsbRange + usbFifo0Csr
mpUsbFifo0Csr		:= mpUsbRange + usbFifo0Csr
usbFifo1Csr		:= 01B4h
pUsbFifo1Csr		:= pUsbRange + usbFifo1Csr
mpUsbFifo1Csr		:= mpUsbRange + usbFifo1Csr
usbFifo2Csr		:= 01B8h
pUsbFifo2Csr		:= pUsbRange + usbFifo2Csr
mpUsbFifo2Csr		:= mpUsbRange + usbFifo2Csr
usbFifo3Csr		:= 01BCh
pUsbFifo3Csr		:= pUsbRange + usbFifo3Csr
mpUsbFifo3Csr		:= mpUsbRange + usbFifo3Csr
usbDmaFifo		:= 01C0h
pUsbDmaFifo		:= pUsbRange + usbDmaFifo
mpUsbDmaFifo		:= mpUsbRange + usbDmaFifo
usbDmaCtrl		:= 01C8h
pUsbDmaCtrl		:= pUsbRange + usbDmaCtrl
mpUsbDmaCtrl		:= mpUsbRange + usbDmaCtrl
usbDmaLen		:= 01C9h
pUsbDmaLen		:= pUsbRange + usbDmaLen
mpUsbDmaLen		:= mpUsbRange + usbDmaLen
usbDmaAddr		:= 01CCh
pUsbDmaAddr		:= pUsbRange + usbDmaAddr
mpUsbDmaAddr		:= mpUsbRange + usbDmaAddr

_msd_Init:
	push	ix
	push	iy
	call	initMsd			; attempt to initialize mass storage device
	jr	nc,initFailed
	pop	iy
	pop	ix
	xor	a,a
	inc	a
	ret
initFailed:
	call	usbCleanup
	pop	iy
	pop	ix
	xor	a,a
	ret

_usb_Cleanup:
	push	ix
	push	iy
	call	usbCleanup
	pop	iy
	pop	ix
	ret

include 'debug.inc'
include 'host.inc'
include 'usb.inc'
include 'msd.inc'
include 'fat.inc'

