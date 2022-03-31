#include <ultra64.h>
#include "pacman.h"

/* 2 bits of fraction  */
static Vp vp = {
	SCREEN_WD*2, SCREEN_HT*2, G_MAXZ/2, 0, /* "scale"  */
	SCREEN_WD*2, SCREEN_HT*2, G_MAXZ/2, 0
};

/* initialise RSP state  */
Gfx rspinit[] = {
	gsSPViewport(OS_K0_TO_PHYSICAL(&vp)),
	gsSPClearGeometryMode(G_SHADE | G_SHADING_SMOOTH | G_CULL_BOTH 
			| G_FOG | G_LIGHTING | G_TEXTURE_GEN 
			| G_TEXTURE_GEN_LINEAR | G_LOD),
	gsSPTexture(0,0,0,0,G_OFF),
	gsSPEndDisplayList()
};

/* initialise RDP state  */
Gfx rdpinit[] = {
	gsDPSetScissor(G_SC_NON_INTERLACE, 0, 0, SCREEN_WD, SCREEN_HT),
	gsDPSetCombineKey(G_CK_NONE),
	gsDPSetRenderMode(G_RM_NOOP, G_RM_NOOP2),
	gsDPSetAlphaCompare(G_AC_NONE),
	gsDPSetColorDither(G_CD_DISABLE),
	gsDPSetCycleType(G_CYC_COPY),
	gsDPSetTexturePersp(G_TP_NONE),
	gsDPSetTextureFilter(G_TF_POINT),
	gsDPSetTextureLUT(G_TT_RGBA16),
	/* TODO - load static colour tables  */
	gsDPPipeSync(),
	gsSPEndDisplayList(),
};

Gfx clearcfb[] = {
	gsDPSetCycleType(G_CYC_FILL),
	gsDPSetFillColor(CLEARCOLOR << 16 | CLEARCOLOR),
	gsDPFillRectangle(0, 0, SCREEN_WD-1, SCREEN_HT-1),
	gsDPPipeSync(),
	gsSPEndDisplayList()
};
