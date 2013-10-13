#ifndef SENSINGANDTIMING_H_
#define SENSINGANDTIMING_H_

#define			SPS_STD			0x007F		///Standard Sampling rate (128Hz)

//clock timing
#define 		DELTA 			900			///< Target DCO = DELTA*(4096) = 3.686400 MHz
#define			RSEL_BITS		0x07

//Timing Function Prototypes

int clkInit(void);
unsigned int NextClockCompare(void);
long SetDco(unsigned long TargetFrequency);
void tbInit(unsigned int SMP_rate);
unsigned int tbGetTime(void);
void setSamplingRate(unsigned int SMP_rate);

//void adcInit(int BoR, unsigned int NoS);
void adcInit(char BoR);
void adcAddSamples(unsigned int NoS);
void adcSetSamples(unsigned int NoS);
unsigned int getData(int choose);
unsigned int getDataBOR(int choose);
__interrupt void ADC12_ISR(void);

#endif /* SENSINGANDTIMING_H_ */
