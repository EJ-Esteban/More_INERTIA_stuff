#include <msp430x16x.h>
#include "useful.h"
#include "UART_v2.h"

//UART Global Variables
volatile unsigned char UartRx0Buffer[UART0_RX_BUFFER_SIZE] ;
unsigned int		   UartRx0GetIndex ;
volatile unsigned int  UartRx0PutIndex ;
volatile unsigned int  Uart0RxQueSize    ;

unsigned char Uart0TxBuffer[UART0_TX_BUFFER_SIZE] ;
volatile unsigned int  Uart0TxGetIndex   ;
unsigned int  		   Uart0TxPutIndex   ;
volatile unsigned int  Uart0TxQueSize    ;
volatile unsigned int  Uart0TxRunning    ;

volatile unsigned char UartRx1Buffer[UART1_RX_BUFFER_SIZE] ;
unsigned int		   UartRx1GetIndex ;
volatile unsigned int  UartRx1PutIndex ;
volatile unsigned int  Uart1RxQueSize    ;

unsigned char Uart1TxBuffer[UART1_TX_BUFFER_SIZE] ;
volatile unsigned int  Uart1TxGetIndex   ;
unsigned int		   Uart1TxPutIndex   ;
volatile unsigned int  Uart1TxQueSize    ;
volatile unsigned int  Uart1TxRunning    ;


#ifdef USE_UART0 //change in header file for using uart0 or not

/**************************************************************************//**
 * \brief Initialize USART for async mode
 *
 * \param		baud	Baud rate
 * \param		mode	UART operation mode (see UART_v2.h)
 *****************************************************************************/
void uart0Init(unsigned long baud,  unsigned char mode)
{
	UartRx0GetIndex = UartRx0PutIndex = Uart0RxQueSize = 0 ;
	Uart0TxGetIndex = Uart0TxPutIndex = Uart0TxQueSize = 0 ;
	Uart0TxRunning = 0 ;

	U0ME |= UTXE0 + URXE0;  				// enable USART0 module

	P3DIR |=   0x10   ;	  					//Establish Port Directions For UART0 use
	P3DIR &= ~0x20   ;
	P3SEL |=   0x30   ;


	P1DIR |=  UART0_CTS_MASK;				// Establish Directions for HandShake Lines
	P1DIR &= ~UART0_RTS_MASK;   			//This is an input which tells us we can send more characters
	P1IES |=  UART0_RTS_MASK;				//This sets an interrupt on High to low transition
	P1IES |=  UART0_CTS_MASK;				//This sets an interrupt on High to low transition
	P1IFG = 0;   							//Clear All Interrupts	from this port
	P1IE  |=  UART0_RTS_MASK;				//And Enable the interrupts.


	UCTL0 = CHAR+SWRST;						// reset the USART

	// set the number of characters and other
	// user specified operating parameters
	UCTL0 = mode;

	// select the baudrate generator clock
	UTCTL0 = SSEL0+SSEL1 ;                	// UCLK = SMCLK;

	// load the modulation & baudrate divisor registers
	//Note that these baud rates were obtained from the MSPGcc
	//Online calculator ..
	switch (baud)
	{
		case 115200 :
			#ifdef CLK_800MHZ
				  UBR00=0x45;
				  UBR10=0x00;
				  UMCTL0=0xD6;
			#endif
			#ifdef CLK_738MHZ
				  UBR00=0x40;
				  UBR10=0x00;
				  UMCTL0=0x10;
			#endif
			#ifdef CLK_36864MHZ
				  UBR00=0x20;
				  UBR10=0x00;
				  UMCTL0=0x00;
			#endif
		break ;
		case 9600 :
	    default :
			#ifdef CLK_800MHZ
				  UBR00=0x41 ;
				  UBR10=0x03 ;
				  UMCTL0=0x92;
			#endif
			#ifdef CLK_738MHZ
			      UBR00=0x00 ;
			      UBR10=0x03 ;
			      UMCTL0=0x00;
			#endif
			#ifdef CLK_36864MHZ
				  UBR00=0x80;
				  UBR10=0x01;
				  UMCTL0=0x00;
			#endif
		break ;
		case 4800 :
			#ifdef CLK_800MHZ
				  UBR00=0x82 ;
				  UBR10=0x06 ;
				  UMCTL0=0xDB;
			#endif
			#ifdef CLK_738MHZ
			      UBR00=0x00 ;
			      UBR10=0x06 ;
			      UMCTL0=0x00;
			#endif
			#ifdef CLK_36864MHZ
				  UBR00=0x00;
				  UBR10=0x03;
				  UMCTL0=0x00;
			#endif
		break ;
	}
	URCTL0 = 0;  							// init receiver contol register

	// enable receiver interrupts	 only, xmitter will be enabled
	//when there are characters to go ....
	U0IE |= URXIE0 ;
	#ifdef UART0_HANDSH_EN
		UART0_CTS = READY;
	#endif
	ME1   |= URXE0 +UTXE0; 					// Enable the module
	P3SEL |= 0x30;         					// P3.6,7 special function
	U0CTL &= ~SWRST;       					// Release the UART to operation
	IE1   |= URXIE0;            			// Enable RX interrupt

}

