/*
 * MiniVendi Project
 * Microcontroller 2 Code
 *
 * Created: 11/2/2017 7:31:03 PM
 * Author : Ethan Valdez
 */ 

#include <stdint.h> 
#include <stdlib.h> 
#include <stdio.h> 
#include <stdbool.h> 
#include <string.h> 
#include <math.h> 
#include <avr/io.h> 
#include <avr/interrupt.h> 
#include <avr/eeprom.h> 
#include <avr/portpins.h> 
#include <avr/pgmspace.h> 
 
//FreeRTOS include files 
#include "FreeRTOS.h" 
#include "task.h" 
#include "croutine.h" 

//Other include files
#include "usart_ATmega1284.h"
#include "lcd.h"
#include "shiftreg.h" // For debugging purposes


/************************* List of State Machines ****************************
 * Stepper_Driver: State machine to drive stepper motors in full-step drive to
 *   rotate coil to dispense an item if the machine has determined there is 
 *   enough money in the machine. Updates global variable motorRunning
 * 
 * LCD_Logic: State machine to drive LCD screen to display information about
 *   the current status of the machine. Will display current amount of money
 *   the machine contains and will give feedback for when customer attempts
 *   to purchase an item.
 *
 * Product_Output: State machine to process information from first 
 *   microcontroller and determines the validity of purchases. Updates global 
 *   variables productValid and currentCoins.
 *
 */

/************************* Global Functions *************************/

// NOTE: This function will fail to display values greater than or equal to $10
void LCD_DisplayCoins(unsigned char coinCnt) {
	// First position: Column 11
	unsigned char dollars = coinCnt / 4; // integer division gives how many dollars
											// per 4 quarters
	unsigned char cents = (coinCnt % 4) * 25; // Modulus gives how many quarters if  
											// not rounded to whole dollar
	
	LCD_Cursor(11);
	LCD_WriteData('0' + dollars);		// Writes dollar val
	LCD_WriteData('.');					// Decimal pt
	LCD_WriteData('0' + (cents / 10));	// 10's place of cents
	LCD_WriteData('0' + (cents % 10));	// 1's place of cents
}

/************************* Global Variables *************************/
unsigned char currentCoins = 0;
unsigned char productValid = 0;
unsigned char motorRunning = 0;
const unsigned short PHASES_TO_DISPENSE = (360/11.25)*64;
const unsigned char PRICE1 = 1;
const unsigned char PRICE2 = 2;
const unsigned char sequence1[4] = {0x30,0x60,0xC0,0x90}; // Full-step drive on upper nibble
const unsigned char sequence2[4] = {0x03,0x06,0x0C,0x09}; // Full-step drive on lower nibble

/************************* State Machines *************************/

enum SDState {SD_INIT,SD_PROCESS,SD_DRIVE1,SD_DRIVE2,SD_FINISH} sd_state;
enum LCDState {LCD_INIT,LCD_WELCOME,LCD_COINCNT,LCD_DISPENSE1,LCD_DISPENSE2,
				LCD_INSUFFICIENT,LCD_THANKYOU} lcd_state;
enum POState {PO_INIT,PO_RECEIVE} po_state;

void SD_Init(){
	sd_state = SD_INIT;
}

void LCD_Init(){
	lcd_state = LCD_INIT;
}

void PO_Init(){
	po_state = PO_INIT;
}

void SD_Tick(){
	//Local vars
	static unsigned char stepperOut, pos1, pos2;
	static unsigned short motorCount;
	//Actions
	switch(sd_state){
		case SD_INIT:
			motorRunning = 0;
			stepperOut = 0;
			motorCount = 0;
			pos1 = 0;
			pos2 = 0;
			break;
		case SD_PROCESS:
			break;
		case SD_DRIVE1:
			motorRunning = 1;
			stepperOut = sequence1[pos1];
			if (pos1 < 3)
				pos1++;
			else
				pos1 = 0;
			break;
		case SD_DRIVE2:
			motorRunning = 1;
			stepperOut = sequence2[pos2];
			if (pos2 < 3)
				pos2++;
			else
				pos2 = 0;
			break;
		case SD_FINISH:
			motorRunning = 0;
			productValid = 0; // Need to clear for rest of state machines (handshake)
			break;
		default:
			break;
	}

	PORTB = stepperOut;

	//Transitions
	switch(sd_state){
		case SD_INIT:
			sd_state = SD_PROCESS;
			break;
		case SD_PROCESS:
			if (productValid == 0 || productValid == 2 || productValid == 4)
				sd_state = SD_PROCESS;
			else if (productValid == 3)
				sd_state = SD_DRIVE1;
			else if (productValid == 5)
				sd_state = SD_DRIVE2;
			break;
		case SD_DRIVE1:
			if (motorCount < PHASES_TO_DISPENSE) {
				sd_state = SD_DRIVE1;
				motorCount++;
			} else {
				sd_state = SD_FINISH;
				motorCount = 0;
			}
			break;
		case SD_DRIVE2:
			if (motorCount < PHASES_TO_DISPENSE) {
				sd_state = SD_DRIVE2;
				motorCount++;
			} else {
				sd_state = SD_FINISH;
				motorCount = 0;
			}
			break;
		case SD_FINISH:
			sd_state = SD_PROCESS;
			break;
		default:
			sd_state = SD_INIT;
			break;
	}
}

