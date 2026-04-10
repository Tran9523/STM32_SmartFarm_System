#include "device_driver.h"

void TIM4_Repeat_Interrupt_Enable(int time)
{
    Macro_Set_Bit(RCC->APB1ENR, 2); // TIM4 클럭 ON

    // 96MHz / 96 = 1MHz (1us 틱)
    TIM4->PSC = 96 - 1; 
        
    // time(ms)를 1000배 하여 us로 변환
    TIM4->ARR = (time * 1000) - 1; 

    Macro_Set_Bit(TIM4->DIER, 0); // Update Interrupt Enable
        
    // NVIC (Nested Vectored Interrupt Controller)에서 TIM4 인터럽트(IRQ 30) 허용
    NVIC->ISER[0] = (1 << 30); 
        
    Macro_Set_Bit(TIM4->CR1, 0);  // 타이머 카운터 시작
}