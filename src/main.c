// uint8_t for x and y coordinates
// keep global variables of character

#include <rp6502.h>
#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include <stdint.h>

// 23-Feb-2024 PJF
// This dungeon navigator is based on an M68K mac game (1985ish) named Dungeon of Doom
// For the full retro effect, we'll use binary graphics.

// screen size in pixels
#define SC_WIDTH 320
#define SC_HEIGHT 180


// tile size in pixels
#define T_WIDTH 8
#define T_HEIGHT 8

// screen size in tiles
#define SC_T_WIDTH (SC_WIDTH/T_WIDTH)
#define SC_T_HEIGHT (SC_HEIGHT/T_HEIGHT)

// Coordinates for character
uint8_t CHARX;
uint8_t CHARY;
uint8_t CharHitpoints;

// Coordinates for adversary
uint8_t ADVX;
uint8_t ADVY;
uint8_t AdvHitpoints;

// handy macros for the rather lazy
#define A0 (RIA.addr0)
#define R0 (RIA.rw0)
#define S0 (RIA.step0)

#define A1 (RIA.addr1)
#define R1 (RIA.rw1)
#define S1 (RIA.step1)

// even handier macros for the extremely lazy
#define A A0
#define R R0
#define S S0

#define V (RIA.vsync)

// the board's address and the tile data's address.
#define BOARD_BASE 0x0000

#define TILE_DATA_BASE (BOARD_BASE + SC_T_WIDTH*SC_T_HEIGHT)

// TODO: make the tiles assets and load them directly

// a tile is an 8x8 array of bytes
typedef struct _tile {
    uint8_t data[64];
    } tile;

// there can be 256 different tiles in use
typedef uint8_t tile_id;

// TODO: make the initial board contents an asset
// (since we need a 6502-memory copy of the board (?? do we?),
// make sure to copy it out)

// a board is a SC_T_WIDTH x SC_T_HEIGHT array of tile_ids (uint8_ts)
typedef struct _board {
    tile_id data[SC_T_WIDTH*SC_T_HEIGHT];
    } board;

board b;

#define B(i,j) (b.data[(i)*SC_T_WIDTH+(j)]) 

// initialize tiles
// number of tiles must be >=2
// tile 0 is all-0 (black, using standard pallette)
// tile 1 is all-1 (white, using standard pallette)
// for now,all tiles nubered 2 and higher are randomly initialized.

// tiles are numbered from 0 to tidmax --> inclusive <--
// 
tile *create_tiles(uint8_t tidmax) {
    tile *t;
    uint8_t i,j;
    uint8_t tsz = sizeof(t->data);
    if (tidmax < 3) {
        printf("The game can't operate with less than two tiles.\n");
        exit(0);
        }
    t = (tile *)malloc((unsigned int)(tidmax+1)*sizeof(tile));

    if (t == NULL) {
        printf("malloc() failed.\n");
        exit(0);
        }

    // tile 0 and 1
    // no bzero or memset, so...
    for(j=0;j<tsz;j++) {
        t[0].data[j] = 0; //balck
        t[1].data[j] = 0xff; //white   make 2 more
        }

    // all the other tiles are random

    i=2;
    do {
	for (j=0; j<tsz; j++)
		t[i].data[j] = rand() & 0xff; ///////////////////////////  & limits the random from 1-255
    	} while (i++ < tidmax);

    return t;
}                  /////// don't need this
  
void clobber_tiles(tile *t) {
    free(t);
}

void init_graphics(tile *tiles, uint8_t tidmax) {
    uint8_t i,j;
    // initialize struct before setting mode
    xram0_struct_set(0xFF00, vga_mode2_config_t, x_wrap, false);
    xram0_struct_set(0xFF00, vga_mode2_config_t, y_wrap, false);
    xram0_struct_set(0xFF00, vga_mode2_config_t, x_pos_px, 0);
    xram0_struct_set(0xFF00, vga_mode2_config_t, y_pos_px, 0);
    xram0_struct_set(0xFF00, vga_mode2_config_t, width_tiles, SC_T_WIDTH);
    xram0_struct_set(0xFF00, vga_mode2_config_t, height_tiles, SC_T_HEIGHT);
    xram0_struct_set(0xFF00, vga_mode2_config_t, xram_data_ptr, BOARD_BASE);
    xram0_struct_set(0xFF00, vga_mode2_config_t, xram_palette_ptr, 0xFFFF); // special w/b
    xram0_struct_set(0xFF00, vga_mode2_config_t, xram_tile_ptr, TILE_DATA_BASE);

    // install tile data
    A = TILE_DATA_BASE;
    S = 1;
    i=0;
    do {
        for(j=0; j<sizeof(tiles[i].data); j++)
            R = tiles[i].data[j]; 
        }
    while (i++ < tidmax);

    // 320x180; for 8x8 tiles; 40x22 with 4 pix @ bottom
    xreg_vga_canvas(2);
    xreg_vga_mode(2,3,0xff00);
    // xreg_vga_mode(0,1); // Turn on extra layer for print statements

    return;
}


