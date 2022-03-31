
/* change if too big/not big enough */
#define STACKSIZE 0x1000
#define GLIST_LEN 0x0400

#define SCREEN_WD 320
#define SCREEN_HT 240

#define GHOST_TLUT_ID 0
#define PAC_TLUT_ID   1
#define VULN1_TLUT_ID 2
#define VULN2_TLUT_ID 3
#define TILE_TLUT_ID  4

#define CLEARCOLOR GPACK_RGBA5551(50, 64, 255, 1)

#define TILEMAP_WIDTH 28
#define TILEMAP_HEIGHT 31
#define TILEDIMS 8

/* ghost index enum  */
enum {blinky, pinky, inky, clyde};
//tile value mappings
enum {TILE_PELLET=37, TILE_POWERPELLET, TILE_NONE};
//ghost modes
enum {chase, scatter, vulnerable, goToHouse, 
	enterDoor, cooldown, leaveHouse};
enum {up, down, left, right};
//gamestates
enum {ready, pause, playing, ghostLynched,
	dead, winnar, showScore, levelTransition};

//Data structures
typedef struct
{
    int x, y;
    int tileX, tileY;

    int targetX, targetY; /* coordinate to pathfind to */

    char xDir, yDir; /* direction vector */
    char xOffset, yOffset; /* position relative to local tile */

    unsigned int dotCounter; //number dots pacman has eaten since entering the ghost house - SCRATCH THAT, NOW JUST NUMBER OF FRAMES
    unsigned int dotThreshold;
    //if dotCounter > threshold -> switch to leave the ghost house

    float subPixelX;
    float subPixelY;
    float speed;    // number of subpixels per frame

    unsigned char homeX, homeY; //home tile (one of the map corners)
    unsigned char direction; //also used to select if vulnerable
    unsigned char mode; //switch between chasing the player, scattering to home corners, being vulnerable, being dead, going to the monster house, leaving the monster house
    unsigned char ID; //used to select which 'AI type' is used

} ghost;
extern ghost ghosts[];
extern int curGhostFrame;

/* "Dynamic" data struct  */
/* groups all the data that changes per-frame  */
/* easier to writeback only specific cache lines, instead of full cache  */
typedef struct 
{

	Gfx glist[GLIST_LEN]; /* dynamic display list  */
}Dynamic;
/* instance created in main  */
extern Dynamic dynamic;

/* static display lists - defined in static.c */
extern Gfx rspinit[];
extern Gfx rdpinit[];
extern Gfx clearcfb[];

/* tilemap data - defined in tilemap.c  */
extern unsigned char tilemapData[];

/* textures - defined in textures.c  */
extern u16 ghost1[];
extern u16 ghost2[];

/* texture colour LUTs - defined in tlut.c */
extern u16 ghostTLUTs[4][4];

/* 2 16 bit cfbs - created in cfb.c */
extern u16 cfb[][SCREEN_WD*SCREEN_HT];



