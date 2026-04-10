#include "device_driver.h"
#include <stdio.h>

void Step_Init(void)
{
    Macro_Set_Bit(RCC->AHB1ENR, 1);
    Macro_Write_Block(GPIOB->MODER, 0x3, 0x1, 16); // PB8
    Macro_Write_Block(GPIOB->MODER, 0x3, 0x1, 18); // PB9
    Macro_Write_Block(GPIOB->MODER, 0x3, 0x1, 20); // PB10
    Macro_Write_Block(GPIOB->MODER, 0x3, 0x1, 28); // PB14

    Macro_Clear_Bit(GPIOB->OTYPER, 8);
    Macro_Clear_Bit(GPIOB->OTYPER, 9);
    Macro_Clear_Bit(GPIOB->OTYPER, 10);
    Macro_Clear_Bit(GPIOB->OTYPER, 14);

    // Read-Modify-Write 방식으로 안전하게 초기화
    int mask = (1<<8)|(1<<9)|(1<<10)|(1<<14);
    GPIOB->ODR &= ~mask;
}

static void Step_Drive(int seq_idx)
{
    const int pattern[8][4] = {
        {1,0,0,0},{1,1,0,0},{0,1,0,0},{0,1,1,0},
        {0,0,1,0},{0,0,1,1},{0,0,0,1},{1,0,0,1}
    };

    int out = 0;
    if(pattern[seq_idx][0]) out |= (1<<8);
    if(pattern[seq_idx][1]) out |= (1<<9);
    if(pattern[seq_idx][2]) out |= (1<<10);
    if(pattern[seq_idx][3]) out |= (1<<14);

    int mask = (1<<8)|(1<<9)|(1<<10)|(1<<14);
    GPIOB->ODR = (GPIOB->ODR & ~mask) | out;

    for(volatile int i=0; i<80000; i++); // 속도 조절 딜레이
}

void Step_Move(int steps, int dir)
{
    int seq = 0;

    for(int i=0;i<steps;i++)
    {
        Step_Drive(seq);
        seq += dir;

        if(seq > 7) seq = 0;
        if(seq < 0) seq = 7;
    }

    GPIOB->ODR &= ~((1<<8)|(1<<9)|(1<<10)|(1<<14));
}

void Step_Move_Angle(int angle)
{
    int steps = (2048 * angle) / 360;

    if(steps > 0)
        Step_Move(steps, 1);
    else if(steps < 0)
        Step_Move(-steps, -1);
}