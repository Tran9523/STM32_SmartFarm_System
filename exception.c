#include "device_driver.h"
#include <stdio.h>

// 전역 변수 선언
volatile int Emergency_Flag = 0;       

// 화재 센서 카운터
volatile unsigned int fire_detect_count = 0; 

extern volatile char Uart_Data;
extern volatile int Uart_Data_In;

void _Invalid_ISR(void)
{
    unsigned int r = Macro_Extract_Area(SCB->ICSR, 0x1ff, 0);
    printf("\nInvalid_Exception: %d!\n", r);
    printf("Invalid_ISR: %d!\n", r - 16);
    for(;;); 
}

// 시스템 틱(Tick) 타이머 인터럽트 (TIM4) - 1ms 주기
void TIM4_IRQHandler(void)
{
    if (Macro_Check_Bit_Set(TIM4->SR, 0))
    {
        Macro_Clear_Bit(TIM4->SR, 0); 
        System_Tick++;  

        if (Emergency_Flag == 0) 
        {
            if (Macro_Check_Bit_Set(GPIOC->IDR, 12)) {
                fire_detect_count++;

                if (fire_detect_count >= 1000) {
                    Emergency_Flag = 1;

                    Pump_Set(0, 0);       
                    Env_LED_Set(0);       
                    RGB_LED_Set(1);       
                    Buzzer_Set(1000);     
                }
            } 
            else {
                fire_detect_count = 0; 
            }
        }
    }
}

void USART2_IRQHandler(void) 
{
    Uart_Data = (char)USART2->DR; 
    Uart_Data_In = 1;
    NVIC_ClearPendingIRQ(38); 
}

// 화재 센서(PC12) 및 유저 버튼(PC13) 초기화
void Fire_Interrupt_Init(void)
{
    Macro_Set_Bit(RCC->AHB1ENR, 2);  // GPIOC 클럭 ON
    Macro_Set_Bit(RCC->APB2ENR, 14); // SYSCFG 클럭 ON

    // PC12 화재 센서 설정 (Input, Pull-down) 
    Macro_Write_Block(GPIOC->MODER, 0x3, 0x0, 24); 
    Macro_Write_Block(GPIOC->PUPDR, 0x3, 0x2, 24); 

    // PC13 유저 버튼 설정 (비상 해제용 버튼은 EXTI 사용)
    Macro_Write_Block(GPIOC->MODER, 0x3, 0x0, 26);     // PC13 Input
    Macro_Write_Block(GPIOC->PUPDR, 0x3, 0x1, 26);     // Pull-up
    Macro_Write_Block(SYSCFG->EXTICR[3], 0xF, 0x2, 4); // PC13 -> EXTI13
    Macro_Set_Bit(EXTI->IMR, 13);  
    Macro_Set_Bit(EXTI->FTSR, 13); // Falling Edge

    // EXTI15_10_IRQn (IRQ 40) 활성화
    NVIC->ISER[1] = (1 << (40 - 32)); 
}

// EXTI15~10 인터럽트 (PC13 해제 버튼)
void EXTI15_10_IRQHandler(void)
{
    // 비상 해제 유저 버튼 (PC13)
    if (Macro_Check_Bit_Set(EXTI->PR, 13))
    {
        Macro_Set_Bit(EXTI->PR, 13);
        
        if (Emergency_Flag == 1) 
        {
            Emergency_Flag = 0; 
            fire_detect_count = 0;
            
            Buzzer_Set(0);      
            RGB_LED_Set(2);     
        }
    }
}
