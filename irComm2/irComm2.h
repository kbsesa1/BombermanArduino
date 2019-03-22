// irComm2.h

#ifndef _IRCOMM_h
#define _IRCOMM_h

#if defined(ARDUINO) && ARDUINO >= 100
	#include "arduino.h"
#else
	#include "WProgram.h"
#endif
class IR
{
public:
IR(bool highFrequency);
void begin();
void run();
uint32_t read();
uint8_t getCommand();
uint16_t getValue();
void sendCommand(uint8_t command,uint8_t data);
void sendPacket(uint32_t data);
void printTimes(uint16_t *timesBuffer);
protected:
private:
void initSend();
void initRecieve();

};

#endif

