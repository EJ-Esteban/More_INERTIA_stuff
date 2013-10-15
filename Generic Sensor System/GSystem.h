#include "useful.h"
#include <msp430x16x.h>

#ifndef GSYSTEM_H_
#define GSYSTEM_H_

#define LED 	B8_0(&P1OUT)
//#define BUTTON	B8_1(&P6IN)

#define LED_TOGGLE()	LED^=1
#define LED_ON()		LED|=1
#define LED_OFF()		LED&=0

#define P1DIRMASK	0xFF
#define P1SELMASK	0x00
#define P1OUTMASK	0xFF

#define P6DIRMASK	0x00
#define P6SELMASK	0xFF

#define FREQ2SAMP(FREQ_VAL)	16384/(unsigned int)(FREQ_VAL)


#define	WAITING_FOR_INIT	1
#define	RUNNING				2
#define	PAUSED				3
#define	OFF					4

typedef struct
{
	unsigned char leadChar;
	unsigned char PortEn;
	unsigned char SampFreq;
	unsigned char BitsOfRep;
	unsigned int Checksum;
} commandStruct;

void portInit(void);
void terminalStart(int x);
void portSamp(void);
void parseCommands(void);
void buttonStartup(void);
void sysInit(void);
void uartParse(void);

#endif /* GSYSTEM_H_ */
