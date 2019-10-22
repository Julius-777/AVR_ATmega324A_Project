# RallyRacer AVR Project

## Introduction
The program is a vertical scrolling racing game (dubbed RallyRacer). TheAVRATmega324Amicrocontrollerruns the program and receives input from a number of sourcesand outputs a display to a LED display  board,  with  additional  information  being  output  to aserial  terminaland,  to  be implemented as part of this project, a seven segment displayand other LEDs.

## Program Description
    •project.c–this is the main file that contains the event loopand examples of how time-based events are implemented.You should read and understand this file.
    •game.h/game.c–game.c contains the implementation of the operationsin the game(e.g. car movementand background scrolling). You should read this file and understand what representation is used for the game. You will need to modify this file to allow the carto move in other directions.
    •buttons.h/buttons.c –this contains  the  code  which  deals  with  the  IO  board  push buttons. It sets up pin change interrupts on those pins and records rising edges (buttons being pushed).
    •ledmatrix.h/ledmatrix.c–this  contains functions  which  give  easier  access  to  the services provided  by  the  LED  matrix.  It  makes  use  of  the  SPI  routines  implemented  in spi.c.
    •pixel_colour.h–this file contains definitions of some useful colours.•score.h/score.c–a module for keeping track of and adding to the score. This module is not used in the provided code.
    •scrolling_char_display.h/scrolling_char_display.c–this contains code which provides a scrolling message display on the LED matrix board. 
    •serialio.h/serialio.c–this fileis  responsible  for  handling  serial  input  and  output  using interrupts.  It  alsomaps  the C standard  IO  routines  (e.g. printf()and fgetc())  to use the serial interfaceso you are able to use printf()etc.for debugging purposes if you wish. You should not need to look in this file, but you may be interested in how it works and the buffer sizes used for input and output (and what happens when the buffers fill up). 
    •spi.h/spi.c–this module encapsulates  all  SPI  communication.  Note  that  by  default,  all SPI communication uses busy waiting –the “send” routine returns only whenthe data is sent.If  you  need  the  CPU  cycles  for  other  activities,  you  may  wish  to  consider converting this to interrupt basedIO, similar to way serial IO is handled
    .•terminalio.h/terminalio.c–this  encapsulates  the  sending  of  various  escape  sequenceswhich  enable  some  control  over  terminal  appearance  and  text  placement–you  can  call these   functions   (declared   in   terminalio.h)   instead   of   remembering   various   escape sequences.Additional   information   about   terminal   IO is   available on   the   course Blackboard site
    •timer0.h/timer0.c –sets  up  a  timer  that  is  used  to  generate  an  interrupt  every millisecond and update a global time value.

## Initial Operation
The provided template program responds to the following inputs:
    •Rising edge on the button connected to pin B3
    •Serial input character ‘L’ or ‘l’(lower case L)
    •Serial input escape sequence corresponding to the cursor-leftkeyAll of these move thecarleft by one column. Code is present to detect the following, but no actions are taken on these inputs:
    •Rising edge on the buttonconnected to B0 (intended to be move right);
    •Serial input characters ‘r’, ‘R’ (alsomove right);
    •Serial input escape sequences corresponding to the cursor rightkey(also move right).
    •Serial input characters ‘p’ and ‘P’(intended to be the pause/unpause key)

## Added Features
Splash screen
Movecar right
Scoring
Multiple Lives
Acceleration/Dec
Lap Timing
New Game
Random Positions
High Score
Game Pause
Power-up
Level on 7Seg
Diff Game 

## Built With

* C programming for the AVR
* The Atmel Studio environment.
* AVRATmega324Amicrocontrollerruns the program and receives input from a number of sourcesand outputs a display to a LED display  board,  with  additional  information  being  output  to aserial  terminaland,  to  be implemented as part of this project, a seven segment displayand other LEDs.

## Authors

* Julius Miyumo  

## Acknowledgments

* [Dr Peter Sutton](https://uqreview.com/lecturers/peter-sutton/) who was the professor who taught the Networking part of this course [CSSE2010](http://uqreview.com/courses/csse2310/)
* Template code provide contained a version of RallyRacerprovided that implemented basic functionality –the background scrolls  but  the  car  can  only  move  in  one  direction  (left)  and the  game  ends  immediately  when the  car  crashes.You  can  add  features  such  as  scoring, moving  right, acceleration/deceleration, different  levels  of  play, sound  effects, etc. 