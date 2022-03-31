#include <ultra64.h>
#include "pacman.h"



//global variables
int score = 0;
int vulnTimer;
int gameState = ready;

int fruitActive;

int currentGhostCooldownID = pinky; //the highest priority ghost on cooldown (lower ID - higher priority) (default to pinky - he has no cooldown)

/* empty map buffer  */
unsigned char tilemap[TILEMAP_WIDTH*TILEMAP_HEIGHT];
ghost ghosts[4];
struct
{
    int x, y;       /* screen coords in pixels */
    int tileX, tileY;   /* current tile */

    unsigned char currentFrame;
    unsigned char direction;    //multiples of multiples of 14 between: 0 -> 28
    unsigned char frameCount;

    char xDir; //0 if not moving on x axis, 1 is going right, -1 if going left
    char yDir; //0 if not moving on y axis, 1 if going down, -1 if going up

    char xOffset, yOffset; //offset from centre of tile - used to correct around corners

    char currentKey; //current keypress

} sPacman;

/* function prototypes */
int pacmanPerFrame();
void pacmanPerTile();

void updateGhosts();
void updatePacman();

void ghostChooseBestDirection(ghost* s);
void ghostPerFrame(ghost* s);
void ghostPerTile(ghost* s);
void ghostSetMode(ghost* s, char mode); //oneshot function that set initial values for a ghost when an "interrupt" mode change happens (can happen any time)

void resetGhosts();
void resetPacman();
void resetBoard();


//////////////////////////////////////////////////////////////////////////////////////////////////////////////
///INIT (run once) FUNCTIONS
//////////////////////////////////////////////////////////////////////////////////////////////////////////////

//////////////////////////////////////////////////////////////////////////////////////////////////////////////
///PACMAN UPDATE FUNCTIONS
//////////////////////////////////////////////////////////////////////////////////////////////////////////////

/* function that runs every frame while changing between tiles (animation + moving between tiles + corner cutting)
 *  change x/y pixel position
 *  animate
 *  check if off screen (warp)
 */
int pacmanPerFrame()
{


	/* TODO FIX ANIMATION CROPPING  */
    #define framesPerFrame 1
    //update animation
    if(sPacman.xDir || sPacman.yDir){
           if(sPacman.frameCount++ > framesPerFrame){
        }
    }
    //move a pixel in the current direction
    sPacman.x += sPacman.xDir;
    sPacman.y += sPacman.yDir;

        //correct offset
    if(sPacman.xOffset > 0){ //too far right, correct by moving left
        sPacman.xOffset--; sPacman.x--;
    }else if(sPacman.xOffset < 0){
        sPacman.xOffset++; sPacman.x++;
    }

    if(sPacman.yOffset > 0){
        sPacman.yOffset--; sPacman.y--;
    }else if(sPacman.yOffset < 0){
        sPacman.yOffset++; sPacman.y++;
    }

    //warp if offscreen
    if(sPacman.x <= -TILEDIMS){
        sPacman.x = TILEDIMS * (TILEMAP_WIDTH)-1; return 0;
    }else if(sPacman.x >= TILEDIMS * (TILEMAP_WIDTH)){
        sPacman.x = -TILEDIMS+1; return 0;
    }

    return 0;

}



/* pacman update
 *  pick direction
 *  check collision
 *  start moving to tile if possible
 */
