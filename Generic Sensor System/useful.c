#include "useful.h"
#include "GSystem.h"

void blink(int rep)
{
	for(;rep > 0; rep--){
		LED_ON();
		__delay_cycles(TOGGLE_CYC);
		LED_OFF();
		__delay_cycles(TOGGLE_CYC);
	}

	__delay_cycles(PAUSE_CYC);

}

/**************************************************************************//**
 * \brief Perform a 16-bit fletcher checksum on a buffer
 *
 * \param		Buffer		A uchar buffer to be checksummed
 * \param		numBytes	Number of bytes to check (up to Buffer's length)
 * \return		16-bit checksum concatenated with sum1 (checkA) as the low
 * 				byte, and sum2 (checkB) as the high byte.
 * \warning		This checksum is insensitive to 0x00 and 0xFF words, and a
 * 				cleared buffer of 0x00s will checksum to 0x00, which may lead
 * 				to situations where the check passes but does not mean there
 * 				is meaningful information in the buffer.
 * \see			http://en.wikipedia.org/wiki/Fletcher's_checksum#Optimizations
 *****************************************************************************/
int fletcherChecksum(unsigned char *Buffer, int numBytes)
{
	int len = numBytes;
	int result = 0;
	unsigned char *data = Buffer;
	unsigned int sum1 = 0xff, sum2 = 0xff;

	while (len) {
		int tlen = len > 21 ? 21 : len;
		len -= tlen;
		do {
			sum1 += *data++;
			sum2 += sum1;
		} while (--tlen);

		sum1 = (sum1 & 0xff) + (sum1 >> 8);
		sum2 = (sum2 & 0xff) + (sum2 >> 8);
	}

	/* Second reduction step to reduce sums to 8 bits */
	sum1 = (sum1 & 0xff) + (sum1 >> 8);
	sum2 = (sum2 & 0xff) + (sum2 >> 8);
	result =(sum2 << 8) + sum1;

	return result;
}
