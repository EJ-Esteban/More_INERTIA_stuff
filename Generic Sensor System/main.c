/**************************************************************************//**
 * MSPADC (Generic Sensor System)
 *
 * A stripped down version of the TEMPO3.2f firmware. It is useful for sampling
 * any analog sensor via a serial link to an Olimex board.
 *
 *****************************************************************************/

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
