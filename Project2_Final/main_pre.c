#define ANY_DELAY_REQUIRED 0xFFFF

#include "stm32l476xx.h"
#include "SysClock.h"
#include "UART.h"

#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>

#define MOV 0x20
#define WAIT 0x40
#define LOOP 0x80
#define END_LOOP 0xA0
#define RECIPE_END 0x00


uint8_t recipe_1[65];		//holds each command byte
uint8_t recipe_2[65];


uint8_t counter_1 = 0;	//holds the counter for parsing recipes
uint8_t counter_2 = 0;	//holds the counter for parsing recipes
uint8_t wait_counter_1 = 0;
uint8_t wait_counter_2 = 0;

uint8_t LRwait_counter_1 = 0;
uint8_t LRwait_counter_2 = 0;

char user_in_1 = 0x00;
char user_in_2 = 0x00;


//used to track status of each servo
enum states{paused, running, moving, command_error, nested_error, finished, forced_move};	
enum states status_1 = paused;
enum states status_2 = paused;
uint8_t position_1 = 0;	//current position of each servo motor
uint8_t position_2 = 0;

uint8_t loop_index_1 = 0;
uint8_t loop_counter_1 = 0;
uint8_t loop_index_2 = 0;
uint8_t loop_counter_2 = 0;

uint8_t buffer[32];
uint8_t n;
char rxByte_1;
char rxByte_2;

void HW_config(void);
uint8_t checkForChar(char);
void set_servo_position(uint8_t, uint8_t);
void process_1(uint8_t);
void process_2(uint8_t);
void processUserInputs(char, char);
uint16_t readBytes(void);

int main(void){
	System_Clock_Init(); // Switch System Clock = 80 MHz
	UART2_Init();
	USART_Write(USART2, (uint8_t *)"Program Start\r\n>", 16);
	for(int i = 0; i < 64; i++){
		recipe_1[i] = 0;
		recipe_2[i] = 0;
	}
	
	recipe_1[0] = MOV | 0;
	recipe_1[1] = WAIT | 9;
	recipe_1[2] = MOV | 5;
	recipe_1[3] = WAIT | 9;
	recipe_1[4] = LOOP | 10;
	recipe_1[5] = MOV | 2;
	recipe_1[6] = WAIT | 3;
	recipe_1[7] = MOV | 4;
	recipe_1[8] = WAIT | 3;
	recipe_1[9] = END_LOOP;
	
	recipe_2[0] = MOV | 5;
	recipe_2[1] = WAIT | 9;
	recipe_2[2] = MOV | 0;
	recipe_2[3] = WAIT | 9;
	recipe_2[4] = LOOP | 10;
	recipe_2[5] = MOV | 4;
	recipe_2[6] = WAIT | 3;
	recipe_2[7] = MOV | 2;
	recipe_2[8] = WAIT | 3;
	recipe_2[9] = END_LOOP;
	
	
	//System_Clock_Init(); // Switch System Clock = 80 MHz
	HW_config();
	
	while(1){
		TIM2->CNT = 0;	//running at 1khz (1ms)
		
		if(status_1 == running || status_1 == moving){
			process_1(recipe_1[counter_1]);
		} else if(status_1 == forced_move){
			LRwait_counter_1++;
			if(LRwait_counter_1 == 2){
				LRwait_counter_1 = 0;
				status_1 = paused;
			}
		}
		
		if(status_2 == running || status_2 == moving){
			process_2(recipe_2[counter_2]);
		} else if(status_2 == forced_move){
			LRwait_counter_2++;
			if(LRwait_counter_2 == 2){
				LRwait_counter_2 = 0;
				status_2 = paused;
			}
		}
		
		if(status_1 == running){
			GPIOE->ODR |= GPIO_ODR_ODR_8;		//green on
			GPIOB->ODR &= ~GPIO_ODR_ODR_2;	//red off
		} else if(status_1 == paused){
			GPIOE->ODR &= ~GPIO_ODR_ODR_8;		//green off
			GPIOB->ODR &= ~GPIO_ODR_ODR_2;	//red off
		} else if(status_1 == command_error){
			GPIOE->ODR &= ~GPIO_ODR_ODR_8;		//green off
			GPIOB->ODR |= GPIO_ODR_ODR_2;	//red on
		} else if(status_1 == nested_error){
			GPIOE->ODR |= GPIO_ODR_ODR_8;		//green on
			GPIOB->ODR |= GPIO_ODR_ODR_2;	//red on
		}

		for(int i = 0; i < 5; i++){
			while(TIM2->CNT != 199){				//waits until 20ms five times
				if((USART2->ISR & USART_ISR_RXNE)){ //if data is in UART buffer
					if(!rxByte_1)
						rxByte_1 = USART_Read(USART2);	
					else if (!rxByte_2)
						rxByte_2 = USART_Read(USART2);
					else{
						processUserInputs(rxByte_1, rxByte_2);
					}
				}
			}
			while(TIM2->CNT != 0);	//this ensures the loop only repeats once per counter cycle
			
			
		}
		
	}
}