void updatePacman()
{
    int i;
    /* pointer to pacman's current tile */
    unsigned char* currentTile; 

    currentTile = &tilemap[sPacman.tileX + TILEMAP_WIDTH*(sPacman.tileY)];

    /* current tile Pacman is in */
    sPacman.tileX = (sPacman.x) / TILEDIMS;
    sPacman.tileY = (sPacman.y) / TILEDIMS;

    // at the end of warp pipe keep moving off screen
    if(sPacman.tileX == 0 || sPacman.tileX == TILEMAP_WIDTH-1){
        pacmanPerFrame();
        return;
    }

    //check what kind of tile Pacman is on
    if(*currentTile == TILE_PELLET){
            score++;
            *currentTile = TILE_NONE;
            return;
    }else if(*currentTile == TILE_POWERPELLET){
            score++;
            for(i = 0; i < 4; i++){
                if(ghosts[i].mode <= vulnerable)
                   ghostSetMode(&ghosts[i], vulnerable);
            }
            *currentTile = TILE_NONE;
    }
    //*currentTile = TILE_NONE;

    //the tile pacman should move to next
    //unsigned char nextTile;

    //handle keypresses
    switch( sPacman.currentKey ){
    case up: //check if north tile is valid
        //check if nextTile is valid
        if(currentTile[-TILEMAP_WIDTH] >= TILE_PELLET){
            sPacman.xDir = 0; sPacman.yDir = -1;
            sPacman.direction = 28;
            //offset is = (pixel offset in current tile) - 4
            sPacman.xOffset = (sPacman.x % TILEDIMS) - 4;
            goto changedDirection;
        }
        break;
    case down: //check if south tile is valid
        if(currentTile[TILEMAP_WIDTH] >= TILE_PELLET){
            sPacman.xDir = 0; sPacman.yDir = 1;
            sPacman.direction = 42;
            //offset is = (pixel offset in current tile) - 4
            sPacman.xOffset = (sPacman.x % TILEDIMS) - 4;
            goto changedDirection;
        }
        break;
    case left: //check if west tile is valid
        if(currentTile[-1] >= TILE_PELLET){
            sPacman.xDir = -1; sPacman.yDir = 0;
            sPacman.direction = 14;
            sPacman.yOffset = (sPacman.y % TILEDIMS) - 4;
            goto changedDirection;
        }
        break;
    case right: //check if east tile is valid
        if(currentTile[1] >= TILE_PELLET){
            sPacman.xDir = 1; sPacman.yDir = 0;
            sPacman.direction = 0;
            sPacman.yOffset = (sPacman.y % TILEDIMS) - 4;
            goto changedDirection;
        }
        break;

     //no keys pressed so continue in current direction
    }

    //direction stayed the same -> check if it results in a collision
    if(!(currentTile[sPacman.xDir + sPacman.yDir * TILEMAP_WIDTH] >= TILE_PELLET)){
        sPacman.xDir = 0; sPacman.yDir = 0;
        sPacman.xOffset = (sPacman.x % TILEDIMS) - 4;
        sPacman.yOffset = (sPacman.y % TILEDIMS) - 4;
    }
changedDirection:

    pacmanPerFrame();
    //invalidate inputs (maybe remove?)
    sPacman.currentKey = 255; //no key
}

//////////////////////////////////////////////////////////////////////////////////////////////////////////////
///SPOOKY UPDATE FUNCTIONS
//////////////////////////////////////////////////////////////////////////////////////////////////////////////

void ghostSetMode(ghost* s, char mode)
{
    //set initial target for ghost +
    switch (mode){
    case chase: //target is determined per-tile, keep going in current direction
        if(s->ID == blinky)
            s->speed = 1;
        else
            s->speed = 0.9;
        break;
    case scatter://target is home corner
        s->targetX = s->homeX; s->targetY = s->homeY;
        break;
    case vulnerable: //target is determined per-tile. set "direction" to 64 for scared sprite
        s->direction = 64;
        vulnTimer = 500;
        s->speed = 0.5;
        break;

    case goToHouse: //target is fixed (also switch to using "eyes" spritesheet)
        s->targetX = 14; s->targetY = 11;
        s->speed = 2;
        break;
    }

    s->mode = mode;
    s->x = (int)s->subPixelX;
    s->y = (int)s->subPixelY;

    //if not on a tile, calculate offset and pick next direction
    if(s->x & 7 || s->y & 7){
        //pick the best direction for the new target
        char oldDirX = s->xDir;
        char oldDirY = s->yDir;
        s->xDir = 0; s->yDir = 0;

        ghostChooseBestDirection(s);
        //if direction has changed, move back onto current tile
        if(!(s->xDir == oldDirX && s->yDir == oldDirY)){
            s->xOffset = s->x & 7;
            s->yOffset = s->y & 7;
        }
        return;
    }
    // direction will be automatically decided next updateFunction so reset direction
    s->xDir = 0; s->yDir = 0;
    return;
}

