#ifndef USEFUL_H_
#define USEFUL_H_

/**************************************************************************//**
 * \brief Enter a critical section
 *
 * \param	SR_state	Local unsigned int to hold the status register
 * \see		exit_critical()
 *****************************************************************************/
#define enter_critical(SR_state)           do { \
  (SR_state) = (_get_SR_register() & 0x08); \
  _disable_interrupts(); \
} while (0)

/**************************************************************************//**
 * \brief Exit a critical section
 *
 * \param	SR_state	Local unsigned int to hold the status register
 * \see		enter_critical()
 *****************************************************************************/
#define exit_critical(SR_state)         _bis_SR_register(SR_state)

/// Bit Access Structure
typedef struct Bits8
{
    volatile unsigned Bitx0 : 1 ;
    volatile unsigned Bitx1 : 1 ;
    volatile unsigned Bitx2 : 1 ;
    volatile unsigned Bitx3 : 1 ;
    volatile unsigned Bitx4 : 1 ;
    volatile unsigned Bitx5 : 1 ;
    volatile unsigned Bitx6 : 1 ;
    volatile unsigned Bitx7 : 1 ;
} Bits ;

// Bit Access Defines
#define B8_0(x) (((Bits *) (x))->Bitx0)				// Bit in Byte Defines
#define B8_1(x) (((Bits *) (x))->Bitx1)
#define B8_2(x) (((Bits *) (x))->Bitx2)
#define B8_3(x) (((Bits *) (x))->Bitx3)
#define B8_4(x) (((Bits *) (x))->Bitx4)
#define B8_5(x) (((Bits *) (x))->Bitx5)
#define B8_6(x) (((Bits *) (x))->Bitx6)
#define B8_7(x) (((Bits *) (x))->Bitx7)

// Cycle constants for blink()
#define TOGGLE_CYC	600000		///< Clock cycles to wait between toggle of LED
#define PAUSE_CYC	1000000		///< Clock cycles to wait between LED pulse sets

void blink(int rep);

int fletcherChecksum(unsigned char *Buffer, int numBytes);

#endif /*USEFUL_H_*/
