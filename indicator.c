#include "device_driver.h"

void Indicator_Init(void)
{
    // 1. 보조 환경 LED (PC0, PC1, PC2) 초기화
    Macro_Set_Bit(RCC->AHB1ENR, 2); 
    Macro_Write_Block(GPIOC->MODER, 0x3F, 0x15, 0); // PC0,1,2 Output (010101)
    GPIOC->ODR &= ~((1<<0)|(1<<1)|(1<<2)); // 초기 끄기

    // 2. 상태 RGB LED (PA5, PA8, PA9) 초기화
    Macro_Set_Bit(RCC->AHB1ENR, 0); 
    Macro_Write_Block(GPIOA->MODER, 0x3, 0x1, 10); // PA5 Output
    Macro_Write_Block(GPIOA->MODER, 0x3, 0x1, 16); // PA8 Output
    Macro_Write_Block(GPIOA->MODER, 0x3, 0x1, 18); // PA9 Output
    GPIOA->ODR &= ~((1<<5)|(1<<8)|(1<<9)); // 초기 끄기

    // 3. 부저 (PB0 - TIM3_CH3) 설정
    // ※ 주의: TIM3 클럭 활성화와 기본 설정(PSC=960-1)은 pump.c에서 이미 수행됨!
    Macro_Set_Bit(RCC->AHB1ENR, 1);
    Macro_Set_Bit(RCC->APB1ENR, 1);
    Macro_Write_Block(GPIOB->MODER, 0x3, 0x2, 0);  // PB0 AF 모드
    Macro_Write_Block(GPIOB->AFR[0], 0xF, 0x2, 0); // PB0 -> AF2 (TIM3_CH3)
    Macro_Write_Block(TIM3->CCMR2, 0xFF, 0x68, 0); // CH3 PWM Mode 1
    Macro_Set_Bit(TIM3->CCER, 8); // CH3 활성화
}

// 보조 LED 1~3개 제어 (cnt: 0~3)
void Env_LED_Set(int cnt) {
    GPIOC->ODR &= ~((1<<0)|(1<<1)|(1<<2)); // 다 끄기
    if(cnt >= 1) Macro_Set_Bit(GPIOC->ODR, 0);
    if(cnt >= 2) Macro_Set_Bit(GPIOC->ODR, 1);
    if(cnt >= 3) Macro_Set_Bit(GPIOC->ODR, 2);
}

// RGB LED 제어 (예: 1=R, 2=G, 3=B)
void RGB_LED_Set(int color) {
    GPIOA->ODR &= ~((1<<5)|(1<<8)|(1<<9)); // 다 끄기
    if(color == 1) Macro_Set_Bit(GPIOA->ODR, 5); // Red
    if(color == 2) Macro_Set_Bit(GPIOA->ODR, 8); // Green
    if(color == 3) Macro_Set_Bit(GPIOA->ODR, 9); // Blue
}

// 부저 제어 (주파수 입력, 0이면 정지)
void Buzzer_Set(int freq) {
    if(freq == 0) {
        TIM3->CCR3 = 0;       // 소리 끄기
        TIM3->ARR = 100 - 1;  // 펌프의 기본 ARR(1ms)로 원상복구!
    } else {
        // pump.c에서 설정한 100kHz 틱 기준 역산 (예: 2000Hz = 50-1)
        TIM3->ARR = (100000 / freq) - 1; 
        TIM3->CCR3 = TIM3->ARR / 2; // Duty 50%
    }
}