int curGhostFrame = 0;

void ghostPerFrame(ghost* s)
{
    //ghost animation
    static int counter;
    if(counter++ > 16){
        curGhostFrame ^= 1; //flicker between the two frames
        counter = 0;
    }

    //flicker animation when vulnerability is nearly worn off
    if(s->mode == vulnerable){
        if(vulnTimer < 100){
            s->direction = 80; //change to flicker code later
        }
        if(vulnTimer < 0){
            ghostSetMode(s, chase); //immediately switch to chase if timer ends
        }
    }

    switch(s->mode){
    //exitHouse, enterHouse and cooldown override usual behaviour
    case goToHouse: //move to tile above monster house, when there switch to "cooldown" mode
        if(s->x == 13 * TILEDIMS + 3){
            s->mode++; //reached house -> go to "enterDoor" animation
            s->direction = 48;
            return;
        }
        break;

    case enterDoor: //override pathfinding function to pass through the door (eventually move to respective location in ghost house)
        s->speed = 0.4;
        s->xDir = 0; s->yDir = 1; //move down until inside house
        if(s->y == 14 * TILEDIMS){
            s->yDir = 0;
            if(s->ID < inky){
                s->mode++; //in house -> wait for cooldown
                break;
            }
            if(s->ID == clyde){ //move to correct part of ghost house
                if(s->x < 13 * TILEDIMS + 3 + 16)
                    s->xDir = 1;
                else
                    s->mode++;
            }else if(s->ID == inky){
                if(s->x > 13 * TILEDIMS + 3 - 16)
                    s->xDir = -1;
                else
                    s->mode++;
            }
        }
        break;

    case cooldown: //wait until allowed to exit
        //check if priority is higher than lesser ghosts
        if(s->ID <= currentGhostCooldownID) currentGhostCooldownID = s->ID;
        if(s->dotCounter >= s->dotThreshold) s->mode++; //if dots eaten since death > (threshold) -> leave the house
        return;

    case leaveHouse: //override pathfinding function to move in set patterns
/// @TODO: code to force the ghost through the 'door' on the ghost house
        s->speed = 0.4;
        if(s->x != 13 * TILEDIMS + 3){
            if(s->ID == clyde)
                s->xDir = -1;
            else
                s->xDir = 1;
        }else{ //centred correctly
            s->xDir = 0; s->yDir = -1; //move up until outside house
            currentGhostCooldownID = 4; //reset to lowest priority
            if(s->y == 11*TILEDIMS){
                ghostSetMode(s, chase); //start in chase when exiting house
                s->xDir = -1; s->yDir = 0;
                s->direction = 16;
                break;
            }
        }
        break;
    }
    //check collision with pacman

    if(s->tileX == sPacman.tileX && s->tileY == sPacman.tileY){ //detect if touching pacman
        if(s->mode < vulnerable){
            gameState = dead;
        }else if(s->mode == vulnerable){
            gameState = ghostLynched;
            s->dotCounter = 0; //reset dot counter on death
            s->xDir = 0; s->yDir = 0; //pick completely new direction (allow 180*)
            ghostSetMode(s, goToHouse); //start respawn behaviour
        }
    }


    //correct offset
    if(s->xOffset > 0){ //too far right, correct by moving left
        s->xOffset--; s->x--;
        s->subPixelX = s->x;
        return; //dont apply movement
    }
    if(s->yOffset > 0){ //too far down, correct by moving up
        s->yOffset--; s->y--;
        s->subPixelY = s->y;
        return; //dont apply movement
    }

    s->subPixelX += s->xDir * s->speed;
    s->subPixelY += s->yDir * s->speed;

    s->x = (int)s->subPixelX;
    s->y = (int)s->subPixelY;


    //warp pipe
    if(s->x < -TILEDIMS+1){
        s->subPixelX = (TILEDIMS * (TILEMAP_WIDTH)) - 1;
        s->x = s->subPixelX;
        s->xDir = -1; s->yDir = 0;
    }else if(s->x > TILEDIMS * (TILEMAP_WIDTH) - 1){
        s->subPixelX = -TILEDIMS+1;
        s->x = s->subPixelX;
        s->xDir = 1; s->yDir = 0;
    }

}

