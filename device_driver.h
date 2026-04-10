#ifndef DEVICE_DRIVER_H
#define DEVICE_DRIVER_H

#include "stm32f4xx.h"
#include "option.h"
#include "macro.h"

// ==========================================
// 전역 변수 공유 (extern 선언)
// ==========================================
extern volatile unsigned int System_Tick;
extern volatile int Emergency_Flag;
extern volatile char Uart_Data;
extern volatile int Uart_Data_In;
extern volatile int System_Mode;

// System
extern void Clock_Init(void);
extern void TIM4_Repeat_Interrupt_Enable(int en, int time);

// UART
extern void Uart2_Init(int baud);
extern void Uart2_Send_Byte(char data);
extern void Uart2_RX_Interrupt_Enable(int en);

// ADC (수위: PA6 / 조도: PA7)
extern void ADC1_Init(void);
extern int ADC1_Read_Channel(int ch);

// arduino
extern void Arduino_Comm_Init(void);
extern int Read_Arduino_Signal(void);

// Indicator (환경 LED, 부저)
extern void Indicator_Init(void);
extern void Env_LED_Set(int cnt);
extern void RGB_LED_Set(int color);
extern void Buzzer_Set(int freq);

// Step Motor
extern void Step_Init(void);
extern void Step_Move(int steps, int dir);
extern void Step_Move_Angle(int angle);

// Pump (급수 제어 DC 모터)
extern void Pump_Init(void); // PB1, PB2
extern void Pump_Set(int enable, int duty);

// Servo (출입문 PA0, 호스 PA1)
extern void Servo_Init(void);
extern void Servo_Door_Set(int angle);
extern void Servo_Hose_Set(int angle);
extern void Servo_Door_Enable(void);
extern void Servo_Door_Disable(void);

// Exception
extern void _Invalid_ISR(void);
extern void TIM4_IRQHandler(void);
extern void USART2_IRQHandler(void);
extern void EXTI15_10_IRQHandler(void);
extern void Fire_Interrupt_Init(void);

#endif