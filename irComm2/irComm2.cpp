// 
// 
// 

#include "irComm2.h"

//defining the sending ticks of each pulse
#define START 1000
#define STARTLOW 800
#define HIGH 400
#define LOW 200
#define SPACE 200
#define END 800

#define BUFFERLENGTH 34
#define TOGGLES 68

uint16_t decodeTimes[4]; // de lookup table met puls tijden
bool recieveSpeed = false; // bit of de frequentie 38 of 56 KHz is

volatile uint16_t decodeTicks = 0;
volatile uint16_t lastDecodeTicks = 0;
volatile uint16_t sendTicks = 0;
volatile uint16_t times[BUFFERLENGTH]; // buffer voor alle pulstijden
volatile uint16_t timeIndex = 0;
volatile uint8_t fallingEdges = 0;

uint8_t command = 0; //het te ontvangen commando
uint16_t data = 0; // de te ontvangen data
volatile uint8_t recieving = 0; // bit om te zien of de array voor ontvangen gebruikjt word door interrupts
volatile uint8_t sending = 0; // bit om aan te geven dat er nieuwe data klaarstaat voor de interrupts
uint16_t toggleTimes[TOGGLES]; // array met momenten waarop de IR getoggled moet worden
uint8_t toggleIndex = 0;
uint8_t pulse = 0;

ISR(TIMER2_COMPB_vect){// timer voor het zenden van IR
	TCNT2 = 0;
	decodeTicks++;
	sendTicks++;
	if (sending)// als 
	{
		if (sendTicks == toggleTimes[toggleIndex])// als de tijd overeenkomt met een waarde in de array toggle dan de IR
		{
			if (pulse){//zet de IR uit
				pulse = 0;
				TCCR2A &= ~(1<<COM2B0);
				TCCR2A &= ~(1<<COM2B1);
			}
			else{//zet de IR aan
				pulse = 1;
				TCCR2A |= (1<<COM2B0);
				TCCR2A &= ~(1<<COM2B1);
			}
			toggleIndex ++;
			if (toggleIndex >= TOGGLES)// als alle pulsen verstuurd zijn geef aan dat ie klaar is
			{
				toggleIndex = 0;
				sending = 0;
			}
		}
	}
}
ISR(INT0_vect) //interrupt voor de ontvanger
{
	
	uint16_t pulseLength = decodeTicks - lastDecodeTicks;
	fallingEdges++;
	
	if (fallingEdges >= 32)//als alle pulsen zijn ontvangen geef de array vrij
	{
		recieving = 0;
	}
	//als de tijd overeenkomt met een startpuls begin met opslaan van de pulsen
	if (pulseLength > decodeTimes[2]&& pulseLength < decodeTimes[3])
	{
		recieving = 1;
		decodeTicks = 0;
		timeIndex = 0;
		fallingEdges = 0;
	}
	
	
	
	times[timeIndex] = pulseLength;
	timeIndex++;
	if (timeIndex >= BUFFERLENGTH)
	{
		timeIndex = 0;
	}
	
	lastDecodeTicks = decodeTicks;
}

IR::IR(bool highFrequency = false){
	recieveSpeed = highFrequency;
	if (recieveSpeed)//aan de hand van de frequentie stel een andere lookup tabel in
	{
		decodeTimes[0] = 300;
		decodeTimes[1] = 700;
		decodeTimes[2] = 2000;
		decodeTimes[3] = 3000;
	}
	else{
		decodeTimes[0] = 100;
		decodeTimes[1] = 350;
		decodeTimes[2] = 500;
		decodeTimes[3] = 1500;
	}
	
	
}
void IR::initSend(){
	//set ir led port to output
	DDRD |= (1 << PORTD3);
	
	//set waveform generation to normal mode
	TCCR2A &= ~(1 << WGM20);
	TCCR2A &= ~(1 << WGM21);
	TCCR2B &= ~(1 << WGM22);
	
	
	//set prescaler to 1
	TCCR2B |= (1 << CS20);
	TCCR2B &= ~(1 << CS21);
	TCCR2B &= ~(1 << CS22);
	//set output compare register to 120 for 56KHz
	if (recieveSpeed)
	{
		OCR2B = 120;
		
	}
	//set output compare register to 185 for 38KHz
	else
	{
		OCR2B = 185;
	}
	//enable timer compare interrupt
	TIMSK2 |= (1 << OCIE2B);
	sei();
	//disconnect the timer from output pin
	TCCR2A &= ~(1<<COM2B0);
	TCCR2A &= ~(1<<COM2B1);
}
void IR::initRecieve(){
	
	//recieve pin setup
	DDRD &= ~(1<<PORTD2);
	PORTD &= ~(1<<PORTD2);
	EIMSK |= (1 << INT0);    //enable INT0
	EICRA |= (1 << ISC01);   //falling edge interrupt
	sei();  //enable interrupts
}




