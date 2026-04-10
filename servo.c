#include "device_driver.h"

void Servo_Init(void)
{
    // 1. GPIOA 및 TIM2 클럭 활성화
    Macro_Set_Bit(RCC->AHB1ENR, 0); 
    Macro_Set_Bit(RCC->APB1ENR, 0); 

    // 2. PA0, PA1을 AF 모드(0x2)로 설정
    Macro_Write_Block(GPIOA->MODER, 0xF, 0xA, 0); // 0b1010
    Macro_Write_Block(GPIOA->AFR[0], 0xFF, 0x11, 0); // AF1(TIM2) 맵핑

    // 3. 50Hz (20ms 주기) 생성
    TIM2->PSC = 96 - 1;      // 1us 틱
    TIM2->ARR = 20000 - 1;   // 20ms 주기

    // 4. CH1(PA0), CH2(PA1) PWM 모드 1 설정
    Macro_Write_Block(TIM2->CCMR1, 0xFFFF, 0x6868, 0); 
    Macro_Set_Bit(TIM2->CCER, 0); // CH1 출력 활성화
    Macro_Set_Bit(TIM2->CCER, 4); // CH2 출력 활성화
    
    Macro_Set_Bit(TIM2->CR1, 0);  // 카운터 시작
}

void Servo_Door_Set(int angle) {
    TIM2->CCR1 = 500 + (angle * 2000 / 180); 
}

void Servo_Hose_Set(int angle) {
    TIM2->CCR2 = 500 + (angle * 2000 / 180);
}

void Servo_Door_Enable(void)
{
    Macro_Set_Bit(TIM2->CCER, 0);
}

void Servo_Door_Disable(void)
{
    Macro_Clear_Bit(TIM2->CCER, 0);
}