void LCD_Tick(){
	//Local vars
	static unsigned short timer;
	//Actions
	switch(lcd_state){
		case LCD_INIT:
			timer = 0;
			break;
		case LCD_WELCOME:
			break;
		case LCD_COINCNT:
			// Only updates coin value each tick
			// First part of message written on transition
			LCD_DisplayCoins(currentCoins);
			break;
		case LCD_DISPENSE1:
			break;
		default:
			break;
	}
	//Transitions
	switch(lcd_state){
		case LCD_INIT:
			lcd_state = LCD_WELCOME;
			LCD_DisplayString(1, "Welcome to ");
			LCD_AppendString(17, "MiniVendi!");
			break;
		case LCD_WELCOME:
			if (timer < 40) {
				lcd_state = LCD_WELCOME;
				timer++;
			} else {
				lcd_state = LCD_COINCNT;
				timer = 0;
				LCD_DisplayString(1, "Balance: $");
			}
			break;
		case LCD_COINCNT:
			if (productValid == 0x03) {
				lcd_state = LCD_DISPENSE1;
				LCD_DisplayString(1, "Dispensing");
				LCD_AppendString(17, "<PRODUCT1>");
			} else if (productValid == 0x05) {
				lcd_state = LCD_DISPENSE2;
				LCD_DisplayString(1, "Dispensing");
				LCD_AppendString(17, "<PRODUCT2>");
			} else if (productValid == 0x02 || productValid == 0x04) {
				lcd_state = LCD_INSUFFICIENT;
				LCD_DisplayString(1, "INSUFFICIENT");
				LCD_AppendString(17, "FUNDS");
			} else {
				lcd_state = LCD_COINCNT;
			}
			break;
		case LCD_DISPENSE1:
			if (motorRunning) {
				lcd_state = LCD_DISPENSE1;
			} else {
				lcd_state = LCD_THANKYOU;
				LCD_DisplayString(1, "THANK YOU FOR");
				LCD_AppendString(17, "YOUR PURCHASE!");
			}
			break;
		case LCD_DISPENSE2:
			if (motorRunning) {
				lcd_state = LCD_DISPENSE2;
			} else {
				lcd_state = LCD_THANKYOU;
				LCD_DisplayString(1, "THANK YOU FOR");
				LCD_AppendString(17, "YOUR PURCHASE!");
			}
			break;
		case LCD_INSUFFICIENT:
			if (timer < 60) {
				lcd_state = LCD_INSUFFICIENT;
				timer++;
			} else {
				lcd_state = LCD_COINCNT;
				timer = 0;
				productValid = 0; // Reset from this state
				LCD_DisplayString(1, "Balance: $");
			}
			break;
		case LCD_THANKYOU:
			if (timer < 60) {
				lcd_state = LCD_THANKYOU;
				timer++;
			} else {
				lcd_state = LCD_COINCNT;
				timer = 0;
				LCD_DisplayString(1, "Balance: $");
			}
			break;
		default:
			lcd_state = LCD_INIT;
			break;
	}
}

void PO_Tick(){
	//Local variables
	static unsigned char receivedControl;
	//Actions
	switch(po_state){
		case PO_INIT:
			receivedControl = 0;
			break;
		case PO_RECEIVE:
			if(USART_HasReceived(1)) {
				receivedControl = USART_Receive(1);
				if (receivedControl & 0x01) // Received coin
					currentCoins++;
				if (receivedControl & 0x02) { // Selected Product1
					if (currentCoins >= PRICE1) {
						productValid = 0x03; // Product1 valid to dispense
						currentCoins -= PRICE1;
					} else {
						productValid = 0x02; // Product1 not valid to dispense
					}
				} else if (receivedControl & 0x04) { // Selected Product2
					if (currentCoins >= PRICE2) {
						productValid = 0x05; // Product2 valid to dispense
						currentCoins -= PRICE2;
					} else {
						productValid = 0x04; // Product2 not valid to dispense
					}
				}
				receivedControl = 0;
			}
			break;
		default:
			break;
	}
	//Transition
	switch(po_state){
		case PO_INIT:
			po_state = PO_RECEIVE;
			break;
		case PO_RECEIVE:
			po_state = PO_RECEIVE;
			break;
		default:
			po_state = PO_INIT;
			break;
	}
}


void StepperSecTask()
{
	SD_Init();
	for(;;) 
	{ 	
		SD_Tick();
		vTaskDelay(3); 
	} 
}

void LCDSecTask()
{
	LCD_Init();
	for(;;)
	{
		LCD_Tick();
		vTaskDelay(50);
	}
}

void ProductOutputSecTask()
{
	PO_Init();
	for(;;)
	{
		PO_Tick();
		vTaskDelay(25);
	}
}

void StartSecPulse(unsigned portBASE_TYPE Priority)
{
	xTaskCreate(StepperSecTask, (signed portCHAR *)"StepperSecTask", configMINIMAL_STACK_SIZE, NULL, Priority, NULL );
	xTaskCreate(LCDSecTask, (signed portCHAR *)"LCDSecTask", configMINIMAL_STACK_SIZE, NULL, Priority, NULL );
	xTaskCreate(ProductOutputSecTask, (signed portCHAR *)"ProductOutputSecTask", configMINIMAL_STACK_SIZE, NULL, Priority, NULL );
}	
 
int main(void) 
{ 
	DDRA = 0xFF; PORTA = 0x00; // LCD Control
	DDRB = 0xFF; PORTB = 0x00; // For Steppers
	DDRC = 0xFF; PORTC = 0x00; // LCD Data
	DDRD = 0xFC; PORTD = 0x03; // USART input, SR for debugging
   
	initUSART(1);
	LCD_init();
	
	//Start Tasks  
	StartSecPulse(1);
	//RunSchedular 
	vTaskStartScheduler(); 
 
	return 0; 
}