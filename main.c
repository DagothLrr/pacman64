#include <ultra64.h>

#include "pacman.h"

/* makerom symbols used for cart-dram dma  */
extern char _codeSegmentEnd[];
extern char _staticSegmentRomStart[], _texturesSegmentRomEnd[];

u64 bootStack[STACKSIZE/sizeof(u64)];
u64 dram_stack[SP_DRAM_STACK_SIZE8] __attribute((aligned(16)));

/* maybe move these and idle thread into "boiler" header?  */
static void idle(void*);
static void mainproc(void*);

/* TODO: move somewhere sensible  */
static void drawGhosts(Gfx** glistpp);

static OSThread idleThread;
static u64 idleThreadStack[STACKSIZE/sizeof(u64)];

static OSThread mainThread;
static u64 mainThreadStack[STACKSIZE/sizeof(u64)];

OSPiHandle* handler;

#define PI_MSG_LEN 8
/* required message queue for PI?  */
OSMesgQueue PiMessageQueue;
OSMesg PiMessages[PI_MSG_LEN];

/* messages + message queues for thread synchronisation  */
OSMesg dmaMessageBuf, rdpMessageBuf, retraceMessageBuf;
OSMesgQueue dmaMessageQueue, rdpMessageQueue, retraceMessageQueue;
OSIoMesg dmaIOMesgBuf;

/* globals----------------------------------------------- */
/* RSP task descriptor structure  */
OSTask tlist = {
	M_GFXTASK, OS_TASK_DP_WAIT,
	NULL, 0,
	(u64*)gspF3DEX2_xbusTextStart, SP_UCODE_SIZE,
	(u64*)gspF3DEX2_xbusDataStart, SP_UCODE_DATA_SIZE,
	OS_K0_TO_PHYSICAL(dram_stack), SP_DRAM_STACK_SIZE8,
	NULL, 0, NULL, 0, NULL, 0
};

/* dynamic data (glist)  */
Dynamic dynamic;

/* Main entry point  */
/* Initialise OS and create idle thread  */
void boot()
{
	osInitialize();
	handler = osCartRomInit(); 
	osCreateThread(&idleThread, 1, idle, (void*)0, idleThreadStack+STACKSIZE/sizeof(u64), 10);
	osStartThread(&idleThread);
}

/* Idle thread  */
/* sets up video interface stuff  */
/* initialises main thread & sets priority to 0 */
/* when no other threads are runnable, run infinite loop */
void idle(void* arg)
{
	osCreateViManager(OS_PRIORITY_VIMGR);
	osViSetMode(&osViModeTable[OS_VI_NTSC_LAN1]);
	osViSetSpecialFeatures(OS_VI_GAMMA_OFF | OS_VI_GAMMA_DITHER_OFF);

	osCreatePiManager((OSPri)OS_PRIORITY_PIMGR, &PiMessageQueue, PiMessages,PI_MSG_LEN);

	osCreateThread(&mainThread, 3, mainproc, NULL, mainThreadStack+STACKSIZE/sizeof(u64), 10);
	osStartThread(&mainThread);
	
	osSetThreadPri(NULL, OS_PRIORITY_IDLE);

	for(;;);
}


void testf()
{
	float x = 1.0;
	return;
}