void ghostChooseBestDirection(ghost* s)
{
    /// @TODO Move direction choosing code out of ghostPerTile. Change direction instantly based on which brings the target tile closer
    unsigned char* currentTile = &tilemap[s->tileX + s->tileY * TILEMAP_WIDTH];

    //variables for deciding on direction
    int distanceToPacman = 6000000; //actually square of distance (starts are centre of turn)

    //differences
    int dX = s->targetX - s->tileX;
    int dY = s->targetY - s->tileY;

    // distance^2 = dX^2 + dY^2
    int dXSquared = dX * dX;
    int dYSquared = dY * dY;

    // (dX +/- 1)^2 = dX^2 +/-2dX + 1
    int dXDoubled = dX << 1;
    int dYDoubled = dY << 1;

    int testDistance;

    int xDirBuf = s->xDir; //use previous direction if no tiles are valid
    int yDirBuf = s->yDir;
    //check tile in front, left and right of currentTile & determine next direction

    if(currentTile[-TILEMAP_WIDTH] >= TILE_PELLET && !(s->yDir>0) ){ //check if tile above is valid
        if( (testDistance= dXSquared + dYSquared + dYDoubled) < distanceToPacman ){    //if valid compare distance
            //if distance is less than current best, take that direction
            xDirBuf = 0; yDirBuf = -1;
            distanceToPacman = testDistance;
        }
    }

    if(currentTile[TILEMAP_WIDTH] >= TILE_PELLET && !(s->yDir<0) ){ //check if tile below is valid
        if( (testDistance= dXSquared + dYSquared - dYDoubled) < distanceToPacman ){
            xDirBuf = 0; yDirBuf = 1;
            distanceToPacman = testDistance;
        }
    }

    if(currentTile[-1] >= TILE_PELLET && !(s->xDir>0) ){ //check if tile left is valid
        if( (testDistance= dXSquared + dXDoubled + dYSquared) < distanceToPacman ){    //if valid compare distance
            //if distance is less than current best, take that direction
            xDirBuf = -1; yDirBuf = 0;
            distanceToPacman = testDistance;
        }
    }

    if(currentTile[1] >= TILE_PELLET && !(s->xDir<0) ){ //check if tile right is valid
        if( (testDistance= dXSquared - dXDoubled + dYSquared) < distanceToPacman ){
            xDirBuf = 1; yDirBuf = 0;
            distanceToPacman = testDistance;
        }
    }

    s->xDir = xDirBuf;
    s->yDir = yDirBuf;
}

