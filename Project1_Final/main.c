#define ANY_DELAY_REQUIRED 0xFFFF

#include "stm32l476xx.h"
#include "SysClock.h"
#include "UART.h"

#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>

uint8_t buffer[32];
char strSuccess[] = "POST success\r\n";
char strFailure[] = "POST failure, Y to try again\r\n";
char strPromptChange[] = "Y to change limits, any other key to keep\r\n";

uint32_t timeStampPre, timeStampSecond, timeStampFirst;
int recordings[100];
uint8_t tryAgain = 0x00;
uint8_t tryProgramAgain = 0x00;
uint8_t printOnce = 0xFF;
char rxByte;

int n;
int lowerLim = 950;
int newLim;


void TIM2_config(void);
uint8_t POST(void);
uint8_t checkForChar(char);
void USART_printInt(int);
int readValue(void);
void printArray(int*,uint8_t);

int main(void){
	
	System_Clock_Init(); // Switch System Clock = 80 MHz
	UART2_Init();
	TIM2_config();
	TIM2->CR1 |= TIM_CR1_CEN;	//start timer
	
	USART_Write(USART2, (uint8_t *)"\r\n\r\n", 6);
	
	// ----- Power On Self Test -------
	if(!POST())
		tryAgain = 0xFF;
	
	while(tryAgain){
		USART_Write(USART2, (uint8_t *)strFailure, strlen(strFailure));
		if(checkForChar('Y')){
			tryAgain = ~POST();	//try again if post returns false (unsuccessful)
		} else{
			USART_Write(USART2, (uint8_t *)"Program terminated", 18+2);
			while(1);	//wait forever until reset
		}
	}
	USART_Write(USART2, (uint8_t *)strSuccess, strlen(strSuccess));
	// --------------------------------
	
	
	//Report the current limits
	USART_Write(USART2, (uint8_t *)"Lower Limit = ", 14+2);
	USART_printInt(lowerLim);
	USART_Write(USART2, (uint8_t *)"\r\n", 2);
	USART_Write(USART2, (uint8_t *)"Upper Limit = ", 14+2);
	USART_printInt(lowerLim+100);
	USART_Write(USART2, (uint8_t *)"\r\n", 2);
	
	do{	//repetition point if user restarts program
		tryAgain = 0x00;	//make sure this is reset
		newLim = 0;
		
	// ----- Altering the bounds -------
		USART_Write(USART2, (uint8_t *)strPromptChange, strlen(strPromptChange));
		
		if(checkForChar('Y')){
			USART_Write(USART2, (uint8_t *)"Enter new lower limit: ", 23);
			newLim = readValue();		
			
			if (newLim < 50 || newLim > 9950){
				tryAgain = 0xFF;
				USART_Write(USART2, (uint8_t *)"Invalid limit, enter a value between 50 and 9950: ", 50);
				
				while(tryAgain){
					newLim = readValue();
					if(newLim < 50 || newLim > 9950){
						USART_Write(USART2, (uint8_t *)"Invalid limit, enter a value between 50 and 9950: ", 50);
					} else{
						tryAgain = 0x00;
					}
				} //endwhile
			}
			lowerLim = newLim;
		}
	// --------------------------------
		
	//Report the limits
	USART_Write(USART2, (uint8_t *)"Lower Limit = ", 14+2);
	USART_printInt(lowerLim);
	USART_Write(USART2, (uint8_t *)"\r\n", 2);
	USART_Write(USART2, (uint8_t *)"Upper Limit = ", 14+2);
	USART_printInt(lowerLim+100);
	USART_Write(USART2, (uint8_t *)"\r\n", 2);
	
	USART_Write(USART2, (uint8_t *)"Press Enter to Start", 20);
	while(USART_Read(USART2) != '\r');		//wait for enter
	
		USART_Write(USART2, (uint8_t *)"Start program\r\n", 15);
		
		TIM2->CNT = 0;	//reset timer
		TIM2->CCR1 = 0;	//reset compare capture
		for(int i = 0; i < 100; i++){	//reset recordings
			recordings[i] = 0;
		}
		
		timeStampPre = TIM2->CCR1;	//not used in calculation
		
		while(TIM2->CCR1 == timeStampPre);	//wait for the next event
		timeStampFirst = TIM2->CCR1;
		while(TIM2->CCR1 == timeStampFirst);	//wait for next event
		timeStampSecond = TIM2->CCR1;	
		recordings[(timeStampSecond-timeStampFirst) - lowerLim]++;	//mark the recording, normalized to lowerLim
		
		for(int i = 1; i < 1000; i++){	//starting after first measurement, do remaining 999
			timeStampPre = timeStampSecond;		//reset for next measurement
			while(TIM2->CCR1 == timeStampPre);	//wait for the next event
			timeStampFirst = TIM2->CCR1;
			while(TIM2->CCR1 == timeStampFirst);	//wait for next event
			timeStampSecond = TIM2->CCR1;	
			
			recordings[(timeStampSecond-timeStampFirst) - lowerLim]++;	//mark the recording, normalized to lowerLim
		}
		
		printArray(recordings, 100);	//print all of recordings array 
		
		USART_Write(USART2, (uint8_t *)"Program complete. Y to restart\n\r", 32);
		
		tryProgramAgain = 0xFF;
		if(!checkForChar('Y'))
			tryProgramAgain = 0x00;
		
	} while(tryProgramAgain);
	
	USART_Write(USART2, (uint8_t *)"Program terminated", 18);
	
	while(1);	//wait forever
}




int readValue(){
	char inputStr[32];
	for(int i = 0; i < 32; i++){
		inputStr[i] = 0;
	}
	
	int i;
	for(i = 0; i < 32; i++){
		rxByte = USART_Read(USART2);
		if(rxByte == '\r')
			break;
		
		inputStr[i] = rxByte;
	}
	return atoi(inputStr);
}

uint8_t checkForChar(char input){
	rxByte = 0;
	rxByte = USART_Read(USART2);

	USART_Read(USART2);	//read away enter char
	if(rxByte == input){
		return 0xFF;
	} else{
		return 0x00;
	}
}

void USART_printInt(int value){
	n = sprintf((char *)buffer, "%d\t", value);
	USART_Write(USART2, (uint8_t *)buffer, n);
}

void printArray(int *inputArr, uint8_t len){
	for(int i = 0; i < len-1; i++){
		if(inputArr[i] != 0){
			n = sprintf((char *)buffer, "%d: %d\t\r\n", lowerLim+i, inputArr[i]);
			USART_Write(USART2, (uint8_t *)buffer, n);
		}
	}
}


