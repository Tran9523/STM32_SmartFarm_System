#include "device_driver.h"

void ADC1_Init(void)
{
    // 1. 클럭 인가
    Macro_Set_Bit(RCC->APB2ENR, 8); // ADC1 CLK ON
    Macro_Set_Bit(RCC->AHB1ENR, 0); // GPIOA CLK ON

    // 2. PA6, PA7을 Analog 모드(0x3)로 설정
    Macro_Write_Block(GPIOA->MODER, 0xF, 0xF, 12); // PA6, PA7

    // 3. ADC On
    ADC1->CR2 = (1 << 0); // ADON
}

int ADC1_Read_Channel(int ch)
{
    // 채널 설정 (6: 수위, 7: 조도)
    ADC1->SQR3 = ch; 
    
    // 변환 시작
    Macro_Set_Bit(ADC1->CR2, 30); // SWSTART
    
    // 변환 완료 대기 (이 정도 대기는 매우 짧아 시스템에 지장 없음)
    while(!Macro_Check_Bit_Set(ADC1->SR, 1)); 
    
    return ADC1->DR;
}