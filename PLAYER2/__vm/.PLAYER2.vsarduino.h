/* 
	Editor: https://www.visualmicro.com/
			visual micro and the arduino ide ignore this code during compilation. this code is automatically maintained by visualmicro, manual changes to this file will be overwritten
			the contents of the Visual Micro sketch sub folder can be deleted prior to publishing a project
			all non-arduino files created by visual micro and all visual studio project or solution files can be freely deleted and are not required to compile a sketch (do not delete your own code!).
			note: debugger breakpoints are stored in '.sln' or '.asln' files, knowledge of last uploaded breakpoints is stored in the upload.vmps.xml file. Both files are required to continue a previous debug session without needing to compile and upload again
	
	Hardware: Arduino/Genuino Uno, Platform=avr, Package=arduino
*/

#define __AVR_ATmega328p__
#define __AVR_ATmega328P__
#define ARDUINO 10805
#define ARDUINO_MAIN
#define F_CPU 16000000L
#define __AVR__
#define F_CPU 16000000L
#define ARDUINO 10805
#define ARDUINO_AVR_UNO
#define ARDUINO_ARCH_AVR
void setup(void);
void text_button(int x,int y, int z, String text);
void links();
void rechts();
void omhoog();
void omlaag();
void bomb();
void bombtime();
void map();
void wait (unsigned long howLong);
//
void p2links();
void p2rechts();
void p2omhoog();
void p2omlaag();
void p2bomb();
void p2bombneer();
uint8_t gridYFromByte(uint8_t input);
uint8_t gridXFromByte(uint8_t input);
uint8_t getByteFromGrid(uint8_t x,uint8_t y);

#include "pins_arduino.h" 
#include "arduino.h"
#include "PLAYER2.ino"