/**************************************************************************//**
 * \brief Put a character in the UART transmit buffer for transmission
 *
 * \param		ch	Character to be sent
 * \retval		ch on success
 * \retval		-1 on error (queue full)
 *****************************************************************************/
int uart0Putch(char ch)
{
	int state = 0;
	int retval ;
	enter_critical(state);
	if (Uart0TxQueSize >=  UART0_TX_BUFFER_SIZE)
	{
		  retval =  -1;  				// no room in the inn
	}
	else
	{
	    InsertCharacter0(ch) ;
	    U0IE &= ~UTXIE0 ;               // disable TX interrupts

	    if (Uart0TxRunning) 			// check if in process of sending data
	    {
			_NOP() ;
	    }
	    else
	    {
	      Uart0TxRunning = 1 ;			// set running flag and write to output register
	      IFG1 |= UTXIFG0;
	      IE1 |= UTXIE0;
	    }
		if (UART0_RTS == READY)
		{
			U0IE |= UTXIE0;	 			// enable TX interrupts if handshake is clear
		}
		retval = ch & 0xff ;
	}
	exit_critical(state);
	return(retval) ;
}

/**************************************************************************//**
 * \brief Insert a character into the transmit buffer
 *
 * \param		ch	Character to insert
 *****************************************************************************/
void InsertCharacter0(unsigned char ch)
{
	int state = 0;
	enter_critical(state);
	++Uart0TxPutIndex ;
	Uart0TxPutIndex &= UART0_TX_MASK ;
	++Uart0TxQueSize ;
	Uart0TxBuffer[Uart0TxPutIndex] = ch ;
	exit_critical(state);
}

/**************************************************************************//**
 * \brief Copy a buffer into the UART for transmission
 *
 * \param		data		Buffer of data to send
 * \param		numBytes	Number of bytes to pull from Buffer
 *
 * \see			uart0Write(), which this is rendundant to
 *****************************************************************************/
void uart0Putbuf(char *data, int numBytes){
	int i = 0;
	char *ptr = data;
														//Driver Enable for 485
 	for (i = 0; i < numBytes; i++){
		while(uart0Putch (*ptr) == -1);
		ptr++;
	}


}

/**************************************************************************//**
 * \brief Put in integer in the transmit buffer to send
 *
 * \param		var		Integer to send
 * \retval		var on success
 * \retval		-1 on error (queue full)
 *****************************************************************************/
int uart0Putuint(unsigned int var)
{
	int retval ;
	while((retval = uart0Putch((unsigned char)((var & 0xFF00) >> 8))) == -1);
	while((retval = uart0Putch((unsigned char)(var & 0x00FF))) == -1);
	return retval;
}

/**************************************************************************//**
 * \brief Put an unsigned long in the UART transmit queue for sending
 *
 * \param		var		Unsigned long to send
 * \retval		var on success
 * \retval		-1 on error (queue full)
 *****************************************************************************/
