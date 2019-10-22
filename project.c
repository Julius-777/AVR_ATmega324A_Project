/*
 * RallyProject.c
 *
 * Main file
 *
 * Author: Peter Sutton. Modified by <YOUR NAME HERE>
 */ 

#include <avr/io.h>
#include <avr/interrupt.h>
#include <avr/pgmspace.h>
#include <stdio.h>
#include <stdlib.h>		// For random()

#include "ledmatrix.h"
#include "scrolling_char_display.h"
#include "buttons.h"
#include "serialio.h"
#include "terminalio.h"
#include "score.h"
#include "timer0.h"
#include "game.h"

#define F_CPU 8000000L
#include <util/delay.h>

// Function prototypes - these are defined below (after main()) in the order
// given here
void initialise_hardware(void);
void splash_screen(void);
void new_game(void);
void play_game(void);
void handle_game_over(void);
void handle_new_lap(void);
void handle_lives (void);
void crash_handle(void);
int number_lives; 
int level_speed;
int accel;
int lap_time;
int real_time;
int decimal_time;
void init_number_lives(void){
	number_lives = 3;
}
void init_accel(void) {
	accel = 600;
}
void init_level_speed(void){
	level_speed = 600;
}
void init_lap_time(void) {
	real_time = get_clock_ticks();
	lap_time = 0;
	decimal_time=0;

}



// ASCII code for Escape character
#define ESCAPE_CHAR 27

/////////////////////////////// main //////////////////////////////////
int main(void) {
	// Setup hardware and call backs. This will turn on 
	// interrupts.
	initialise_hardware();
	
	// Show the splash screen message. Returns when display
	// is complete
	splash_screen();
	new_game();
	while(1) {
		//handle_lives();
		play_game();
		crash_handle();
		handle_game_over();
	}
}

void initialise_hardware(void) {
	ledmatrix_setup();
	init_button_interrupts();
	
	// Setup serial port for 19200 baud communication with no echo
	// of incoming characters
	init_serial_stdio(19200,0);
	
	init_timer0();
	
	// Turn on global interrupts
	sei();
}

void splash_screen(void) {
	// Reset display attributes and clear terminal screen then output a message
	set_display_attribute(TERM_RESET);
	clear_terminal();
	
	hide_cursor();	// We don't need to see the cursor when we're just doing output
	move_cursor(3,3);
	printf_P(PSTR("RallyRacer"));
	
	move_cursor(3,5);
	set_display_attribute(FG_GREEN);	// Make the text green
	printf_P(PSTR("CSSE2010/7201 project by Julius Miyumo"));	
	set_display_attribute(FG_WHITE);	// Return to default colour (White)
	
	// Output the scrolling message to the LED matrix
	// and wait for a push button to be pushed.
	ledmatrix_clear();
	
	// Orange message the first time through
	set_text_colour(COLOUR_ORANGE);
	while(1) {
		set_scrolling_display_text("RALLYRACER by 43588701");
		// Scroll the message until it has scrolled off the 
		// display or a button is pushed. We pause for 130ms between each scroll.
		while(scroll_display()) {
			_delay_ms(100);
			if(button_pushed() != -1) {
				return;
			}
		}
		// Message has scrolled off the display. Change colour
		// to a random colour and scroll again.
		switch(random()%4) {
			case 0: set_text_colour(COLOUR_LIGHT_ORANGE); break;
			case 1: set_text_colour(COLOUR_RED); break;
			case 2: set_text_colour(COLOUR_YELLOW); break;
			case 3: set_text_colour(COLOUR_GREEN); break;
		}
	}
}

int level=1;
void display_level(){
	char lev_dis[40];
	sprintf(lev_dis, "Level %d", level);
	reset_display();
	ledmatrix_clear();
	if (level<=9){
		set_scrolling_display_text(lev_dis);
	}
	else{
		set_scrolling_display_text("You Win");
	}
	
	while(scroll_display()){
		_delay_ms(60);
	}
}
void new_game(void) {
	level =1;
	display_level();
	// Initialise the game and display
	init_game();
	init_accel();
	init_lap_time();
	init_level_speed();
	init_number_lives();
	handle_lives();
	set_power();
	// Clear the serial terminal
	clear_terminal();
	
	// Initialise the score
	init_score();
	
	// Clear a button push or serial input if any are waiting
	// (The cast to void means the return value is ignored.)
	(void)button_pushed();
	clear_serial_input_buffer();
	
	setting = 1;
	
	// Delay for half a second
	_delay_ms(500);
}
	