/// @TODO change function to have different behaviour for each "mode" (scatter, chase, vulnerable, dead, cooldown, respawn)
void ghostPerTile(ghost* s)
{
    //get current tile
    s->tileX = s->x / TILEDIMS;
    s->tileY = s->y / TILEDIMS;

    //check which "mode" the ghost is in (instead of a switch tree i could have a function pointer. it might be faster?)
    switch(s->mode){
    case chase: //chase mode: set target based on the type of ghost and move towards the current target
    //set target tile
        switch(s->ID){ //i could use a FP* here too, "getTarget"
        case blinky:
            s->targetX = sPacman.tileX; s->targetY = sPacman.tileY; //pacman's exact location
            break;
        case pinky:
            s->targetX = sPacman.tileX + 4*sPacman.xDir; s->targetY = sPacman.tileY + 4*sPacman.yDir; //4 tiles in front of pacman
            break;
        case inky: //inky is a complete mess - sort of mirrors blinky's position relative to pacman
            {
                int fX = sPacman.tileX + 2*sPacman.xDir; int fY = sPacman.tileY + 2*sPacman.yDir; //2 tiles in front of pacman
                int dX = fX - ghosts[blinky].tileX; int dY = fY - ghosts[blinky].tileY; //vector from "focus" to blinky
                s->targetX = fX + dX*2; s->targetY = fY + dY*2; //flip vector around focus
                break;
            }
        case clyde: //stupid can't even get near pacman
            if((sPacman.tileX - s->tileX)*(sPacman.tileX - s->tileX) +
               (sPacman.tileY - s->tileY) * (sPacman.tileY - s->tileY) < 8*TILEDIMS){ //if pacman is within 8 tiles, go to knave corner
                s->targetX = s->homeX; s->targetY = s->homeY;
            }else{
                s->targetX = sPacman.tileX; s->targetY = sPacman.tileY; //go to pacman if he is far
            }
        }

        ghostChooseBestDirection(s); //destination based on target
        break;

    case scatter: //target is ghost's home corner
        ghostChooseBestDirection(s); //destination based on target
        break;

    case vulnerable: //pick random direction. if blocked -> try the next direction (also move slower)
    ///@TODO add speed
        s->targetX = rand() % TILEMAP_WIDTH; s->targetY = rand() % TILEMAP_HEIGHT;
        ghostChooseBestDirection(s);
        goto skipAnimationUpdate;

    case goToHouse: //move to tile above monster house, when there switch to "cooldown" mode
        if( (s->tileX & 0xFFFE) == (s->targetX & 0xFFFE) && s->tileY == s->targetY){
            //move towards centre of door (slight offset from single tile)
            s->speed = 1;

            if(s->x < 13 * TILEDIMS + 3)
                s->xDir = 1;
            else if(s->x > 13 * TILEDIMS + 3)
                s->xDir = -1;

        }else{
            ghostChooseBestDirection(s); //still pathing to house
        }
        break;
    }

    //update animations
    if (s->xDir > 0) s->direction = 16;
    else if (s->xDir < 0) s->direction = 0;
    else if (s->yDir > 0) s->direction = 48;
    else if (s->yDir < 0) s->direction = 32;
skipAnimationUpdate:

    ghostPerFrame(s);
}

void updateGhosts()
{
 	int i;
	ghost* s;
    
	for(i = 0; i < 4; i++){
        	s = &ghosts[i]; //get current ghost to process
        	if(s->x & 7 || s->y  & 7){ //test if on a tile or between tiles
                	ghostPerFrame(s); //move in the current direction
        	}else{
                	ghostPerTile(s); //choose next tile to move to
        	}

    	}

}

//////////////////////////////////////////////////////////////////////////////////////////////////////////////
    ///ABSTRACT GAME FUNCTIONS
//////////////////////////////////////////////////////////////////////////////////////////////////////////////


int knaveformermain()
{

    /* game variables */

    int extraLives = 2;
    //timer for deciding when to switch between states
    int stateTimer;
    int cummulativeScore = 0;


    ghosts[pinky].dotThreshold = 60;
    ghosts[inky].dotThreshold  = 60 * 4;
    ghosts[clyde].dotThreshold = 60 * 10;

    while(1){

        /* basic game loop is:
     *  - process input
     *  - update actors
     *  - draw map (only really need to redraw a few tiles but easier to redraw the whole thing for now)
     *  - draw actors
     *  - scale (allows higher resolution, but is slower, unless we use SDL_Renderer)
     *  - update window surface
     *  - wait until next frame
     */



        switch (gameState){

        case ready: //show "ready" text for a few seconds + reset actors
            printf("\rready     ");
            resetPacman();
            resetGhosts();
            resetBoard();
            if(stateTimer > 60 * 2){ //wait 3 seconds in 'ready' state
                gameState = pause;
                stateTimer = 0; //reset
                break;
            }
            stateTimer++;
            break;

        case pause:
            if(stateTimer > 60){ //wait 3 seconds in 'ready' state
                gameState = playing;
                stateTimer = 0; //reset
                break;
            }
            stateTimer++;
            break;

        case playing: //normal gameplay
            if(currentGhostCooldownID < 4){
                ghosts[currentGhostCooldownID].dotCounter++;
            }

            vulnTimer--;

            updateGhosts();
            updatePacman();
            if(score == 244){ //244 pellets in a level
                stateTimer = 0;
                gameState = winnar;
                break;
            }
            break;

        case ghostLynched: //pause game + show gained score + play sound effect
            if(stateTimer > 20){ //wait 1/3 seconds in 'ready' state
                gameState = playing;
                stateTimer = 0; //reset
            }
            stateTimer++;
            break;

        case dead: //play animation, check if lives >= 0 -> respawn (reset actor locations)
            if(stateTimer > 36){
                stateTimer = 0;

                if(extraLives > 0){
                    stateTimer = 0;
                    resetPacman();
                    resetGhosts();
/// TEMP                    extraLives--;
                    gameState = pause;
                }else{
                    gameState = showScore;
                }

                break;
            }
            stateTimer++;
            break;

        case showScore: //no lives -> show score and go back to ready

            break;

        case winnar: //all pellets eaten, play sound effect
            if(stateTimer > 60)
                gameState = levelTransition;
            stateTimer++;
            break;

        case levelTransition: //reset board and go to ready. (keep score)
            stateTimer = 0;
            gameState = pause;
            cummulativeScore += score; //increase total score
            score = 0; //"score" is actually just a counter for number of pellets
            resetPacman();
            resetGhosts();
            resetBoard();
            break;
        }
    }
}