void IR::begin(){
	initRecieve();
	initSend();
}
void IR::run(){
	sending = 1;
}
void IR::sendCommand(uint8_t command,uint8_t data){//zet de te versturen data om naar 32 bits
	uint32_t output = 0;
	output |= (unsigned long)(command & 0xFF)<<24;//stuur commando normaal
	output |= (unsigned long)(~command & 0xFF)<<16;//stuur commando geïnverteerd zodat het pakket even lang blijft voor elke waarde
	output |= (unsigned long)(data & 0xFF)<<8;
	output |= (unsigned long)(~data & 0xFF)<<0;
	sendPacket(output);
}
void IR::sendPacket(uint32_t data){// zet alle toggletimes klaar voor de timer interrupt
	uint16_t ticks = 0;
	uint8_t tickIndex = 0;
	while(sending){//wachten tot de IR klaar is met sturen
		
	}
	toggleTimes[tickIndex] = ticks;
	tickIndex ++;
	toggleTimes[tickIndex] = (ticks += START);
	tickIndex ++;
	toggleTimes[tickIndex] = (ticks += STARTLOW);
	tickIndex ++;
	for (int i = 0;i<32;i++)//zet alle 32 bits in de array
	{
		if ((data >> i)&1)
		{
			toggleTimes[tickIndex] = (ticks += HIGH);
			tickIndex ++;
		}
		else{
			toggleTimes[tickIndex] = (ticks += LOW);
			tickIndex ++;
		}
		toggleTimes[tickIndex] = (ticks += SPACE);
		tickIndex ++;
	}
	
	toggleTimes[tickIndex] = (ticks += END);
	
	

	
}

uint32_t IR::read(){//decodeer de door interrupt opgeslagen tijden naar data
	
	uint32_t output = 0;
	uint8_t bitIndex = 0;
	uint8_t wait = recieving;
	
		while(wait){//wacht tot klaar met ontvangen
			wait = recieving;
		}
		for (uint8_t i = 0;i<BUFFERLENGTH;i++)//vergelijk alle tijden met de lookup tabel
		{
			uint8_t type = 3;
			for (uint8_t j = 0;j<3;j++)
			{
				if (times[i] < decodeTimes[j+1]&&times[i] > decodeTimes[j])
				{
					type = j;
				}
			}
			switch(type){
				case 0://bit is laag
				output &= ~((unsigned long)1<<bitIndex);
				bitIndex ++;
				break;
				case 1://bit is hoog
				output |= ((unsigned long)1<<bitIndex);
				bitIndex ++;
				break;
				case 2://startbit
				break;
				default:
				break;
			}
			
		
		
	}
	//decodeer de 32 bits terug naar 2 bytes 
	uint8_t command0 = (output>>24) & 0xFF;
	uint8_t command1 = (~output>>16) & 0xFF;
	uint8_t data0 = (output>>8) & 0xFF;
	uint8_t data1 = (~output>>0) & 0xFF;
	if (command0 == command1)// controleer of het pakket goed is ontvangen
	{
		command = command0;
		data = data0;
	}
	
	return output;
}
uint8_t IR::getCommand(){
	return command;
}
uint16_t IR::getValue(){
	return data;
}