void init_board(uint8_t tidmax) {
    uint8_t i;
    uint16_t j;

    // black board
    // for(i=0;i<SC_T_HEIGHT;i++)
	//    for(j=0; j<SC_T_WIDTH; j++)
    //    	B(i,j) = 0;         // = (i+j) & 0xff;
    // return;

    // for(i=0;i<SC_T_HEIGHT;i++)
    //    for(j=0; j<SC_T_WIDTH; j++)
    //        B(i,j) = (i+j) & 0xff;

    for(i=0;i<SC_T_HEIGHT;i++)
        B(i,0) = 1;
    for(i=0;i<SC_T_HEIGHT;i++)
        B(i,SC_T_WIDTH-1) = 1;
    for(j=0;j<SC_T_WIDTH;j++)
        B(0,j) = 1;
    for(j=0;j<SC_T_WIDTH;j++)
        B(SC_T_HEIGHT-1,j) = 1;
    
   
    return;
}           ////// create black background then white fence then define values for character's coordinates
 
void draw_character(uint8_t color){
    B(CHARY,CHARX) = color;
    return;
}

void draw_adversary(uint8_t color){
    B(ADVY,ADVX) = color;
    return;
}

uint8_t check_char_hitpoints(){
    return CharHitpoints;
}

uint8_t check_adv_hitpoints(){
    return AdvHitpoints;
}


void right() {
    uint8_t i,j;
    //for(i=0;i<SC_T_HEIGHT;i++)
    //	for(j=0;j<SC_T_WIDTH-1;j++)
	//    B(i,j) = B(i,j+1);

    //for(i=0;i<SC_T_HEIGHT;i++)
	//B(i,SC_T_WIDTH-1) = rand()&0x01 ? 1 : 1;    // The rand()&0x01 changes the occurence of white tiles (must be binary) can only be 0x01 or 0x11
      //                                  ^ this number set at 1 makes screen all white
    if (CHARX < SC_T_WIDTH-2 && (CHARX != ADVX-1 || CHARY !=ADVY)){
        draw_character(0);
        CHARX += 1;
        draw_character(4);
    }
    if (CHARX == ADVX-1 && CHARY == ADVY){
        if (AdvHitpoints != 0 && CharHitpoints != 0){
            AdvHitpoints -= 1;
            CharHitpoints -=1;
        }
        else if (AdvHitpoints == 0){
            draw_adversary(0);
        }
        else if (CharHitpoints == 0){
            draw_character(0);
        }
    }
}

void left() {
    uint8_t i,j;
    //for (i=0; i<SC_T_HEIGHT; i++)
	//    for(j=SC_T_WIDTH-1; j>=1; --j) 
	//	    B(i,j) = B(i,j-1);
    //for(i=0;i<SC_T_HEIGHT;i++)
	//B(i,0) = rand()&0x01 ? 1 : 1;                ///// in right and left change everything. Juts change the coordinates of the character and redraw board
    if (CHARX > 1 && (CHARX != ADVX+1 || CHARY !=ADVY)){
        draw_character(0);
        CHARX -= 1;
        draw_character(4);
    }
    if (CHARX == ADVX+1 && CHARY == ADVY){
        if (AdvHitpoints != 0 && CharHitpoints != 0){
            AdvHitpoints -= 1;
            CharHitpoints -=1;
        }
        else if (AdvHitpoints == 0){
            draw_adversary(0);
        }
        else if (CharHitpoints == 0){
            draw_character(0);
        }
    }
    
}