int uart0Putulong(unsigned long var)
{
	int retval ;
	while((retval = uart0Putch((unsigned char)((var & 0xFF000000) >> 24))) == -1);
	while((retval = uart0Putch((unsigned char)((var & 0x00FF0000) >> 16))) == -1);
	while((retval = uart0Putch((unsigned char)((var & 0x0000FF00) >> 8))) == -1);
	while((retval = uart0Putch((unsigned char)(var & 0x000000FF))) == -1);
	return retval;
}

/**************************************************************************//**
 * \brief Get the available space in the transmit queue
 *
 * \return		Available space in transmit queue
 *****************************************************************************/
unsigned int uart0Space(void)
{
	return(UART0_TX_BUFFER_SIZE-Uart0TxQueSize) ;
}

/**************************************************************************//**
 * \brief Write a NULL-terminated string to the UART transmit queue
 *
 * Writes until NULL-terminator found or queue full. DOES NOT write the NULL
 * character into the buffer.
 *
 * \param		string	(Pointer to a) NULL-terminated string to send
 * \retval		Pointer to the next character to be written
 * \retval		NULL if full string writen
 *****************************************************************************/
const char *uart0Puts(const char *string)
{
	register char ch;

	while (ch = *string)
	{
	    while(uart0Putch(ch) == -1);
	    string++;
	}
	return string;
}

/**************************************************************************//**
 * \brief Write a buffer to the transmit queue for sending
 *
 * \param		buffer	Buffer of data to send
 * \param		count	Number of bytes to take from buffer
 * \retval		0 on success
 * \retval		-1 if insufficient space (no chars written)
 * \retval		-2 if space runs out after started writing
 *
 * \see			uart0Putbuf(), Sam's later blocking version of this
 *****************************************************************************/
int uart0Write(const char *buffer, unsigned int count)
{
	if (count > uart0Space())
	{
	    return -1;
	}
	else
	{
	    while (count && (uart0Putch(*buffer++) >= 0))
	      count--;
		  return (count ? -2 : 0);
	}
}

/**************************************************************************//**
 * \brief Get the number of bytes in the receive queue
 *
 * \return		Number of bytes in the receive queue
 *****************************************************************************/
int uart0RxSize(void)
{
	return Uart0RxQueSize;
}

/**************************************************************************//**
 * \brief Get status of UART registers (i.e., whether empty)
 *
 * Check whether the TX holding and shift registers are empty
 *
 * \retval		1 if empty
 * \retval		0 if not empty
 *****************************************************************************/
int uart0TxEmpty(void)
{
	return (UTCTL0 & TXEPT) == TXEPT;
}

/**************************************************************************//**
 * \brief Report whether UART is done sending all enqueued transmit data
 *
 * Check both the UART transmit buffer and the UART holding and shift
 * registers to determine whether all waiting transmission is done.
 *
 * \retval		1 if transmission all done
 * \retval		0 if transmission not all done
 *****************************************************************************/
int uart0TxDone(void)
{
	int state = 0;
	int retval;
	enter_critical(state);
		retval = (Uart0TxQueSize == 0) && uart0TxEmpty();
	exit_critical(state);
	return retval;
}

/**************************************************************************//**
 * \brief Flush all enqueued characters from the UART transmit buffer
 *****************************************************************************/
void uart0TxFlush(void)
{
	int state = 0;
	enter_critical(state);
	Uart0TxGetIndex = Uart0TxPutIndex = Uart0TxQueSize = 0 ;
	Uart0TxRunning = 0 ;
	exit_critical(state);
}

/**************************************************************************//**
 * \brief Flush all characters from the receive queue
 *****************************************************************************/
void uart0RxFlush(void)
{
	int state = 0;
	enter_critical(state);
	UartRx0GetIndex = UartRx0PutIndex = Uart0RxQueSize = 0 ;
	exit_critical(state);
}

/**************************************************************************//**
 * \brief Get a character from the UART receive queue
 *
 * \retval		character from buffer on success
 * \retval		-1 if no characters available
 *****************************************************************************/
