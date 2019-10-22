/*
 * game.c
 *
 * Author: Peter Sutton
 *
 * Note that we use the display in a rotated orientation. The display should 
 * be oriented such that the "R/G LED MATRIX" words are in the correct
 * orientation. 
 * Display columns (0 to 15) are game rows 15 (top) to 0 (bottom) in this orientation.
 * Display rows (0 to 7) are game columns 0 (left) to 7 (right) in this orientation.
 */ 

#include "game.h"
#include "ledmatrix.h"
#include "pixel_colour.h"
#include "timer0.h"
#include "serialio.h"
#include "terminalio.h"
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>

///////////////////////////////// Global variables //////////////////////
// car_column stores the current position of the car. Game columns are numbered
// from 0 (left) to 7 (right). The car spans game rows 1 and 2.
static int8_t car_column;
#define CAR_START_ROW 1

// Boolean flag to indicate whether the car has crashed or not
static uint8_t car_crashed;

// Boolean flag to indicate whether the lap has finished or not
static uint8_t lap_finished;

// Background - 8 bits in each row. A 1 indicates background being
// present, a 0 is empty space. We will scroll through this and
// return to the other end. Bit 0 (LSB) in these patterns will end
// up on the left of the display (column 0).
#define NUM_GAME_ROWS 32	
uint8_t postions_py[]={2,3,6,1,5,0,2,3,2};
uint8_t postions_px[]={4,3,1,2,3,4,5,8,9};
static uint8_t background_data[NUM_GAME_ROWS] = {
		0b10000011,
		0b10000111,
		0b10000111,
		0b10000111,
		0b11000011,
		0b11100001,
		0b11110001,
		0b11110001,
		0b11100000,
		0b11100000,
		0b11100000,
		0b11000000,//0-11 level 1,2,3
		0b11000001,
		0b10000001,
		0b10000011,
		0b10000011,//15
		0b10000001,//16 level 4,5,6
		0b10000001,
		0b00010000,
		0b00011000,
		0b00011100,
		0b00111100,
		0b00111000,//22 level 7,8,9
		0b00111000,
		0b00010000,
		0b00010000,
		0b00000001,
		0b10000001,
		0b10000001,
		0b11000011,
		0b10000111,//15
		0b10000111
};
		
#define RACE_DISTANCE 127	// The effective row number where the finish line 
							// will appear

// Which row is at the bottom of the screen - starting at 0 and counting up 
// until we reach the finish line (defined by RACE_DISTANCE)
static uint8_t scroll_position;

// Colours
#define COLOUR_BACKGROUND	COLOUR_LIGHT_GREEN
#define COLOUR_CAR			COLOUR_LIGHT_ORANGE
#define COLOUR_CRASH		COLOUR_RED	
#define COLOUR_FINISH_LINE	COLOUR_YELLOW		/* Also the start line */


/////////////////////////////// Function Prototypes for Helper Functions ///////
// These functions are defined after the public functions. Comments are with the
// definitions.
static uint8_t car_crashes_at(uint8_t column);
void redraw_background();
static void redraw_game_row(uint8_t row);
static void draw_start_or_finish_line(uint8_t row);
static void redraw_car();
static void erase_car();
uint8_t activate_power();
uint8_t car_colour;	


/////////////////////////////// Public Functions ///////////////////////////////
// These functions are defined in the same order as declared in game.h

// Reset the game
void init_game(void) {
	// Initial scroll position
	scroll_position = 0;
	
	redraw_background();
	
	// Add a car to the display. (This will redraw the car.)
	put_car_at_start();
	
}



void draw_power_ups(){
	uint8_t colour= COLOUR_GREEN;
	switch(random()%4) {
			case 0: colour =(COLOUR_LIGHT_ORANGE); break;
			case 1: colour=(COLOUR_RED); break;
			case 2: colour=(COLOUR_YELLOW); break;
			case 3: colour=(COLOUR_GREEN); break;
		}
		if(setting&&(postionx<=15)){
			ledmatrix_update_pixel(postionx, postiony, colour);
		}
}



uint32_t start_time, passed_time;
int state = 0;
	

uint8_t activate_power(){
	uint8_t column =car_column;
	if (setting){
	if((postionx==14)||(postionx==13)){
		if(postiony==column){
			state = 1;
			setting=0;
			postionx=16;
			start_time = get_clock_ticks();
		}		
	}
	}
	if(state){
		passed_time = get_clock_ticks();
		if(passed_time-start_time>6000){
			state=0;
			}	
	}
	
	return state;
}


// Add a car to the game. It is assumed that placing the car in column 3
// will NOT overlap the background.
void put_car_at_start(void) {
	// Initial starting position of car. It must be guaranteed that this
	// initial position does not clash with the background.
	car_column = 3;
	
	// Car is initially alive and hasn't finished
	car_crashed = 0;
	lap_finished = 0;
	
	// Show the car
	redraw_car();
}

void move_car_left(void) {
	if(car_column != 0) {
		// Car not at left hand side
		erase_car();
		car_column--;
		
		car_crashed = car_crashes_at(car_column);
		redraw_car();
	} // else car is at left hand side (column 0) and can't move left
}

