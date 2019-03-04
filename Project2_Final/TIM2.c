#include "stm32l476xx.h"
#include "SysClock.h"

void HW_config(void){
	
	RCC->AHB2ENR |= RCC_AHB2ENR_GPIOAEN;
	RCC->APB1ENR1 |= RCC_APB1ENR1_TIM2EN;
	
	RCC->AHB2ENR |= RCC_AHB2ENR_GPIOBEN;
	RCC->AHB2ENR |= RCC_AHB2ENR_GPIOEEN;

	// Set PB2 (red led) as output
	GPIOB->MODER &= ~(0x03<<(2*2)) ;
	GPIOB->MODER |= (1<<4);
		
	// Set PE8 (green led) as output
	GPIOE->MODER &= ~(0x03<<(2*8));		 
	GPIOE->MODER |= (1<<16);
		
	// Set PE8 and PB2 output type as push-pull
	GPIOE->OTYPER &= ~(0x100);
	GPIOB->OTYPER &= ~(0x4);
	
	// PE8 and PB2 output type as No pull-up no pull-down
	GPIOE->PUPDR &= ~(0x30000);
	GPIOB->PUPDR &= ~(0x30);
	
	
	GPIOA->MODER &= ~0X3;		//PA0
	GPIOA->MODER &= ~0xC;		//PA1
	GPIOA->MODER |= (1<<1) | (1<<3);	//pin A0, A1 alternate function mode
	GPIOA->AFR[0] |= (1<<0) | (1<<4);	//TIM2_CH1 and TIM2_CH2	(AF1)
	
	GPIOA->PUPDR &= ~(0x03);	//no pullup no pulldown
	GPIOA->PUPDR &= ~(0xC);	//no pullup no pulldown
	
	///////////////////////////////////////////////////////// 
	
	TIM2->PSC = 799; //prescaler (100kHz, 10us)
	TIM2->ARR = 1999; //auto reload register Period of 20 ms
	
	TIM2->CCMR1 = 0;
	TIM2->CCMR1 |= TIM_CCMR1_OC1M_1 | TIM_CCMR1_OC1M_2; //channel 1 pwm mode output
	TIM2->CCMR1 |= TIM_CCMR1_OC2M_1 | TIM_CCMR1_OC2M_2; //channel 2 pwm mode output
	TIM2->CCMR1 |= TIM_CCMR1_OC1PE | TIM_CCMR1_OC2PE; // preload enable


	TIM2->CR1 |= TIM_CR1_ARPE; //auto reload
	TIM2->CCER |= TIM_CCER_CC1E;	//enable output
	TIM2->CCER |= TIM_CCER_CC2E;	//enable output
	TIM2->EGR |= (1<<0); // trigger update
	TIM2->CR1 |= TIM_CR1_CEN;	//start timer
	
	 /////////////////TIMER5//////////////////////
	/*
	RCC->APB1ENR1 |= RCC_APB1ENR1_TIM5EN;
	GPIOA->AFR[0] |= (1<<1) | (1<<5);	//TIM5_CH1 and TIM5_CH2	(AF2)
	TIM5->PSC = 7999; //prescaler (10kHz, 10us)
	TIM5->ARR = 199; //auto reload register Period of 20 ms
	
	TIM5->CCMR1 = 0;
	TIM5->CCMR1 |= TIM_CCMR1_OC1M_1 | TIM_CCMR1_OC1M_2; //channel 1 pwm mode output
	TIM5->CCMR1 |= TIM_CCMR1_OC2M_1 | TIM_CCMR1_OC2M_2; //channel 2 pwm mode output
	TIM5->CCMR1 |= TIM_CCMR1_OC1PE | TIM_CCMR1_OC2PE; // preload enable


	TIM5->CR1 |= TIM_CR1_ARPE; //auto reload
	TIM5->CCER |= TIM_CCER_CC1E;	//enable output
	TIM5->CCER |= TIM_CCER_CC2E;	//enable output
	TIM5->EGR |= (1<<0); // trigger update
	TIM5->CR1 |= TIM_CR1_CEN;	//start timer
	*/
	/////////////////////////////////////////////////////
	
}