int uart0Getch(void)
{
	unsigned char ch;
	if (Uart0RxQueSize == 0) 					// check if character is available
	{
	    return -1;
	}
	else
	{
	    ch = UartRx0Buffer[UartRx0GetIndex++]; 	// get character, bump pointer
	    UartRx0GetIndex &= UART0_RX_MASK ; 		// limit the pointer
		--Uart0RxQueSize ;
		if (Uart0RxQueSize <= UART0_RX_REFILL)
		{
			#ifdef UART0_HANDSH_EN
				UART0_CTS = READY ; 			//If Que is small, enough, handshake "on"
			#endif
		}
		else
		{
			if (Uart0RxQueSize >= UART0_RX_MAX_QUE)
			{
				#ifdef UART0_HANDSH_EN
					UART0_CTS = NOT_READY ;
				#endif
			}
		}
		return (ch & 0xff) ;
	}
}

/**************************************************************************//**
 * \brief UART0 Receive ISR
 *
 * Grab the ready character and put it in the receive buffer, unless the UART
 * registers indicate an error, in which case dummy read to clear the error.
 *****************************************************************************/
#pragma vector=USART0RX_VECTOR
__interrupt void uart0RcvIsr(void)
{
	volatile unsigned char dummy;
	// check status register for receive errors
	if (URCTL0 & RXERR)
	{
	     dummy = RXBUF0;							// clear error flags by forcing a dummy read
	}
	else
	{
	    UartRx0Buffer[UartRx0PutIndex++] = RXBUF0; 	// read the character
		UartRx0PutIndex &= UART0_RX_MASK ;
		++Uart0RxQueSize ;
		if (Uart0RxQueSize >= UART0_RX_MAX_QUE)
		{
			#ifdef UART0_HANDSH_EN
				UART0_CTS = NOT_READY ;
			#endif
		}
	}
}

/**************************************************************************//**
 * \brief UART0 Transmit ISR
 *
 * Send the next character if one is ready, otherwise disable the TX
 * interrupt (to be re-enabled later by uart0Putch())
 *****************************************************************************/
#pragma vector=USART0TX_VECTOR
__interrupt void uart0XmtIsr(void)
{
	if (UART0_RTS == READY)
	{
	    if (Uart0TxQueSize > 0)
	    {
	    	++Uart0TxGetIndex ;
	    	Uart0TxGetIndex &= UART0_TX_MASK ;
	      	--Uart0TxQueSize ;
	    	TXBUF0 = Uart0TxBuffer[Uart0TxGetIndex] ;
	    }
		else
		{
		   // if((U0TCTL & TXEPT) != 0)
		   // {
	            U0IE &= ~UTXIE0 ;
	            Uart0TxRunning = 0;
	      //  }
		}


	}
	else
	{
	//   if((U0TCTL & TXEPT) != 0)
	        U0IE &= ~UTXIE0 ;
	}
}

/**************************************************************************//**
 * \brief Port 1 ISR
 *
 * Handle RTS line change (if enabled) to re-enable transmission
 *****************************************************************************/
#pragma vector=PORT1_VECTOR
__interrupt void port1Isr(void)
{
	P1IFG = 0 ; 		//Clear Interrupt source
	U0IE |= UTXIE0 ; 	//Turn on Xmitter interrupt
}

#endif



#ifdef USE_UART1 //change in header file for using uart1 or not


/**************************************************************************//**
 * \brief Initialize USART for async mode
 *
 * \param		baud	Baud rate
 * \param		mode	UART operation mode (see UART_v2.h)
 *****************************************************************************/
