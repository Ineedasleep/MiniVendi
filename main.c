/*
 * MiniVendi Project
 * Microcontroller 1 Code
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
#include "keypad.h"
#include "lcd.h"
#include "shiftreg.h" // For debugging purposes

// Global Functions
void ADC_init() {
	ADCSRA |= (1 << ADEN) | (1 << ADSC) | (1 << ADATE);
	// ADEN: setting this bit enables analog-to-digital conversion.
	// ADSC: setting this bit starts the first conversion.
	// ADATE: setting this bit enables auto-triggering. Since we are
	//        in Free Running Mode, a new conversion will trigger whenever
	//        the previous conversion completes.
}

/*************************** List of State Machines ***************************
 * LED: State machine to handle IR LED input to detect coin passing through
 *   detection mechanism. Updates global variable coinReceived.
 * 
 * Input_Logic: State machine to handle input from Bluetooth Module and keypad.
 *   Updates global variables productSelect and adminKey. Blocks further input
 *   after receiving until the system has successfully sent the signal to the
 *   second microcontroller.
 *
 * Product_Logic: State machine to process information from both LED and Input_
 *   Logic state machines. Determines what encoded unsigned char value to send
 *   to second microcontroller. Updates global variable control.
 *
 * Transmit: State machine to send encoded unsigned char value to second 
 *   microcontroller via USART. Solely responsible for sending value when
 *   Product_Logic SM updates value.
 */

 // Global Variables
 unsigned char coinReceived = 0;
 unsigned char productSelect = 0;
 unsigned char control = 0;
 // unsigned char AdminKey;
 unsigned char reset = 0;

enum LEDState {IR_INIT,IR_READ/*,IR_WAIT*/} led_state;
enum INState {IN_INIT,IN_RECEIVE,IN_SELECT1,IN_SELECT2,IN_BLOCK} in_state;
enum PLState {PL_INIT,PL_UPDATE} pl_state;
enum TRState {TR_INIT,TR_CHECKUPDATE,TR_TRANSMIT,TR_RESET} tr_state;

void LEDS_Init(){
	led_state = IR_INIT;
}

void IN_Init(){
	in_state = IN_INIT;
}

void PL_Init(){
	pl_state = PL_INIT;
}

void TR_Init(){
	tr_state = TR_INIT;
}

void LEDS_Tick(){
	//Local vars
	static unsigned short irVal = 0;
	static unsigned char str[10];
	//Actions
	switch(led_state){
		case IR_INIT:
			coinReceived = 0;
			break;
		case IR_READ:
			irVal = ADC;
			sprintf(str, "%d", ADC);
			LCD_AppendString(1, str);
			if (irVal > 900) {
				coinReceived = 0;
				/*LCD_AppendString(17, "No coin");*/
			} else {
				coinReceived = 1;
				/*LCD_AppendString(17, "Coin in");*/
			}
			/*LCD_AppendString(5, "1R");*/
			break;
// 		case IR_WAIT:
// 			LCD_AppendString(5, "1W");
// 			break;
		default:
			break;
	}
	//Transitions
	switch(led_state){
		case IR_INIT:
			led_state = IR_READ;
			break;
		case IR_READ:
// 			if(coinReceived)
// 				led_state = IR_WAIT;
// 			else
				led_state = IR_READ;
			break;
// 		case IR_WAIT:
// 			if(reset) {
// 				coinReceived = 0;
// 				led_state = IR_READ;
// 				/*LCD_AppendString(5, "1R");*/
// 			} else {
// 				led_state = IR_WAIT;
// 				/*LCD_AppendString(5, "1W");*/
// 			}
// 			break;
		default:
			led_state = IR_INIT;
			break;
	}
}

void IN_Tick(){
	//Local vars
	static unsigned char inputKey,BTinput;
	//Actions
	switch(in_state){
		case IN_INIT:
			productSelect = 0;
			inputKey = 0;
			BTinput = 0;
			break;
		case IN_RECEIVE:
			if(USART_HasReceived(0))
				BTinput = USART_Receive(0);
			inputKey = GetKeypadKey();
			break;
		case IN_SELECT1:
			productSelect = 1;
			break;
		case IN_SELECT2:
			productSelect = 2;
			break;
		case IN_BLOCK:
			break;
		default:
			break;
	}
	//Transitions
	switch(in_state){
		case IN_INIT:
			in_state = IN_RECEIVE;
			break;
		case IN_RECEIVE:
			if (inputKey == '1' || BTinput == '1')
				in_state = IN_SELECT1;
			else if (inputKey == '2' || BTinput == '2')
				in_state = IN_SELECT2;
			else
				in_state = IN_RECEIVE;
			break;
		case IN_SELECT1:
			in_state = IN_BLOCK;
			break;
		case IN_SELECT2:
			in_state = IN_BLOCK;
			break;
		case IN_BLOCK:
			if (reset) {
				in_state = IN_RECEIVE;
				productSelect = 0;
				BTinput = 0;
			} else {
				in_state = IN_BLOCK;
			}
			break;
		default:
			in_state = IN_INIT;
			break;
	}
}