void move_car_right(void) {
	if(car_column != 7) {
		// Car not at right hand side
		erase_car();
		car_column++;
		
		car_crashed = car_crashes_at(car_column);
		redraw_car();
		//else car is at right hand side (column 7) and can't move right
	
	}
}

uint8_t get_car_column(void) {
	return car_column;
}

uint8_t has_car_crashed(void) {
	
	return car_crashed;
}

uint8_t has_lap_finished(void) {
	// If the top pixel in the car (two ahead of the scroll position) has
	// reached the finish row then we have crossed the line
	return lap_finished;
}

void scroll_background(void) {
	scroll_position++;
	
	// Check if the lap has finished. We add 2 to the scroll position
	// because we're looking at the front of the car. 
	if(scroll_position + 2 == RACE_DISTANCE) {
		lap_finished = 1;	
	} else {
		// If we haven't finished the lap, then 
		// check whether the car has crashed or not in its current column
		// (The background may have scrolled into it.)
		car_crashed = car_crashes_at(car_column);
	}
	
	// For speed purposes, we don't redraw the whole display, we
	// erase the car, shift the display down (right in the sense of the 
	// LED matrix) and redraw the car and draw the new row 15
	
	
	

	if (postionx<15){
		ledmatrix_update_pixel(postionx, postiony, COLOUR_BLACK);
	}
	
	erase_car();
	ledmatrix_shift_display_right();
	postionx++;
	redraw_car();
	redraw_game_row(15);
	}
	

		
/////////////////////////////// Private (Helper) Functions /////////////////////

// Return 1 if the car crashes if moved into the given column. We compare the car
// position with the background in rows 1 and 2
static uint8_t car_crashes_at(uint8_t column) {
	// Check row 1 at this column
	uint8_t background_row_number = (1 + scroll_position) % NUM_GAME_ROWS;
	uint8_t background_row_data = background_data[background_row_number];
	if(background_row_data & (1<< column)) {
		// Collision between car and background in row 1
		if (activate_power()){
		
			return 0;
			
		} else {
			return 1;
			}
		}
	// Check row 2
	background_row_number = (2 + scroll_position) % NUM_GAME_ROWS;
	background_row_data = background_data[background_row_number];
	if(background_row_data & (1<< column)) {
		// Collision between car and background in row 2
		if (activate_power()){
			return 0;
			} else {
			return 1;
		}
	}
	// No collision 
	return 0;
}

// Clear the screen and redraw the background. The car is not redrawn.
void redraw_background() {
	// Clear the display
	ledmatrix_clear();
	
	uint8_t row;
	for(row = 0; row <= 15; row++) {
		redraw_game_row(row);
	}
}


// Redraw the row with the given number (0 to 15). The car is not redrawn.
// ROWS in the game are COLUMS in the terminology of the display
static void redraw_game_row(uint8_t row) {	
	MatrixColumn row_display_data; // Note that a game row corresponds to a display
								   // column.
	uint8_t i;
	uint8_t race_row = scroll_position + row;
	if(race_row == 0 || race_row == RACE_DISTANCE) {
		draw_start_or_finish_line(row);
	} else {
		uint8_t background_row_data = background_data[race_row % NUM_GAME_ROWS];
		for(i=0;i<=7;i++) {
			if(background_row_data & (1<<i)) {
				// Bit i is set, meaning background is present
				row_display_data[i] = COLOUR_BACKGROUND;
			} else {
				
				row_display_data[i] = COLOUR_BLACK;
			}
			
		}
		ledmatrix_update_column(15 - row, row_display_data);
		
	}
}


static void draw_start_or_finish_line(uint8_t row) {
	// Draw a solid line at the given game display row (0 to 15)
	MatrixColumn row_display_data;
	uint8_t i;
	for(i=0;i<=7;i++) {
		row_display_data[i] = COLOUR_FINISH_LINE;
	}
	ledmatrix_update_column(15 - row, row_display_data);
	
}

// Redraw the car in its current position.


static void redraw_car(void) {
	uint8_t car_colour = COLOUR_CAR;
	if(car_crashed) {
		car_colour = COLOUR_CRASH;
	}
	if(activate_power()){
		if(passed_time-start_time<4000){
			car_colour = COLOUR_YELLOW;
			
		}
		
	}
		ledmatrix_update_pixel(15 - CAR_START_ROW, car_column, car_colour);
		ledmatrix_update_pixel(15 - (CAR_START_ROW+1), car_column, car_colour);

}

// Erase the car (we assume it hasn't crashed - so we just replace
// the car position with black)
static void erase_car(void) {
	ledmatrix_update_pixel(15 - CAR_START_ROW, car_column, COLOUR_BLACK);
	ledmatrix_update_pixel(15 - (CAR_START_ROW+1), car_column, COLOUR_BLACK);
}

void Car_Powered(){
	if(activate_power()){
		if(passed_time-start_time>4000){
			switch(random()%2) {
				case 0: car_colour =(COLOUR_YELLOW); break;
				case 1: car_colour=(COLOUR_CAR); break;
			}
		ledmatrix_update_pixel(15 - CAR_START_ROW, car_column, car_colour);
		ledmatrix_update_pixel(15 - (CAR_START_ROW+1), car_column, car_colour);
		}
	}
	
}