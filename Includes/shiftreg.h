// Permission to copy is granted provided that this header remains intact. 
// This software is provided with no warranties.

#ifndef SHIFTREG_H
#define SHIFTREG_H

#define SET_BIT(p,i) ((p) |= (1 << (i)))
#define CLR_BIT(p,i) ((p) &= ~(1 << (i)))
#define GET_BIT(p,i) ((p) & (1 << (i)))

/*-------------------------------------------------------------------------*/

#define DATAPORT PORTA		// port connected to shift register
#define SER 4				// pin number of uC connected to SER pin
#define SRCLR 7				// pin number of uC connected to SRCLR pin
#define SRCLK 6				// pin number of uC connected to SRCLK pin
#define RCLK 5				// pin number of uC connected to RCLK pin

/*-------------------------------------------------------------------------*/

// transmits to 1 shift register
// void transmit_data(unsigned char data) {
// 	/*---------------- Serial read ----------------*/
// 	signed char i;
// 	// Sets SRCLR(A7) to 1 allowing data to be set
// 	DATAPORT |= (1<<SRCLR);
// 	// Sets RCLK (A5) to 0 to prevent preemptive latching
// 	CLR_BIT(DATAPORT,RCLK);
	
// 	for (i = 7; i >= 0; i--) {
// 		// Clears SRCLK(A6) and SER(A4) in preparation of sending data
// 		CLR_BIT(DATAPORT,SRCLK); CLR_BIT(DATAPORT,SER);
// 		// set SER(A4) = next bit of data to be sent
// 		DATAPORT |= (((data>>i) & 0x01) << SER);
// 		// set SRCLK(A6) = 1. Rising edge shifts next bit of data into the shift register
// 		SET_BIT(DATAPORT,SRCLK);
// 	}

// 	/*---------------- Parallel Load ----------------*/
// 	// set RCLK(A5) = 1. Rising edge copies data from "Shift" register to "Storage" register
// 	SET_BIT(DATAPORT,RCLK);
// 	// clears all lines in preparation of a new transmission
// 	CLR_BIT(DATAPORT,SER); CLR_BIT(DATAPORT,SRCLR); CLR_BIT(DATAPORT,SRCLK); CLR_BIT(DATAPORT,RCLK);
// }

// transmits to 1 shift register connected to A4-7:
	// PA4: SER
	// PA5: RCLK
	// PA6: SRCLK
	// PA7: SRCLR
void transmit_data(unsigned char data) {
	/*---------------- Serial read ----------------*/
	signed char i;
	PORTD |= 0x80;
	// First char
	for (i = 7; i >= 0; i--) {
		// Sets SRCLR(A7) to 1 allowing data to be set
		// Also clears SRCLK(A6) in preparation of sending data
		PORTD &= 0x8F;
		// set SER(A4) = next bit of data to be sent
		PORTD |= (((data>>i) & 0x01) << 4);
		// set SRCLK(A6) = 1. Rising edge shifts next bit of data into the shift register
		PORTD |= 0x40;
	}

	/*---------------- Parallel Load ----------------*/
	// set RCLK(A5) = 1. Rising edge copies data from "Shift" register to "Storage" register
	PORTD |= 0x20;
	// clears all lines in preparation of a new transmission
	PORTD &= 0x0F;
}


// // transmits to 3 daisy-chained shift registers connected to A4-7
// void transmit_3_data(unsigned char data1, unsigned char data2, unsigned char data3) {
// 	/*---------------- Serial read ----------------*/
// 	int i;
// 	PORTA |= 0x80;
// 	// First char
// 	for (i = 7; i >= 0; i--) {
// 		// Sets SRCLR(A7) to 1 allowing data to be set
// 		// Also clears SRCLK(A6) in preparation of sending data
// 		PORTA &= 0x8F;
// 		// set SER(A4) = next bit of data to be sent
// 		PORTA |= (((data1>>i) & 0x01) << 4);
// 		// set SRCLK(A6) = 1. Rising edge shifts next bit of data into the shift register
// 		PORTA |= 0x50;
// 	}
// 	// Second char
// 	for (i = 7; i >= 0; i--) {
// 		// Sets SRCLR(A7) to 1 allowing data to be set
// 		// Also clears SRCLK(A6) in preparation of sending data
// 		PORTA &= 0x8F;
// 		// set SER(A4) = next bit of data to be sent
// 		PORTA |= (((data2>>i) & 0x01) << 4);
// 		// set SRCLK(A6) = 1. Rising edge shifts next bit of data into the shift register
// 		PORTA |= 0x50;
// 	}
// 	// Third char
// 	for (i = 7; i >= 0; i--) {
// 		// Sets SRCLR(A7) to 1 allowing data to be set
// 		// Also clears SRCLK(A6) in preparation of sending data
// 		PORTA &= 0x8F;
// 		// set SER(A4) = next bit of data to be sent
// 		PORTA |= (((data3>>i) & 0x01) << 4);
// 		// set SRCLK(A6) = 1. Rising edge shifts next bit of data into the shift register
// 		PORTA |= 0x50;
// 	}

// 	/*---------------- Parallel Load ----------------*/
// 	// set RCLK(A5) = 1. Rising edge copies data from "Shift" register to "Storage" register
// 	PORTA |= 0x20;
// 	// clears all lines in preparation of a new transmission
// 	PORTA = 0x00;
// }

#endif