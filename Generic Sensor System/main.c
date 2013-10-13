#include <msp430x16x.h>
#include "GSystem.h"

unsigned char uartCheck = 0;

int main(void)
{
	WDTCTL = WDTPW + WDTHOLD;             // Stop watchdog timer
	_disable_interrupts();
	sysInit();
	_enable_interrupts();

	while(1){
		if(uartCheck) uartParse();
	}
}