void uart1Init(unsigned long baud,  unsigned char mode)
{
	UartRx1GetIndex = UartRx1PutIndex = Uart1RxQueSize = 0 ;
	Uart1TxGetIndex = Uart1TxPutIndex = Uart1TxQueSize = 0 ;
	Uart1TxRunning = 0 ;

	U1ME |= UTXE1 + URXE1;  		// enable USART1 module

	P3SEL |= 0xC0;          		// P3.6,7 special function
	P3DIR |= 0x40;          		// P3.6   TX output
	P3DIR &= ~0x80;         		// P3.7   RX input


	P2DIR |= UART1_CTS_MASK;   		// Establish Directions for HandShake Lines
	P2DIR &= ~UART1_RTS_MASK;   	//This is an input which tells us we can send more characters
	P2IES |= UART1_RTS_MASK;		//This sets an interrupt on High to low transition
	P2IES |= UART1_CTS_MASK;		//This sets an interrupt on High to low transition
	P2IFG = 0;   					//Clear All Interrupts	from this port
	P2IE |= UART1_RTS_MASK;   		//And Enable the interrupts.


	UCTL1 = CHAR+SWRST;				// reset the USART

	// set the number of characters and other
	// user specified operating parameters
	UCTL1 = mode;

	// select the baudrate generator clock
	UTCTL1 = SSEL1 ;                // UCLK = SMCLK;

	// load the modulation & baudrate divisor registers
	//Note that these baud rates were obtained from the MSPGcc
	//Online calculator ..
	switch (baud)
	{
	  	case 115200 :
			#ifdef CLK_800MHZ
				  UBR01=0x45;
				  UBR11=0x00;
				  UMCTL1=0xD6;
			#endif
			#ifdef CLK_738MHZ
				  UBR01=0x40;
				  UBR11=0x00;
				  UMCTL1=0x10;
			#endif
			#ifdef CLK_36864MHZ
				  UBR01=0x20;
				  UBR11=0x00;
				  UMCTL1=0x00;
			#endif
	  	break ;
		case 9600 :
	    default :
			#ifdef CLK_800MHZ
				  UBR01=0x41 ;
				  UBR11=0x03 ;
				  UMCTL1=0x92;
			#endif
			#ifdef CLK_738MHZ
			      UBR01=0x00 ;
			      UBR11=0x03 ;
			      UMCTL1=0x00;
			#endif
			#ifdef CLK_36864MHZ
				  UBR01=0x80;
				  UBR11=0x01;
				  UMCTL1=0x00;
			#endif
		break ;
		case 4800 :
			#ifdef CLK_800MHZ
				  UBR01=0x82 ;
				  UBR11=0x06 ;
				  UMCTL1=0xDB;
			#endif
			#ifdef CLK_738MHZ
			      UBR01=0x00 ;
			      UBR11=0x06 ;
			      UMCTL1=0x00;
			#endif
			#ifdef CLK_36864MHZ
				  UBR01=0x00;
				  UBR11=0x03;
				  UMCTL1=0x00;
			#endif
		break ;
	}
	URCTL1 = 0;  					// init receiver contol register

	ME2   |= URXE1 +UTXE1; 			// Enable the module
	P3SEL |= 0xC0;         			// P3.6,7 special function
	U1CTL &= ~SWRST;       			// Release the UART to operation
	IE2   |= URXIE1;            	// Enable UART Rx interrupt

	// enable receiver interrupts	 only, xmitter will be enabled
	//when there are characters to go ....
	U1IE |= URXIE1 ;
	#ifdef UART0_HANDSH_EN
		UART1_CTS = READY ;
	#endif
}

/**************************************************************************//**
 * \brief Put a character in the UART transmit buffer for transmission
 *
 * \param		ch	Character to be sent
 * \retval		ch on success
 * \retval		-1 on error (queue full)
 *****************************************************************************/
int uart1Putch(char ch)
{
	int retval ;
	int state = 0;
	enter_critical(state);
	if (Uart1TxQueSize >=  UART1_TX_BUFFER_SIZE)
	{
		  retval =  -1;  			// no room in the inn
	}
	else
	{
	    InsertCharacter1(ch) ;
	    U1IE &= ~UTXIE1 ;			// disable TX interrupts

	    if (Uart1TxRunning) 		// check if in process of sending data
	    {
			_NOP() ;
	    }
	    else
	    {
	      Uart1TxRunning = 1 ;		// set running flag and write to output register
	 	  IFG2 |= UTXIFG1;
	 	  IE2 |= UTXIE1;
	    }
		if (UART1_RTS == READY)
		{
			U1IE |= UTXIE1;	 		// enable TX interrupts if handshake is clear
		}
		retval = ch & 0xff ;
	}
	exit_critical(state);
	return(retval) ;
}