void PL_Tick(){
	//Actions
	switch(pl_state){
		case PL_INIT:
			control = 0;
			break;
		case PL_UPDATE:
			if (coinReceived) {
				control |= 0x01; // Set bit 0 high to indicate a coin has been received
			} else {
				control &= 0xFE; // Set bit 0 low to indicate no coin received
			}
			switch(productSelect){
				case 0: control &= 0xF9; break;
				case 1: 
					control |= 0x02; // Set bit 1 high to indicate product1 selected
					break;
				case 2:
					control |= 0x04; // Set bit 2 high to indicate product2 selected
					break;
				default: control &= 0xF9; break;
			}
			break;
		default:
			break;
	}
	//Transition
	switch(pl_state){
		case PL_INIT:
			pl_state = PL_UPDATE;
			break;
		case PL_UPDATE:
			pl_state = PL_UPDATE;
			break;
		default:
			pl_state = PL_INIT;
			break;
	}
}

void TR_Tick(){
	//Local vars
	static unsigned char lastSent, delayCount;
	//Actions
	switch(tr_state){
		case TR_INIT:
			lastSent = 0;
			delayCount = 0;
			break;
		case TR_CHECKUPDATE:
			break;
		case TR_TRANSMIT:
			break;
		case TR_RESET:
			reset = 1;
			break;
		default:
			break;
	}
	//Transitions
	switch(tr_state){
		case TR_INIT:
			tr_state = TR_CHECKUPDATE;
			break;
		case TR_CHECKUPDATE:
			if (lastSent != control) {
				tr_state = TR_TRANSMIT;
				lastSent = control;
			} else {
				tr_state = TR_CHECKUPDATE;
			}
			break;
		case TR_TRANSMIT:
			if(USART_IsSendReady(1)) {
				USART_Send(control,1);
				tr_state = TR_RESET;
				transmit_data(control); // Show char being sent via USART
			} else {
				tr_state = TR_TRANSMIT;
			}
			break;
		case TR_RESET:
			if (delayCount < 10) {
				tr_state = TR_RESET;
				delayCount++;
			} else {
				tr_state = TR_CHECKUPDATE;
				reset = 0;
				delayCount = 0;
			}
			break;
		default:
			tr_state = TR_INIT;
			break;
	}
}

void LedSecTask()
{
	LEDS_Init();
	for(;;) 
	{ 	
		LEDS_Tick();
		vTaskDelay(5); 
	} 
}

void InputSecTask()
{
	IN_Init();
	for(;;)
	{
		IN_Tick();
		vTaskDelay(25);
	}
}

void ProductLogicSecTask()
{
	PL_Init();
	for(;;)
	{
		PL_Tick();
		vTaskDelay(50);
	}
}

void TransmitSecTask()
{
	TR_Init();
	for(;;)
	{
		TR_Tick();
		vTaskDelay(50);
	}
}

void StartSecPulse(unsigned portBASE_TYPE Priority)
{
	xTaskCreate(LedSecTask, (signed portCHAR *)"LedSecTask", configMINIMAL_STACK_SIZE, NULL, Priority, NULL );
	xTaskCreate(InputSecTask, (signed portCHAR *)"InputSecTask", configMINIMAL_STACK_SIZE, NULL, Priority, NULL );
	xTaskCreate(ProductLogicSecTask, (signed portCHAR *)"ProductLogicSecTask", configMINIMAL_STACK_SIZE, NULL, Priority, NULL );
	xTaskCreate(TransmitSecTask, (signed portCHAR *)"TransmitSecTask", configMINIMAL_STACK_SIZE, NULL, Priority, NULL );
}	
 
int main(void) 
{ 
	DDRA = 0xC0; PORTA = 0x3F; // ADC input
	DDRB = 0xFF; PORTB = 0x00; // For debugging
	DDRC = 0xF0; PORTC = 0x0F; // Keyboard hybrid
	DDRD = 0xFF; PORTD = 0x00; // USART output
   
	ADC_init();
	initUSART(0);
	initUSART(1);
	LCD_init();
	
	//Start Tasks  
	StartSecPulse(1);
	//RunSchedular 
	vTaskStartScheduler(); 
 
	return 0; 
}