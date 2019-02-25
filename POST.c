#include "stm32l476xx.h"
#include "SysClock.h"

uint8_t POST(){
	TIM2->CNT = 0;	//reset timer
	TIM2->CCR1 = 0;	//reset compare capture
	while(TIM2->CNT < 100000){	//check for captures within 100ms (counting in microseconds)
		if(TIM2->CCR1 > 0)
			return 0xFF;	//return success
	}
	return 0x00;	//return failure
}