/**************************************************************************//**
 * \brief Insert a character into the transmit buffer
 *
 * \param		ch	Character to insert
 *****************************************************************************/
void InsertCharacter1(unsigned char ch)
{
	int state = 0;
    enter_critical(state);
	++Uart1TxPutIndex ;
	Uart1TxPutIndex &= UART1_TX_MASK ;
	++Uart1TxQueSize ;
	Uart1TxBuffer[Uart1TxPutIndex] = ch ;
	exit_critical(state);
}

/**************************************************************************//**
 * \brief Put in integer in the transmit buffer to send
 *
 * \param		var		Integer to send
 * \retval		var on success
 * \retval		-1 on error (queue full)
 *****************************************************************************/
int uart1Putuint(unsigned int var)
{
	int retval ;
	while((retval = uart1Putch((unsigned char)((var & 0xFF00) >> 8))) == -1);
	while((retval = uart1Putch((unsigned char)(var & 0x00FF))) == -1);
	return retval;
}

/**************************************************************************//**
 * \brief Put an unsigned long in the UART transmit queue for sending
 *
 * \param		var		Unsigned long to send
 * \retval		var on success
 * \retval		-1 on error (queue full)
 *****************************************************************************/
int uart1Putulong(unsigned long var)
{
	int retval ;
	while((retval = uart1Putch((unsigned char)((var & 0xFF000000) >> 24))) == -1);
	while((retval = uart1Putch((unsigned char)((var & 0x00FF0000) >> 16))) == -1);
	while((retval = uart1Putch((unsigned char)((var & 0x0000FF00) >> 8))) == -1);
	while((retval = uart1Putch((unsigned char)(var & 0x000000FF))) == -1);
	return retval;
}

/**************************************************************************//**
 * \brief Get the available space in the transmit queue
 *
 * \return		Available space in transmit queue
 *****************************************************************************/
unsigned int uart1Space(void)
{
	return(UART1_TX_BUFFER_SIZE-Uart1TxQueSize) ;
}

/**************************************************************************//**
 * \brief Write a NULL-terminated string to the UART transmit queue
 *
 * Writes until NULL-terminator found or queue full. DOES NOT write the NULL
 * character into the buffer.
 *
 * \param		string	(Pointer to a) NULL-terminated string to send
 * \retval		Pointer to the next character to be written
 * \retval		NULL if full string writen
 *****************************************************************************/
const char *uart1Puts(const char *string)
{
	register char ch;

	while (ch = *string)
	{
	    while(uart1Putch(ch) == -1);
	    string++;
	}
	return string;
}

/**************************************************************************//**
 * \brief Write a buffer to the transmit queue for sending
 *
 * \param		buffer	Buffer of data to send
 * \param		count	Number of bytes to take from buffer
 * \retval		0 on success
 * \retval		-1 if insufficient space (no chars written)
 * \retval		-2 if space runs out after started writing
 *****************************************************************************/
int uart1Write(const char *buffer, unsigned int count)
{

	if (count > uart1Space())
	{
	    return -1;
	}
	else
	{
	    while (count && (uart1Putch(*buffer++) >= 0))
	      count--;
		  return (count ? -2 : 0);
	}
}


/**************************************************************************//**
 * \brief Get status of UART registers (i.e., whether empty)
 *
 * Check whether the TX holding and shift registers are empty
 *
 * \retval		1 if empty
 * \retval		0 if not empty
 *****************************************************************************/
int uart1TxEmpty(void)
{
	return (UTCTL1 & TXEPT) == TXEPT;
}

/**************************************************************************//**
 * \brief Report whether UART is done sending all enqueued transmit data
 *
 * Check both the UART transmit buffer and the UART holding and shift
 * registers to determine whether all waiting transmission is done.
 *
 * \retval		1 if transmission all done
 * \retval		0 if transmission not all done
 *****************************************************************************/