void process_1(uint8_t command){
	uint8_t parameter = command&0x1F;

	switch(command&0xE0){
		
		case WAIT:
			if(wait_counter_1 == parameter){	//when we've waited the defined amount of time
				counter_1++;		//wait is done, move on to the next command
				wait_counter_1 = 0;	//reset wait counter
			} else{
				wait_counter_1++;	
			}
			break;
			
		case MOV:
			if(parameter <= 5){	//unsigned datatype can't be less than 0, no point to check
				set_servo_position(1, parameter);
				
				if(wait_counter_1 == 2*(parameter - position_1) || wait_counter_1 == 2*(position_1 - parameter)){	//200ms per position change
					status_1 = running;
					position_1 = parameter;	//update position tracker
					counter_1++;		
					wait_counter_1 = 0;	
				} else{
					status_1 = moving;
					wait_counter_1++;	
					
				}
				
			} else{
				status_1 = command_error;
				//UART print: "Servo movement invalid, limits reached"
			}
			break;
		case LOOP:
			
		
			if(loop_index_1){
				status_1 = nested_error;
			} else{
				loop_index_1 = counter_1;
				loop_counter_1 = parameter;
				counter_1++;
			}
		
			
			break;
		case END_LOOP:
			
			
			if(loop_counter_1 == 0){
				loop_index_1 = 0;	//reset index, also acts as a flag
				counter_1++;
			} else{
				loop_counter_1--;
				counter_1 = loop_index_1+1;	//jump to right after the loop command
			}
		
			break;
			
		case RECIPE_END:
			status_1 = finished;
			break;
					
		default:
			status_1 = command_error;
			break;
	}
}

void process_2(uint8_t command){
	
	uint8_t parameter = command&0x1F;

	switch(command&0xE0){
		
		case WAIT:
			if(wait_counter_2 == parameter){	//when we've waited the defined amount of time
				counter_2++;		//wait is done, move on to the next command
				wait_counter_2 = 0;	//reset wait counter
			} else{
				wait_counter_2++;	
			}
			break;
			
		case MOV:
			if(parameter <= 5){	//unsigned datatype can't be less than 0, no point to check
				set_servo_position(2, parameter);
				
				if(wait_counter_2 == 2*(parameter - position_2) || wait_counter_2 == 2*(position_2 - parameter)){	//200ms per position change
					status_2 = running;
					position_2 = parameter;	//update position tracker
					counter_2++;		
					wait_counter_2 = 0;	
				} else{
					status_2 = moving;
					wait_counter_2++;	
					
				}
				
			} else{
				status_2 = command_error;
				//UART print: "Servo movement invalid, limits reached"
			}
			break;
		case LOOP:
			if(loop_index_2){
				status_2 = nested_error;
			} else{
				loop_index_2 = counter_2;
				loop_counter_2 = parameter;
				counter_2++;
			}
			break;
		case END_LOOP:
			if(loop_counter_2 == 0){
				loop_index_2 = 0;	//reset index, also acts as a flag
				counter_2++;
			} else{
				loop_counter_2--;
				counter_2 = loop_index_2+1;	//jump to right after the loop command
			}		
			break;
		
		case RECIPE_END:
			status_2 = finished;
			break;
			
		default:
			status_2 = command_error;
			break;
	}
}

void processUserInputs(char userIn_1, char userIn_2){
	
	USART_Write(USART2, (uint8_t *)">", 1);	
	
	switch(userIn_1){
		case 'p':
		case 'P':
			if(status_1 != nested_error || status_1 != command_error || status_1 != finished)
				status_1 = paused;
			break;
		case 'c':
		case 'C':
			if(status_1 != nested_error || status_1 != command_error || status_1 != finished)
				status_1 = running;
			break;
		case 'r':
		case 'R':
			if(status_1 == paused && position_1 != 0){
				status_1 = forced_move;
				set_servo_position(1, --position_1);	
			}
			break;
		case 'l':
		case 'L':
			if(status_1 == paused && position_1 != 5){
				status_1 = forced_move;
				set_servo_position(1, ++position_1);
			}
			break;
		case 'b':
		case 'B':
			status_1 = running;
			counter_1 = 0;
		default:
			break;
		}
	
	switch(userIn_2){
		case 'p':
		case 'P':
			if(status_1 != nested_error || status_1 != command_error || status_1 != finished)
				status_2 = paused;
			break;
		case 'c':
		case 'C':
			if(status_1 != nested_error || status_1 != command_error || status_1 != finished)
				status_2 = running;
			break;
		case 'r':
		case 'R':
			if(status_2 == paused && position_2 != 0){
				status_2 = forced_move;
				set_servo_position(2, --position_2);	
			}
			break;
		case 'l':
		case 'L':
			if(status_2 == paused && position_2 != 5){
				status_2 = forced_move;
				set_servo_position(2, ++position_2);
			}
			break;
		case 'b':
		case 'B':
			status_2 = running;
			counter_2 = 0;
		default:
			break;
		}	
		USART_Read(USART2);
		rxByte_1 = 0x00;
		rxByte_2 = 0x00;
}



void set_servo_position(uint8_t servo_num, uint8_t position){
	uint8_t duty_cycle = 0;
	
	switch(position){
		case 0:
			duty_cycle = 4;		// 2% DC  //0.4 ms
			break;
		case 1:
			duty_cycle = 7;		//	3.6% DC
			break;
		case 2:
			duty_cycle = 10;	//	5.2% DC
			break;
		case 3:
			duty_cycle = 14;	//	6.8% DC
			break;
		case 4:
			duty_cycle = 17;	//	8.4% DC
			break;
		case 5:
			duty_cycle = 20;	//	10% DC  //2.0ms
			break;
		default:
			break;
	}
	if(servo_num == 1)
		TIM2->CCR1 = duty_cycle;
	if(servo_num == 2)
		TIM2->CCR2 = duty_cycle;
}


