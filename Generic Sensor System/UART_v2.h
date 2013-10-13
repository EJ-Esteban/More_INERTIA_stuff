/**************************************************************************//**
 * \file UART_v2.h	You must examine this header file and uncomment certain
 * 					macros in order to enable/disable the UART0 and UART1
 * 					functions, as well as specifc features and settings.
 *****************************************************************************/
#include <in430.h>
//#include <msp430Fx.h>
#include <msp430x16x.h>

#include <ctype.h>
#include <stdlib.h>
#include <stddef.h>
#include <math.h>
#include <string.h>
#include <setjmp.h>

//Use this to select whether Hardware Handshakes are used
//disabled by default
//#define UART0_HANDSH_EN			//Hardware Handshaking enabled on UART0
//#define UART1_HANDSH_EN			//Hardware Handshaking enabled on UART1

//Use this to select using uart1 and uart0
#define USE_UART0
//#define USE_UART1

//Use one of these to select the master clock frequency ...
//#define CLK_800MHZ
//#define CLK_738MHZ
#define CLK_36864MHZ

///////////////////////////////////////////////////////////////////////////////
// use the following macros to determine the 'mode' parameter values
// for usartInit0() and usartInit1()
#define USART_NONE  (0)
#define USART_EVEN  (PENA + PEV)
#define USART_ODD   (PENA)
#define USART_1STOP (0)
#define USART_2STOP (SPB)
#define USART_7IBIT  (0)
#define USART_8BIT  (CHAR)

// Definitions for typical USART 'mode' settings
#define USART_8N1   (unsigned char)(USART_8BIT + USART_NONE + USART_1STOP)
//#define USART_7N1   (uint8_t)(USART_7BIT + USART_NONE + USART_1STOP)
//#define USART_8N2   (uint8_t)(USART_8BIT + USART_NONE + USART_2STOP)
//#define USART_7N2   (uint8_t)(USART_7BIT + USART_NONE + USART_2STOP)
//#define USART_8E1   (uint8_t)(USART_8BIT + USART_EVEN + USART_1STOP)
//#define USART_7E1   (uint8_t)(USART_7BIT + USART_EVEN + USART_1STOP)
//#define USART_8E2   (uint8_t)(USART_8BIT + USART_EVEN + USART_2STOP)
//#define USART_7E2   (uint8_t)(USART_7BIT + USART_EVEN + USART_2STOP)
//#define USART_8O1   (uint8_t)(USART_8BIT + USART_ODD  + USART_1STOP)
//#define USART_7O1   (uint8_t)(USART_7BIT + USART_ODD  + USART_1STOP)
//#define USART_8O2   (uint8_t)(USART_8BIT + USART_ODD  + USART_2STOP)
//#define USART_7O2   (uint8_t)(USART_7BIT + USART_ODD  + USART_2STOP)



//HandShake Line Def's
#define READY     			0					//Active Low Value handshake...
#define NOT_READY 			1

#ifdef UART0_HANDSH_EN							//Hardware Handshaking enabled on UART0
	#define UART0_RTS_MASK		0x10;			//Masking for UART0 RTS (must be on port1)
	#define UART0_CTS_MASK		0x04;			//Masking for UART0 CTS
	#define UART0_RTS  			B8_4(&P1IN)
	#define UART0_CTS  			B8_2(&P1OUT)
#endif
#ifndef	UART0_HANDSH_EN							//Hardware Handshaking disabled on UART0
	#define UART0_RTS_MASK		0x00;			//nothing on ports
	#define UART0_CTS_MASK		0x00;			//nothing on ports
	#define UART0_RTS		READY
	//requires that there are no references to UART0_CTS without checking for UART0_HANDSH_EN
#endif

#ifdef UART1_HANDSH_EN							//Hardware Handshaking enabled on UART1
	#define UART1_RTS_MASK		0x01;			//Masking for UART1 RTS (must be on port2)
	#define UART1_RTS  			B8_0(&P2IN)
	#define UART1_CTS_MASK		0x10;			//Masking for UART1 CTS
	#define UART1_CTS  			B8_4(&P2OUT)
#endif

#ifndef	UART1_HANDSH_EN							//Hardware Handshaking disabled on UART1
	#define UART1_RTS_MASK		0x00;			//nothing on ports
	#define UART1_CTS_MASK		0x00;			//nothing on ports
	#define UART1_RTS		READY
	#define UART1_CTS  			B8_4(&P2OUT)
	//requires that there are no references to UART1_CTS without checking for UART1_HANDSH_EN
#endif


#define U0IFG IFG1				///< USART0 RX & TX Interrupt Flag Register
#define U0IE  IE1				///< USART0 RX & TX Interrupt Enable Register
#define U0ME  ME1				///< USART0 RX & TX Module Enable Register
#define U1IFG IFG2				///< USART0 RX & TX Interrupt Flag Register
#define U1IE  IE2				///< USART0 RX & TX Interrupt Enable Register
#define U1ME  ME2				///< USART0 RX & TX Module Enable Register

//Mote that Buffer Sizes MUST be a power of 2
#define  UART0_RX_BUFFER_SIZE   256
#define  UART0_RX_MASK          UART0_RX_BUFFER_SIZE-1
#define  UART0_RX_MAX_QUE       UART0_RX_BUFFER_SIZE-16
#define  UART0_RX_REFILL        UART0_RX_BUFFER_SIZE-32

#define  UART0_TX_BUFFER_SIZE   1024
#define  UART0_TX_MASK          UART0_TX_BUFFER_SIZE-1

#define  UART1_RX_BUFFER_SIZE   64
#define  UART1_RX_MASK          UART1_RX_BUFFER_SIZE-1
#define  UART1_RX_MAX_QUE       UART1_RX_BUFFER_SIZE-16
#define  UART1_RX_REFILL        UART1_RX_BUFFER_SIZE-32

#define  UART1_TX_BUFFER_SIZE   64
#define  UART1_TX_MASK          UART1_TX_BUFFER_SIZE-1


void uart0Init(unsigned long baud , unsigned char mode);

int uart0Putch(char ch);

void InsertCharacter0(unsigned char ch);

void uart0Putbuf(char *data, int numBytes);

int uart0Putuint(unsigned int var);

int uart0Putulong(unsigned long var);

unsigned int uart0Space(void);

const char *uart0Puts(const char *string);

int uart0Write(const char *buffer, unsigned int count);

int uart0RxSize(void);

int uart0TxEmpty(void);

int uart0TxDone(void);

void uart0TxFlush(void);

void uart0RxFlush(void);

int uart0Getch(void);

__interrupt void uart0RcvIsr(void);

__interrupt void uart0XmtIsr(void);

__interrupt void port1Isr(void);

void uart1Init(unsigned long baud , unsigned char mode);

int uart1Putch(char ch);

void InsertCharacter1(unsigned char ch);

int uart1Putuint(unsigned int var);

int uart1Putulong(unsigned long var);

unsigned int uart1Space(void);

const char *uart1Puts(const char *string);

int uart1Write(const char *buffer, unsigned int count);

int uart1TxEmpty(void);

int uart1TxDone(void);

void uart1TxFlush(void);

int uart1Getch(void);

//__interrupt void uart1RcvIsr(void);

//__interrupt void uart1XmtIsr(void);

//__interrupt void port2Isr(void);