void Display_Info() {
	move_cursor(3,1);
	printf("Level: %d", level); //Level
	move_cursor(3,2);
	printf("Lives: %d", number_lives); //life
	move_cursor(3,3);
	printf("Game Score: %d", (int) get_score()); //score
	move_cursor(3,4);
	printf("Lap Time: %d.%d",  (int) lap_time, (int) decimal_time); //time
}
// LED LIGHTS

void handle_lives (void) {
	// Set the pin 0 at port A as output
	DDRA |= (1<<0)|(1<<1)|(1<<2)|(1<<3);
	if (has_car_crashed()&& number_lives != 0) {
		number_lives-- ;
		
	}
		if(number_lives==4){
			PORTA |= ((1<<PA3)|(1<<PA2)|(1<<PA1)|(1<<PA0));
		}
		else if (number_lives == 3) {
			PORTA &= ~((1<<PA3)|(1<<PA2)|(1<<PA1)|(1<<PA0));
			PORTA |= ((1<<PA2)|(1<<PA1)|(1<<PA0));
		}
		else if (number_lives ==2) {
			PORTA &= ~((1<<PA2)|(1<<PA1)|(1<<PA0));
			PORTA |= ((1<<PA1)|(1<<PA0));
		}
		else if (number_lives ==1) {
			PORTA &= ~((1<<PA2)|(1<<PA1)|(1<<PA0));
			PORTA |= (1<<PA0);
		}
		else if(number_lives ==0) {
			PORTA &= ~((1<<PA2)|(1<<PA1)|(1<<PA0));
		}

		Display_Info();

}


void play_game(void) {
	uint8_t paused = 0;
	uint32_t current_time, last_move_time;
	int8_t button;
	char serial_input, escape_sequence_char;
	uint8_t characters_into_escape_sequence = 0;
	
	// Get the current time and remember this as the last time the background scrolled.
	current_time = get_clock_ticks();
	last_move_time = current_time;
	
	uint16_t car_moves = 0;
	// We play the game while the car hasn't crashed
		while(!has_car_crashed()) {
		
		// Check for input - which could be a button push or serial input.
		// Serial input may be part of an escape sequence, e.g. ESC [ D
		// is a left cursor key press. We will be processing each character
		// independently and can't do anything until we get the third character.
		// At most one of the following three variables will be set to a value
		// other than -1 if input is available.
		// (We don't initalise button to -1 since button_pushed() will return -1
		// if no button pushes are waiting to be returned.)
		// Button pushes take priority over serial input. If there are both then
		// we'll retrieve the serial input the next time through this loop
		serial_input = -1;
		escape_sequence_char = -1;
		button = button_pushed();
		
		if(button == -1) {
			// No push button was pushed, see if there is any serial input
			if(serial_input_available()) {
				// Serial data was available - read the data from standard input
				serial_input = fgetc(stdin);
				// Check if the character is part of an escape sequence
				if(characters_into_escape_sequence == 0 && serial_input == ESCAPE_CHAR) {
					// We've hit the first character in an escape sequence (escape)
					characters_into_escape_sequence++;
					serial_input = -1; // Don't further process this character
					} else if(characters_into_escape_sequence == 1 && serial_input == '[') {
					// We've hit the second character in an escape sequence
					characters_into_escape_sequence++;
					serial_input = -1; // Don't further process this character
					} else if(characters_into_escape_sequence == 2) {
					// Third (and last) character in the escape sequence
					escape_sequence_char = serial_input;
					serial_input = -1;  // Don't further process this character - we
					// deal with it as part of the escape sequence
					characters_into_escape_sequence = 0;
					} else {
					// Character was not part of an escape sequence (or we received
					// an invalid second character in the sequence). We'll process
					// the data in the serial_input variable.
					characters_into_escape_sequence = 0;
				}
			}
		}
		
		if(serial_input=='N' || serial_input=='n') {
				new_game();	// new game
		} else if(serial_input=='P' || serial_input=='p') {
			// pause or unpause
			if(paused) {
				
				// unpause
				paused = 0;
				TCCR0B=((1<<CS01)|(1<<CS00));
				
			} else {
				// pause
				
				paused = 1;
				TCCR0B=0;
				
			}
			
			continue;
		} else if(paused) {
			
			continue;
			
		} 
		
		else if(button==3 || escape_sequence_char=='D' || serial_input=='L' || serial_input=='l') {
			// Attempt to move left
			move_car_left();
			car_moves++;
		} else if(button==0 || escape_sequence_char=='C' || serial_input=='R' || serial_input=='r') {
			// Attempt to move right
			move_car_right();
			car_moves++;
		}
			
			
		else if ((button==1)&&(accel<=1000)){
			accel+=100;
		}
		else if((button==2)&&(accel>=100)){
			accel-=100;//if B2 pushed if scrolls faster till 100ms
		}
		// else - invalid input or we're part way through an escape sequence -
		// do nothing
		
		lap_time = ((get_clock_ticks()-real_time)/1000);
		decimal_time = (((get_clock_ticks()-real_time)%1000)/100); // lap time
		Display_Info();
		current_time = get_clock_ticks(); 
		if(current_time >= last_move_time + 5){
			draw_power_ups();
			Car_Powered();
		}
			
		current_time = get_clock_ticks(); 
		

		
		if(!has_car_crashed() && current_time >= last_move_time + accel) {
			// 600ms (0.6 second) has passed since the last time we scrolled
			// the background, so scroll it now and check whether that means
			// we've finished the lap. (If a crash occurs we will drop out of
			// the main while loop so we don't need to check for that here.)
			scroll_background();
			
			//Add to score
			if  (car_moves<= 5){
				add_to_score (5-car_moves);	
			}car_moves=0;

			if(has_lap_finished()) {
				add_to_score(100);
				init_accel();
				handle_new_lap();
				Display_Info();
				(void)button_pushed();
				clear_serial_input_buffer();
				// Pauses until a button is pushed
				// Reset the time of the last scroll
				last_move_time = get_clock_ticks();
				} else {
				last_move_time = current_time;
			}
		}
		// If we get here the car has crashed.
			handle_lives();
			if ((number_lives>=1)&& (has_car_crashed())) {
				move_cursor(10,14);
				printf_P(PSTR("You crashed     "));
				move_cursor(10,15);
				printf_P(PSTR("Press a button to continue   "));
				TCCR0B=0;
				while(1){
					if(button_pushed() != -1) {
						TCCR0B=((1<<CS01)|(1<<CS00));
						return;
					}

				}
				init_accel();
				accel = level_speed;
				Display_Info();
			}
				
				// Clear a button push or serial input if any are waiting
				// (The cast to void means the return value is ignored.)
		
				// Delay for half a second
		
			}
}
			
