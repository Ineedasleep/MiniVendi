#ifndef SPI_1284_H
#define SPI_1284_H

// Global Variables
unsigned char SPI_MasterState;
unsigned char receivedData;

// Master code
void SPI_MasterInit(void) {
	// Set DDRB to have MOSI, SCK, and SS as output and MISO as input
	DDRB = (1<<DDB2) | (1<<DDB3) | (1<<DDB4) |(1<<DDB5) | (1<<DDB7);
	// Set SPCR register to enable SPI, enable master, and use SCK frequency
	//   of fosc/16  (pg. 168)
	SPCR = (1<<SPE) | (1<<MSTR) | (1<<SPR0);
	// Make sure global interrupts are enabled on SREG register (pg. 9)
	SREG |= 0x80;
}

void SPI_MasterTransmit(unsigned char cData) {
	// data in SPDR will be transmitted, e.g. SPDR = cData;
	SPDR = cData;
	// set SS low
	SPI_MasterState &= 0xEF; // B4 low
	PORTB = SPI_MasterState; // Clear SS
	
	while(!(SPSR & (1<<SPIF))) { // wait for transmission to complete
		;
	}

	// set SS high
	SPI_MasterState |= 0x1C; // B4 high
	PORTB = SPI_MasterState;
}


// Servant code
void SPI_ServantInit(void) {
	// set DDRB to have MISO line as output and MOSI, SCK, and SS as input
	DDRB = (1<<DDB6);
	// set SPCR register to enable SPI and enable SPI interrupt (pg. 168)
    SPCR = (1<<SPE) | (1<<SPIE);
	// make sure global interrupts are enabled on SREG register (pg. 9)
    SREG |= 0x80;	// 0x80: 1000000
}

ISR(SPI_STC_vect) { // this is enabled in with the SPCR register’s “SPI
	// Interrupt Enable”
	// SPDR contains the received data, e.g. unsigned char receivedData =
	// SPDR;
    receivedData = SPDR;
}

#endif