void up() {
    uint8_t i,j;

    if (CHARY > 1 && (CHARY !=ADVY+1 || CHARX != ADVX)) {
        draw_character(0);
        CHARY -= 1;
        draw_character(4);
    }
    if (CHARY == ADVY+1 && CHARX == ADVX){
        if (AdvHitpoints != 0 && CharHitpoints != 0){
            AdvHitpoints -= 1;
            CharHitpoints -=1;
        }
        else if (AdvHitpoints == 0){
            draw_adversary(0);
        }
        else if (CharHitpoints == 0){
            draw_character(0);
        }
    }

}

void down() {
    uint8_t i,j;
    if (CHARY < SC_T_HEIGHT-2 && (CHARY != ADVY-1 || CHARX != ADVX)){
        draw_character(0);
        CHARY += 1;
        draw_character(4);
    }
    if (CHARY == ADVY-1 && CHARX == ADVX){
        if (AdvHitpoints != 0 && CharHitpoints != 0){
            AdvHitpoints -= 1;
            CharHitpoints -=1;
        }
        else if (AdvHitpoints == 0){
            draw_adversary(0);
        }
        else if (CharHitpoints == 0){
            draw_character(0);
        }
    }
    
}
	 
void draw_board() {
    uint8_t i,j;
    uint8_t *bptr = &B(0,0);
    A = BOARD_BASE;
    S = 1;
    for(i=0;i<SC_T_HEIGHT;i++){
        for(j=0;j<SC_T_WIDTH;j+=10) {
            if (i == ADVX || j == ADVY){   // doesn't draw black spot at the adversaries location
                continue;
            }
            else 
            {
                R = *bptr++;
                R = *bptr++;
                R = *bptr++;
                R = *bptr++;
                R = *bptr++;
                R = *bptr++;
                R = *bptr++;
                R = *bptr++;
                R = *bptr++;
                R = *bptr++;
            }
        }
    }
    return;
}

#define NTWEAKS 16
void tweak_board(uint8_t tidmax) {
    uint8_t i,j,k,tid;
    for(k=0; k<NTWEAKS; k++) {
        i = rand() % SC_T_HEIGHT;
        j = rand() % SC_T_WIDTH;
        tid = rand() % (tidmax+1);
        B(i,j) = rand()&0x01 ? 1 : 0;
    }
}

// keyboard uses
// 32 bytes immediately before 0xFF00, config struct for graphics.
#define KEYBOARD_BASE 0xFEE0

void enable_raw_keyboard() {
    xreg(0, 0, 0x00, KEYBOARD_BASE); // enable
}

void disable_raw_keyboard() {
    xreg(0, 0, 0x00, 0xFFFF); // disable
}

uint8_t key_down(uint8_t c) {
	uint8_t res;
	A = KEYBOARD_BASE + (c>>3);
	res = R & (1<<(c&0x07));
	printf("%c down? %d\n",c,res);
	return res;
}

#define TRUE (1==1)
#define FALSE (1==0)


char s[256];

void main() {
    uint8_t tidmax = 255;
    uint8_t board_changed = TRUE;
    uint8_t cnt = 0;
    tile *t;

    _randomize();

    
    CHARX = 5;
    CHARY = 3;
    ADVX = 19; //rand() % SC_T_HEIGHT-5 + 5;
    ADVY = 11; //rand() % SC_T_WIDTH-5 + 5;
    CharHitpoints = rand() % 10 + 1;
    AdvHitpoints = rand() % 10 + 1;

    init_board(tidmax);
    draw_character(4);
    draw_adversary(1);
    t = create_tiles(tidmax);
    init_graphics(t,tidmax); 
    clobber_tiles(t);

    enable_raw_keyboard();

    // printf("Hello");        // print 30 blank lines to clear all the junk words


    //tweak_board(tidmax);

    stdin_opt(0,2);

    while (1==1) {
        if (board_changed) {
            board_changed = FALSE;
	    // side-to-side
	    //if ((cnt++ & 0x20) == 0)
		    //right();
	    //else
		    //left();
		    //
	    //if (key_down('a')) left();
	    //else if (key_down('d')) right();
	    
	    gets(s);
	    if ((s[0] == 'a') || (s[0] == 'A')) left();
	    else if ((s[0] == 'd') || (s[0] == 'D')) right();  ////// add w and s for up and down
        else if ((s[0] == 'w') || (s[0] == 'W')) up(); 
        else if ((s[0] == 's') || (s[0] == 'S')) down(); 

            draw_board();
            board_changed = TRUE;
            }
        }
    disable_raw_keyboard();
}

