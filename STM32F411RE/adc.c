#include "device_driver.h"

void ADC1_Init(void)
{
    Macro_Set_Bit(RCC->APB2ENR, 8); // ADC1 CLK ON
    Macro_Set_Bit(RCC->AHB1ENR, 0); // GPIOA CLK ON

    Macro_Write_Block(GPIOA->MODER, 0xF, 0xF, 12); // PA6, PA7

    ADC1->CR2 = (1 << 0); // ADON
}

int ADC1_Read_Channel(int ch)
{
    // 채널 설정 (6: 수위, 7: 조도)
    ADC1->SQR3 = ch; 
    
    Macro_Set_Bit(ADC1->CR2, 30);
    
    while(!Macro_Check_Bit_Set(ADC1->SR, 1)); 
    
    return ADC1->DR;
}