void resetBoard()
{
    int i;
    for(i = 0; i < TILEMAP_HEIGHT*TILEMAP_WIDTH; i++)
        tilemap[i] = tilemapData[i];
}

void resetGhosts()
{
    int i;
    //init ghosts
 /*  
    ghosts[blinky].subPixelX = 13.f * TILEDIMS + 3.f; ghosts[blinky].subPixelY = 11.f * TILEDIMS;
    ghosts[pinky].subPixelX  = 13.f * TILEDIMS + 3.f; ghosts[pinky].subPixelY  = 14.f * TILEDIMS; //make y = 14 to start them in the jail
    ghosts[inky].subPixelX   = 11.f * TILEDIMS + 3.f; ghosts[inky].subPixelY   = 14.f * TILEDIMS;
    ghosts[clyde].subPixelX  = 15.f * TILEDIMS + 3.f; ghosts[clyde].subPixelY  = 14.f * TILEDIMS;

    for(i = 0; i < 4; i++){
        ghosts[i].dotCounter = 0;
        ghosts[i].xDir = 0; ghosts[i].yDir = 0;
        ghosts[i].x = (int)ghosts[i].subPixelX; ghosts[i].y = (int)ghosts[i].subPixelY;
	ghosts[i].xOffset = 0; ghosts[i].yOffset = 0;
    }
    
    //target tile for scatter mode
    ghosts[blinky].homeX = TILEMAP_WIDTH; ghosts[blinky].homeY = 0;
    ghosts[pinky].homeX  = 0; ghosts[pinky].homeY  = 0;
    ghosts[inky].homeX   = TILEMAP_WIDTH; ghosts[inky].homeY   = TILEMAP_HEIGHT;
    ghosts[clyde].homeX  = 0; ghosts[clyde].homeY  = TILEMAP_HEIGHT;

    ghosts[blinky].xDir = -1;
    ghosts[blinky].yDir = 0;
    ghosts[blinky].mode = chase;

    ghosts[pinky].direction = 48;
    ghosts[inky].direction = 32;
    ghosts[clyde].direction = 32;

    ghosts[pinky].mode = cooldown;
    ghosts[inky].mode  = cooldown;
    ghosts[clyde].mode = cooldown;

    ghosts[blinky].speed = 1;
    ghosts[pinky].speed  = 0.8;
    ghosts[inky].speed   = 0.8;
    ghosts[clyde].speed  = 0.8;
  */ 
    return;
}

void resetPacman()
{
    //pacman's coords are from sprite centre
    sPacman.x = 14 * TILEDIMS;
    sPacman.y = 23 * TILEDIMS + 4;
    sPacman.xOffset = 0; sPacman.yOffset = 0;
    sPacman.tileX = 14;
    sPacman.tileY = 23;

    //start moving left
    sPacman.xDir = -1;
    sPacman.yDir = 0;
    sPacman.direction = 14;
}
