#include <msp430x16x.h>
#include <stddef.h>
#include "useful.h"
#include "SensingAndTiming.h"
#include "UART_v2.h"
#include "GSystem.h"

volatile unsigned int data[8];
volatile int BitsOfRep;

extern unsigned char uartCheck;
extern unsigned char STATE;
//volatile int NumbOfSamp;

//void adcInit(int BoR, unsigned int NoS)
void adcInit(char BoR){

	BitsOfRep=BoR;
	if(!BitsOfRep) BitsOfRep=1;

	ADC12CTL0 &= ~ENC;												// ADC Initialization Settings
	ADC12CTL0  = MSC + REF2_5V + REFON + ADC12ON + SHT1_8;
	ADC12CTL1  = SHS_3 + SHP + CONSEQ_1;       						// Source sampling from Timer B CCR1 Output
	ADC12MCTL0 = INCH_0;
	ADC12MCTL1 = INCH_1;
	ADC12MCTL2 = INCH_2;
	ADC12MCTL3 = INCH_3;
	ADC12MCTL4 = INCH_4;
	ADC12MCTL5 = INCH_5;
	ADC12MCTL6 = INCH_6;
	ADC12MCTL7 = INCH_7+EOS;										// last sample
	ADC12IE = BIT7;													// Only interrupt upon the last one in sequence
	ADC12CTL0 |= ENC;

}

inline void processAndSend()
{
	int i;
	extern commandStruct commands;

	if(STATE == PAUSED) return;

	for(i = 0; i < 8; i++){
		if(commands.PortEn & (1 << i)) data[i] >>= (12-BitsOfRep);
		else data[i] = 0xFFFF;
		uart0Putuint(data[i]);
	}

	return;
}

#pragma vector=ADC12_VECTOR
__interrupt void ADC12_ISR(void)
{
	data[0]=ADC12MEM0;
	data[1]=ADC12MEM1;
	data[2]=ADC12MEM2;
	data[3]=ADC12MEM3;
	data[4]=ADC12MEM4;
	data[5]=ADC12MEM5;
	data[6]=ADC12MEM6;
	data[7]=ADC12MEM7;

	ADC12CTL0 |= ENC; // ADC12 Re-Enable Conversion

	uartCheck = 1;
	processAndSend();
}

// Timer B management
unsigned int sampStep = SPS_STD;				///< TBCCR1 Sampling Step
unsigned int flipStep = 32768;

/**************************************************************************//**
 * \brief Initialize the DCO to the target frequency needed to support UART
 *
 * \retval		1 success
 * \retval		-1 failure
 *****************************************************************************/
int clkInit(void)
{
	unsigned long TargFreq;
	long OutFreq;
	unsigned int status = 0;

	TargFreq = 3686400;							//3.686400 MHz

	// Run DCO init in critical section
	enter_critical(status);
		OutFreq = SetDco(TargFreq);
	exit_critical(status);

	if((OutFreq > ((long)(TargFreq)-10000)) && (OutFreq < ((long)(TargFreq)+10000)))
		return 1;								//Output frequency within bounds
	else
		return -1;								//Clock init failed (out of bounds)
}

/**************************************************************************//**
 * \brief Get the next ACLK edge for frequency comparison
 *
 * \return		capture value from CCR2
 *****************************************************************************/
unsigned int NextClockCompare(void)
{
	unsigned int firstCCR2, lastCCR2;
	unsigned int i;
	while ((CCTL2 & CCIFG) != CCIFG);		//Wait until capture occured!
	firstCCR2 = TACCR2;
	CCTL2 &= ~CCIFG;

	// Do 8 iterations (rather than having ACLK/8)
	for(i = 0; i < 8; i++) {
		while ((CCTL2 & CCIFG) != CCIFG);	//Wait until capture occured!
		lastCCR2 = TACCR2;
		CCTL2 &= ~CCIFG;
	}

	return lastCCR2 - firstCCR2;
}

/**************************************************************************//**
 * \brief Set the DCO to the desired frequency
 *
 * Keep adjusting the DCO and measuring the resulting frequency until the
 * desired frequency is reached.
 *
 * Record a new timestamp pair into the timing log of the flash card, using
 * the current timing epoch in effect.
 *
 * \param[in]	TargetFrequency		Target DCO frequency, in Hz
 * \return		Resulting frequency, in Hz
 * \retval		-1 if greater than +/- 40.1 kHz outside of target
 *****************************************************************************/