int uart1TxDone(void)
{
	int state = 0;
	int retval;
	enter_critical(state);
		retval = (Uart1TxQueSize == 0) && uart1TxEmpty();
	exit_critical(state);
	return retval;
}

/**************************************************************************//**
 * \brief Flush all enqueued characters from the UART transmit buffer
 *****************************************************************************/
void uart1TxFlush(void)
{
	int state = 0;
	enter_critical(state);
	Uart1TxGetIndex = Uart1TxPutIndex = Uart1TxQueSize = 0 ;
	Uart1TxRunning = 0 ;
	exit_critical(state);
}

/**************************************************************************//**
 * \brief Get a character from the UART receive queue
 *
 * \retval		character from buffer on success
 * \retval		-1 if no characters available
 *****************************************************************************/
int uart1Getch(void)
{
	unsigned char ch;

	if (Uart1RxQueSize == 0) 					// check if character is available
	{
	    return -1;
	}
	else
	{
	    ch = UartRx1Buffer[UartRx1GetIndex++]; 	// get character, bump pointer
	    UartRx1GetIndex &= UART1_RX_MASK ; 		// limit the pointer
		--Uart1RxQueSize ;
		if (Uart1RxQueSize <= UART1_RX_REFILL)
		{
			#ifdef UART1_HANDSH_EN
				UART1_CTS = READY ; 			//If Que is small, enough, handshake "on"
			#endif
		}
		else
		{
			if (Uart1RxQueSize >= UART1_RX_MAX_QUE)
			{
				#ifdef UART1_HANDSH_EN
					UART1_CTS = NOT_READY ;
				#endif
			}
		}
		return (ch & 0xff) ;
	}
}

/**************************************************************************//**
 * \brief UART1 Receive ISR
 *
 * Grab the ready character and put it in the receive buffer, unless the UART
 * registers indicate an error, in which case dummy read to clear the error.
 *****************************************************************************/
#pragma vector=USART1RX_VECTOR
__interrupt void uart1RcvIsr(void)
{
	volatile unsigned char dummy;
	// check status register for receive errors
	if (URCTL1 & RXERR)
	{
	     dummy = RXBUF1;							// clear error flags by forcing a dummy read
	}
	else
	{
	    UartRx1Buffer[UartRx1PutIndex++] = RXBUF1; 	// read the character
		UartRx1PutIndex &= UART1_RX_MASK ;
		++Uart1RxQueSize ;
		if (Uart1RxQueSize >= UART1_RX_MAX_QUE)
		{
			#ifdef UART1_HANDSH_EN
				UART1_CTS = NOT_READY ;
			#endif
		}
	}
}


/**************************************************************************//**
 * \brief UART1 Transmit ISR
 *
 * Send the next character if one is ready, otherwise disable the TX
 * interrupt (to be re-enabled later by uart0Putch())
 *****************************************************************************/
#pragma vector=USART1TX_VECTOR
__interrupt void uart1XmtIsr(void)
{
	if (UART1_RTS == READY)
	{
	    if (Uart1TxQueSize > 0)
	    {
	    	++Uart1TxGetIndex ;
	    	Uart1TxGetIndex &= UART1_TX_MASK ;
	      	--Uart1TxQueSize ;
	    	TXBUF1 = Uart1TxBuffer[Uart1TxGetIndex] ;
	    }
		else
		{
			U1IE &= ~UTXIE1 ;
			Uart1TxRunning = 0;
		}
	}
	else
	{
		U1IE &= ~UTXIE1 ;
	}
}

/**************************************************************************//**
 * Function Name: port1Isr(void)
 *
 * Description:
 *    port1 isr to Re-Enable Xmitter on Change of UART1_RTS
 *
 * Calling Sequence:
 *    void
 *
 * Returns:
 *    void
 *****************************************************************************/
/*#pragma vector=PORT2_VECTOR
__interrupt void port2Isr(void)
{
    P2IFG = 0 ; 		//Clear Interrupt source
	U1IE |= UTXIE1 ; 	//Turn on Xmitter interrupt
}
*/
#endif