void crash_handle(){
	if (number_lives>0){
		init_game();
		init_accel();
		accel = level_speed;
		// Clear the crash message 
		move_cursor(10,14);
		printf_P(PSTR("                      "));
		move_cursor(10,15);
		printf_P(PSTR("                            "));
		// Clear a button push or serial input if any are waiting
		// (The cast to void means the return value is ignored.)
		(void)button_pushed();
		clear_serial_input_buffer();
		
		// Delay for half a second
		_delay_ms(500);
	}
	set_power();
}



void handle_game_over() {
	if (number_lives==0){
		
	// Print a message to the terminal. The spaces on the end of the message
	// will ensure the "LAP COMPLETE" message is completely overwritten.
	move_cursor(10,14);
	printf_P(PSTR("GAME OVER   "));
	move_cursor(10,15);
	printf_P(PSTR("Press a button to start again"));
	Display_Info();
	while(button_pushed() == -1) {
		; // wait until a button has been pushed
		}
	}
		
}
void set_power(){
	uint8_t postions_py[]={2,4,4,2,3,4,2,3,4};
	uint8_t postions_px[]={2,4,5,2,3,3,3,3,3};
	postionx = postions_px[level-1];
	postiony = postions_py[level-1];
	
}

void handle_new_lap() {
	setting = 1;
	level++;
	if(level<9) {
	display_level();
	init_accel();
	level_speed -=200;
	accel =level_speed;
	
	if (number_lives<4){
		number_lives++;
	}
	
	move_cursor(10,14);
	printf_P(PSTR("LAP COMPLETE"));
	move_cursor(10,15);
	printf_P(PSTR("Press a button to continue"));
	
	
	init_game(); // This will need to be changed for multiple lives
	move_cursor(10,14);
	printf_P(PSTR("            "));
	move_cursor(10,15);
	printf_P(PSTR("                             "));
	_delay_ms(1000);
	init_lap_time();
	set_power();
	} else{
		display_level();
		new_game();
	}
	
	}
	
	
