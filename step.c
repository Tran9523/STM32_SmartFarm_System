#include "device_driver.h"
#include <stdio.h>

// Non-Blocking 적용을 위한 변수 선언
volatile int step_target_count = 0;
volatile int step_current_count = 0;
volatile int step_dir_current = 0;
volatile int step_seq_idx = 0;
unsigned int last_step_tick = 0;

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
}

void Step_Move_Angle(int angle)
{
    int steps = (2048 * angle) / 360;
    
    if(steps > 0) {
        step_target_count = steps;
        step_dir_current = 1;
    } else if(steps < 0) {
        step_target_count = -steps;
        step_dir_current = -1;
    }
    step_current_count = 0;
}

void Step_Task(void)
{
    extern volatile unsigned int System_Tick;

    if (step_current_count < step_target_count) 
    {
        if (System_Tick - last_step_tick >= 2) 
        {
            Step_Drive(step_seq_idx);
            
            step_seq_idx += step_dir_current;
            if(step_seq_idx > 7) step_seq_idx = 0;
            if(step_seq_idx < 0) step_seq_idx = 7;

            step_current_count++;
            last_step_tick = System_Tick;
        }
    } 
    else if (step_target_count > 0 && step_current_count == step_target_count) 
    {
        GPIOB->ODR &= ~((1<<8)|(1<<9)|(1<<10)|(1<<14));
        step_target_count = 0;
    }
}