long SetDco(unsigned long TargetFrequency)
{
	unsigned int Delta, compareDelta;
	int i;

	Delta = (unsigned int)(TargetFrequency >> 12); //Target DCO = Delta*(4096)

	TACCTL2 = CM_1 + CCIS_1 + CAP;				// CAP, ACLK rising edge triggered
	TACTL = TASSEL_2 + MC_2 + TACLR;			// SMCLK, cont-mode, clear

	for(i = 2100; i > 0; i--){					// Number of iterations allowed (must be > 2^11)
		compareDelta = NextClockCompare();

		if (Delta == compareDelta) break;		// If equal, leave "for(2100)"
		else if (Delta < compareDelta){			// DCO is too fast, slow it down
			DCOCTL--;							// Decrement control
			if (DCOCTL == 0xFF){				// Did we roll under?
				if(BCSCTL1 & RSEL_BITS){
					BCSCTL1--;					//Sel lower RSEL
					DCOCTL = 0x60 ;  			// ReCenter Up DCO For Next Cycle
				}
			}
		}
		else {									// If DCO is too slow, speed it up
			DCOCTL++;							// Increment control
			if (DCOCTL == 0x00){				// Did we roll under
				if((BCSCTL1 & RSEL_BITS) != RSEL_BITS) {	// Make sure RSEL does not roll over
					BCSCTL1++; 					// Did DCO roll over? Sel higher RSEL
					DCOCTL = 0x60 ;  			// Recenter Up DCO For Next Cycle
				}
			}
		}
	}

	TACCTL2 = 0;								// Stop CCR2
	TACTL = 0;									// Stop Timer_A
	BCSCTL1 &= 0xCF;
	IFG1 &= ~OFIFG;								// Clear OSC Fault flag
	IE1   |= OFIE;            					// Enable OSC Fault interrupt

	//within +/- 40.96 kHz range
	if((compareDelta > (Delta-10)) && (compareDelta < (Delta+10))){
		return (((long)(compareDelta)) << 12);	// Result DCO = Compare*(4096)
	}
	else return -1;								// Error: far outside target range
}

/**************************************************************************//**
 * \brief Initialize Timer B
 *
 * Setup capture/compare settings for TBCCR0 to supply the maintenance clock
 * at 256 Hz, TBCCR1 to supply the sampling clock (default 256 Hz, which
 * effectively yields 128 Hz sampling rate), and TBCCTL6 for capture mode
 * to help with clock calibration.
 *
 * \see			TempoSystem.c for TimerB ISRs
 *****************************************************************************/
void tbInit(unsigned int SMP_rate)
{
	TBCCR0 = SPS_STD;								// Set timing CCR (SPS_128 -> 256 Hz)
	TBCCR1 = SMP_rate; 								// Set sampling CCR
	setSamplingRate(SMP_rate);
	TBCCTL0 =  CCIE;								// Set Timer B0 for system operation
	TBCCTL1 =  CCIE + OUTMOD_4; 					// Set Timer B1 for sampling
	TBCTL = TBSSEL_1 + MC_2 + TBIE;					// Source off ACLK, Continuous mode with interrupt bit set

	// Set up CCR6 for clock capture
	TBCCTL6 = CAP + CM_3 + SCS + CCIS1 + CCIE; 		// Capture Mode, Rising edge, Initially at ground...
}

/**************************************************************************//**
 * \brief Read the current TimerB value using a software-triggered capture
 *
 * Force a software-controlled capture of TimerB as a means of safely reading
 * the current TimerB value.
 *
 * \return		TimerB value as stored in TBCCR6 capture register
 *****************************************************************************/
unsigned int tbGetTime(void)
{
	TBCCTL6 &= ~COV;
	TBCCTL6 &= ~CCIFG ;						// Clear capture compare interrupt flag

    TBCCTL6 ^= CCIS0 ; 						// Flip Capture Edge,   effect a capture in Software ...
    while (!(TBIV & TBIV_TBCCR6));			// Wait for interrupt flag
    TBCCTL6 &= ~(CCIFG) ;
    return (TBCCR6) ;
}

/**************************************************************************//**
 * \brief Set the sampling rate to a value given by an unsigned int.
 *
 * controlled
 *****************************************************************************/
void setSamplingRate(unsigned int SMP_rate)
{
	sampStep = SMP_rate;
}

/**************************************************************************//**
 * \brief TBIV interrupts ISR
 *
 * Manage the sampling rate time source.
 *
 * \warning	Be very careful about editing this function. Consult your peers.
 *****************************************************************************/
#pragma vector=TIMERB1_VECTOR
__interrupt void Timer_B1(void)
{

	switch( TBIV ){
		// TBCCR1
		case TBIV_TBCCR1:
			P1OUT	^= 0x04;
			TBCCR1 += sampStep;	// Increment CCR1 for another sampling tick
			break;
 		// Overflow
		case TBIV_TBIFG:
	       //if(SysState == STATE_CALIB) tbMajorCntr++;  // Increment counter for clock calibration
    		break;
  	}
}

#pragma vector=TIMERB0_VECTOR
__interrupt void TB0_ISR (void)
{
	TBCCR0	+= flipStep;
	if(STATE == RUNNING ){
		LED_TOGGLE();
	}
}
