#include <msp430x16x.h>
#include "GSystem.h"
#include "useful.h"
#include "SensingAndTiming.h"
#include "UART_v2.h"

volatile commandStruct commands;
unsigned char STATE = WAITING_FOR_INIT;

void portInit(void) {
	P1DIR = P1DIRMASK;
	P1SEL = P1SELMASK;
	P1OUT = P1OUTMASK;

	P6DIR = P6DIRMASK;
	P6SEL = P6SELMASK;
}

int getCommands(void) {

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

	commands.leadChar = 'S';
	while ((c = uart0Getch()) == -1)
		;
	commands.PortEn = c;
	while ((c = uart0Getch()) == -1)
		;
	commands.SampFreq = c;
	while ((c = uart0Getch()) == -1)
		;
	commands.BitsOfRep = c;
	while ((c = uart0Getch()) == -1)
		;
	commands.Checksum = (c << 8);
	while ((c = uart0Getch()) == -1)
		;
	commands.Checksum |= (unsigned char) c;

	uart0RxFlush();

	//terminal mode detection
	if (commands.PortEn == 0x74 && commands.SampFreq == 0x72
			&& commands.SampFreq == 0x72) {
		commands.BitsOfRep=12;
		terminalStart(commands.Checksum);
		return 1;
	}

	if (commands.SampFreq == 0)
		return 0;
	if (commands.BitsOfRep > 12 || commands.BitsOfRep == 0)
		return 0;
	z = fletcherChecksum((unsigned char*) &commands, 4);
	if (commands.Checksum != z)
		return 0;

	else
		return 1;
	//commands.SampStep	=uart0Getch();*0x0100+uart0Getch();
	//commands.SampNumb	=commands.SampNumb1*0x0100+commands.SampNumb0;
}

void terminalStart(int x) {
	char a, b;
	a = x >> 8;
	b = x & 0xFF;

	//controls the frequency
	switch (a) {
	case '0':
		commands.PortEn = 0x01;
		break;
	case '1':
		commands.PortEn = 0x03;
		break;
	case '2':
		commands.PortEn = 0x07;
		break;
	case '3':
		commands.PortEn = 0x0F;
		break;
	case '4':
		commands.PortEn = 0x1F;
		break;
	case '6':
		commands.PortEn = 0x7F;
		break;
	case '7':
		commands.PortEn = 0xFF;
		break;
	default:
		commands.PortEn = 0x3F;
	}

	switch (b) {
	case '0':
		commands.SampFreq = 1;
		break;
	case '1':
		commands.SampFreq = 2;
		break;
	case '2':
		commands.SampFreq = 4;
		break;
	case '3':
		commands.SampFreq = 8;
		break;
	case '4':
		commands.SampFreq = 16;
		break;
	case '5':
		commands.SampFreq = 32;
		break;
	case '6':
		commands.SampFreq = 64;
		break;
	case '8':
		commands.SampFreq = 255;
		break;
	default:
		commands.SampFreq = 128;
	}
}

void uartParse(void) {
	switch (uart0Getch()) {
	case 'P': // Pause
		STATE = PAUSED;
		LED_ON();
		break;
	case 'C': // Continue
		STATE = RUNNING;
		break;
	case 'K':
		LED_OFF();
		STATE = OFF;
		LPM4;
		break;
	case 'R': // Reset
		LED_ON();
		ADC12CTL0 &= ~ENC; //disable ADC conversion/interrupt
		WDTCTL = 0;
		break;
	default:
		break;
	}
}

void sysInit(void) {
	portInit();
	__delay_cycles(PAUSE_CYC);
	blink(1);
	if (clkInit() == -1) { // Initialize the system clock

		while (1)
			;
	}
	blink(2);
	uart0Init(115200, USART_8N1); // Initialize USART0 for communication}
	LED_ON(); // LED stays on to signal awaiting command

	_enable_interrupts();
	uart0Write("send command ", 13);

	while (1) {
		if (uart0Getch() == (int) 'S') {
			if (getCommands())
				break;
			else
				uart0Putch('F');
		}
	}
	uart0Putch('S');
	__delay_cycles(5000);
	_disable_interrupts();

	blink(3);
	tbInit(FREQ2SAMP(commands.SampFreq)); //Initialize timer B at given sampling rate
	blink(4);
	adcInit(commands.BitsOfRep); // Initialize ADC
	blink(5);
	STATE = RUNNING;
}
