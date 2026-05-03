#include "device_driver.h"

void Pump_Init(void)
{
    Macro_Set_Bit(RCC->AHB1ENR, 1); 
    Macro_Set_Bit(RCC->APB1ENR, 1); // TIM3 CLK ON

    Macro_Write_Block(GPIOB->MODER, 0x3, 0x2, 2); // PB1: AF 모드 (PWM)
    Macro_Write_Block(GPIOB->MODER, 0x3, 0x1, 4); // PB2: Out 모드 (DIR)
    Macro_Write_Block(GPIOB->AFR[0], 0xF, 0x2, 4); // PB1 -> AF2 (TIM3_CH4)

    // 96MHz / 960 = 100kHz 틱 (10us)
    TIM3->PSC = 960 - 1; 
    TIM3->ARR = 100 - 1; // 10us * 100 = 1ms (1kHz PWM)
    
    Macro_Write_Block(TIM3->CCMR2, 0xFF00, 0x6800, 0); // CH4 PWM Mode 1
    Macro_Set_Bit(TIM3->CCER, 12); // CH4 활성화
    Macro_Set_Bit(TIM3->CR1, 0);
}

void Pump_Set(int enable, int duty)
{
    if(enable) {
        Macro_Set_Bit(GPIOB->ODR, 2); // On
        TIM3->CCR4 = duty;
    } else {
        Macro_Clear_Bit(GPIOB->ODR, 2);
        TIM3->CCR4 = 0;               // Off
    }
}