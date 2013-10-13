#include <msp430x16x.h>
#include "GSystem.h"
#include "useful.h"
#include "SensingAndTiming.h"
#include "UART_v2.h"

volatile commandStruct commands;
unsigned char STATE = WAITING_FOR_INIT;

void portInit(void){
	P1DIR=P1DIRMASK;
	P1SEL=P1SELMASK;
	P1OUT=P1OUTMASK;

	P6DIR=P6DIRMASK;
	P6SEL=P6SELMASK;
}


int getCommands(void){

	/* Payload format is as follows:
	 *
	 * 		'S'			 0xXX			  0xXX			0xXX			 0xXXXX
	 *
	 * [Start of Seq] [   PortEn   ] [  SampFreq  ] [  BitsOfRep  ] [    Checksum    ]
	 *
	 *    8 bits         8 bits          8 bits         8 bits           16 bits
	 */

	int z = 0;
	signed int c;

	commands.leadChar	= 'S';
	while((c = uart0Getch()) == -1);
	commands.PortEn	= c;
	while((c = uart0Getch()) == -1);
	commands.SampFreq = c;
	while((c = uart0Getch()) == -1);
	commands.BitsOfRep = c;
	while((c = uart0Getch()) == -1);
	commands.Checksum = (c <<  8);
	while((c = uart0Getch()) == -1);
	commands.Checksum |= (unsigned char)c;


	uart0RxFlush();

	if(commands.SampFreq == 0) return 0;
	if(commands.BitsOfRep > 12 || commands.BitsOfRep == 0) return 0;
	z = fletcherChecksum((unsigned char*)&commands, 4);
	if(commands.Checksum != z) return 0;

	else return 1;
	//commands.SampStep	=uart0Getch();*0x0100+uart0Getch();
	//commands.SampNumb	=commands.SampNumb1*0x0100+commands.SampNumb0;
}

void uartParse(void)
{
	switch(uart0Getch()){
		case 'P':	// Pause
			STATE = PAUSED;
			break;
		case 'C':	// Continue
			STATE = RUNNING;
			break;
		case 'K':
			STATE = OFF;
			LPM4;
			break;
		case 'R':	// Reset
			ADC12CTL0 &=~ENC; //disable ADC conversion/interrupt
			WDTCTL = 0;
			break;
		default:
			break;
	}
}

void sysInit(void){
	portInit();
	__delay_cycles(PAUSE_CYC);
	blink(1);
	if(clkInit() == -1){			// Initialize the system clock

		while(1);
	}
	blink(2);
	uart0Init(115200, USART_8N1);	// Initialize USART0 for communication}
	LED_ON();						// LED stays on to signal awaiting command

	_enable_interrupts();
	uart0Write("send command ", 13);

	while(1){
		if(uart0Getch()==(int)'S'){
			if(getCommands()) break;
			else uart0Putch('F');
		}
	}
	uart0Putch('S');
	__delay_cycles(5000);
	_disable_interrupts();

	blink(3);
	tbInit(FREQ2SAMP(commands.SampFreq));		//Initialize timer B at given sampling rate
	blink(4);
	adcInit(commands.BitsOfRep);	// Initialize ADC
	blink(5);
}
