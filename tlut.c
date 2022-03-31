#include <ultra64.h>
/*#include "paccie.h"
*/
#define blinkycol GPACK_RGBA5551(0xff, 0x00, 0x00, 1)
#define pinkycol  GPACK_RGBA5551(0xff, 0xb7, 0xff, 1)
#define inkycol   GPACK_RGBA5551(0x00, 0xff, 0xff, 1)
#define clydecol  GPACK_RGBA5551(0xff, 0xb7, 0x51, 1)

static Gfx dummy_aligner1[] = { gsSPEndDisplayList() };
const u16 ghostTLUTs[4][4] = {
	{0x0000, blinkycol, 0x18FF, 0xDEF7}, /* blinky  */
	{0x0000, pinkycol,  0x18FF, 0xDEF7}, /* pinky   */
	{0x0000, inkycol,   0x18FF, 0xDEF7}, /* inky    */
	{0x0000, clydecol,  0x18FF, 0xDEF7}  /* clyde   */
};


