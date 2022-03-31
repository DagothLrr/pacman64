
#include "pacman.h"

beginseg
	name "code"
	flags BOOT OBJECT
	entry boot
	stack bootStack + STACKSIZE
	include "codesegment.o"
	include "$(ROOT)/usr/lib/pr/rspboot.o"
	include "$(ROOT)/usr/lib/pr/gspF3DEX2.xbus.o"
endseg

beginseg
	name "static"
	flags OBJECT
	after "code"
	align 16
	include "static.o"
	include "tilemap.o"
endseg

beginseg
	name "textures"
	flags OBJECT
	after "static"
	include "textures.o"
	include "tlut.o"
endseg

beginseg
	name "cfb"
	flags OBJECT
	address 0x80100000
	include "cfb.o"
endseg

beginwave
	name "compiledrom"
	include "code"
	include "static"
	include "textures"
	include "cfb"
endwave
