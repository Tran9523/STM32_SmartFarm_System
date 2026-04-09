#include "device_driver.h"

// 아두이노 신호 수신 초기화 (PA4)
void Arduino_Comm_Init(void)
{
    // GPIOA 클럭 ON
    Macro_Set_Bit(RCC->AHB1ENR, 0); 
    
    // PA4 Input 모드(00) 및 Pull-down(10) 설정
    Macro_Write_Block(GPIOA->MODER, 0x3, 0x0, 8); 
    Macro_Write_Block(GPIOA->PUPDR, 0x3, 0x2, 8); 
}

// 아두이노 신호(1 또는 0) 읽기
int Read_Arduino_Signal(void)
{
    // PA4 핀의 상태를 읽어서 1(High)인지 0(Low)인지 반환
    return Macro_Check_Bit_Set(GPIOA->IDR, 4);
}