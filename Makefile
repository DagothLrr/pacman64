#!smake

include C:/ultra/usr/include/make/PRdefs 


CODEFILES  	= main.c subs.c
HFILES		= pacman.h
DATAFILES	= cfb.c static.c textures.c tlut.c tilemap.c
APP 		= pacman.out
TARGETS 	= pacman.n64

OPTIMIZER 	= -g
LCDEFS 		= -DF3DEX_GBI_2 $(HW_FLAGS)
LCOPTS		= 

LDFLAGS = -L$(ROOT)/usr/lib -L$(ROOT)/usr/lib/PR -lgultra_d -L$(GCCDIR)/mipse/lib -lkmc

#object files
CODEOBJECTS = $(CODEFILES:.c=.o)
DATAOBJECTS = $(DATAFILES:.c=.o)
#thing
CODESEGMENT = codesegment.o
OBJECTS = $(CODESEGMENT) $(DATAOBJECTS)

default: $(TARGETS)
#link all code object files together into one fat object
$(CODESEGMENT): $(CODEOBJECTS)
	$(LD) -o $(CODESEGMENT) -r $(CODEOBJECTS) $(LDFLAGS)

#make the rom using the code segments and "data segments"
$(TARGETS) $(APP): spec $(OBJECTS)
	$(MAKEROM) -r $(TARGETS) -e $(APP) spec

include $(ROOT)/usr/include/make/commonrules
#?????