/* main  */
void mainproc(void* arg)
{
	/* variable defs  */
	int curbuf = 0;
	int i = 0;
	int j = 0;
	unsigned char* dmaAddrp;

	Gfx* glistp;
	
	float test;
	test = 0.0;
	j = (int)test;

	
	osCreateMesgQueue(&rdpMessageQueue, &rdpMessageBuf, 1);
	osSetEventMesg(OS_EVENT_DP, &rdpMessageQueue, 0);

	osCreateMesgQueue(&retraceMessageQueue, &retraceMessageBuf, 1);
	osViSetEvent(&retraceMessageQueue, NULL, 1);

	tlist.t.ucode_boot = (u64*)rspbootTextStart;
	tlist.t.ucode_boot_size = (u32)rspbootTextEnd-(u32)rspbootTextStart;

  	/* DMA all other resources into DRAM  */
/* TODO  */

	dmaAddrp = _codeSegmentEnd;
	osCreateMesgQueue(&dmaMessageQueue, &dmaMessageBuf, 1);
	dmaIOMesgBuf.hdr.pri = OS_MESG_PRI_NORMAL;
	dmaIOMesgBuf.hdr.retQueue = &dmaMessageQueue;
	dmaIOMesgBuf.dramAddr = dmaAddrp;
	dmaIOMesgBuf.devAddr = (u32)_staticSegmentRomStart;
	dmaIOMesgBuf.size = (u32)_texturesSegmentRomEnd-(u32)_staticSegmentRomStart;
	
	osInvalDCache(dmaAddrp, (u32)_texturesSegmentRomEnd-(u32)_staticSegmentRomStart);

	osEPiStartDma(handler, &dmaIOMesgBuf, OS_READ);

  	/* wait for DMA to finish  */
	(void)osRecvMesg(&dmaMessageQueue, NULL, OS_MESG_BLOCK);

	test = 1.0;	
	/* main game loop  */
	/* original game's loop is fixed to refresh rate */
	/* no seperate render/update threads  */
	while(1){
		
	/* update  */
		resetGhosts();

	/* render  */
		
		/* create display list  */
		glistp = dynamic.glist;
		/* set segID #0 base address to 0x0 - access to all dram */
		gSPSegment(glistp++, 0, 0x0);
		/* init RCP */
		gSPDisplayList(glistp++, OS_K0_TO_PHYSICAL(rdpinit));
		gSPDisplayList(glistp++, OS_K0_TO_PHYSICAL(rspinit));
		/* set framebuffer */
		gDPSetColorImage(glistp++, G_IM_FMT_RGBA, G_IM_SIZ_16b, 
				SCREEN_WD, OS_K0_TO_PHYSICAL(&cfb[curbuf]));
		/* clear framebuffer  */
		gSPDisplayList(glistp++, OS_K0_TO_PHYSICAL(clearcfb));
/* TEST HARDWARE SPRITE  */
		gDPPipeSync(glistp++);
		
		/* prepare for drawing sprites  */
		/* enable textures  */
		gSPTexture(glistp++, 0x2000, 0x2000, 0, G_TX_RENDERTILE, G_ON);	
		gDPSetCycleType(glistp++, G_CYC_COPY);

		/* draw background  */
/* TODO  */
		/* draw ghosts  */
	//	drawGhosts(&glistp);

		/* draw paccie  */
/* TODO  */
		
		/* finish display list and sync mainthread with RDP  */
		gDPFullSync(glistp++);
		gSPEndDisplayList(glistp++);

		/* ensure DRAM matches cache before starting RSP task  */
		osWritebackDCache(&dynamic, sizeof(dynamic));  
		
		/* start RSP task  */
		tlist.t.data_ptr = (u64*)dynamic.glist;
		tlist.t.data_size = sizeof(Gfx)*(glistp - dynamic.glist);
		osSpTaskStart(&tlist);
		/* wait until RDP finishes drawing  */
		(void)osRecvMesg(&rdpMessageQueue, NULL, OS_MESG_BLOCK);
		
		/* TEST SOFTWARE SPRITE  
		nwah = ghost1;
		for(i = 0; i<32; i++){
			for(j=0; j<32; j+=2){
				cfb[curbuf][j+320*i] = ghostTLUTs[0][*(nwah)>>4];	
				cfb[curbuf][1+j+320*i] = ghostTLUTs[0][0xF&*(nwah)];
				nwah++;
			}
		}
		*/
	/* swap buffers  */
		osViSwapBuffer(cfb[curbuf]);
		/* synchronize with retrace  */
		(void)osRecvMesg(&retraceMessageQueue, NULL, OS_MESG_BLOCK);
		curbuf ^= 1;
	}
}

void drawGhosts(Gfx** glistpp)
{
	Gfx* glistp = *glistpp;
	int i;
	int x;
	int y;

	/* load texture depending on current animation frame  */	
	gDPLoadTextureBlock_4b(glistp++, ghost1, G_IM_FMT_CI, 32, 32, 0,
			        G_TX_NOMIRROR, G_TX_NOMIRROR, 0, 0,
				G_TX_NOLOD, G_TX_NOLOD);

	/* loop through ghosts, set tlut and draw with correct cropping   */
	/* dont draw ghost if ghost is dead or vulnerable - needs dif tex */
	/* TODO add vulnerable texture into ghost1+2 and maybe eyes */
	/* actually eyes arent even needed? just change palette  */

	for(i=0; i<4; i++){
		x = ghosts[i].x;
		y = ghosts[i].y;
		gDPLoadTLUT_pal16(glistp++, 0, ghostTLUTs[i]);
		gSPTextureRectangle(glistp++, x<<2, y<<2, (x+16-1)<<2,
				(y+16-1)<<2, 0, 16<<10, 0, 4<<10, 1<<10);
	}

	/* update the fat display list  */
	*glistpp = glistp;
	return;
}
