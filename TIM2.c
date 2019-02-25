#include "stm32l476xx.h"
#include "SysClock.h"

void TIM2_config(void){
	
	RCC->AHB2ENR |= RCC_AHB2ENR_GPIOAEN;
	RCC->APB1ENR1 |= RCC_APB1ENR1_TIM2EN;
	GPIOA->MODER &= ~0X3;
	GPIOA->MODER |= (1<<1);	//pin 1 alternate function
	GPIOA->AFR[0] |= (1<<0);	//TIM2_CH1
	
	
	
	TIM2->PSC = 79; //prescaler
	TIM2->EGR |= (1<<0); // trigger update
	
	TIM2->CCER &= ~TIM_CCER_CC1E;
	
	TIM2->CCMR1 &= ~TIM_CCMR1_CC1S;	//direction as input
	TIM2->CCMR1 |= 0X01;	// CC1 to Timer Input 1
	
	TIM2->CCER &= ~(1<<1);
	TIM2->CCER &= ~(1<<3);	// trigger on rising edge
	
	TIM2->CCER |= TIM_CCER_CC1E; //capture enable
}