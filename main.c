#include "device_driver.h"
#include <stdio.h>

#define LIGHT_SAMPLE_COUNT 50  // 5초 (100ms) 데이터

volatile unsigned int System_Tick = 0; 
volatile int System_Mode = 0;
volatile char Uart_Data = 0;
volatile int Uart_Data_In = 0;
volatile unsigned int rfid_open_until = 0;
extern volatile unsigned int fire_detect_count;

int Main(void)
{
    // ==========================================
    // 1. 시스템 및 통신 초기화
    // ==========================================
    Clock_Init();
    Uart2_Init(115200);
    setvbuf(stdout, NULL, _IONBF, 0); 
    
    TIM4_Repeat_Interrupt_Enable(1); // 1ms 시스템 틱

    // ==========================================
    // 2. 하드웨어 초기화
    // ==========================================
    Arduino_Comm_Init(); 
    Indicator_Init();    
    Servo_Init();        
    Pump_Init();         
    Step_Init();         
    ADC1_Init();         
    Fire_Interrupt_Init(); 

    // ==========================================
    // 3. 상태 관리 변수 
    // ==========================================
    // RFID 문 제어
    int current_door_angle = 0;
    int target_door_angle = 0;
    unsigned int last_door_move_time = 0;
    
    int prev_rfid_signal = 0;

    // 급수 제어
    unsigned int last_watering_time = 0;
    int is_watering = 0;
    unsigned int water_start_time = 0;

    int is_water_shortage = 0;
    
    unsigned int last_hose_sweep = 0;
    int hose_angle = 25;
    int hose_dir = 1;

    unsigned int last_buzzer_toggle = 0;
    int buzzer_state = 0;
    
    // 일조량 제어
    int light_samples[LIGHT_SAMPLE_COUNT] = {0};
    int light_sample_idx = 0;
    long light_sum = 0;
    int light_avg = 0;
    int samples_filled = 0;
    unsigned int last_light_sample_time = 0;

    int target_blind_state = 0; 
    int current_blind_state = 0;

    unsigned int last_debug_print_time = 0;

    // 파란색 LED 점멸 관리
    unsigned int last_blue_led_toggle = 0;
    int blue_led_state = 0;
    int was_active = 0;

    Servo_Door_Set(0); 

    // ==========================================
    // 4. 상태 제어 
    // ==========================================
    while (1)
    {
        int is_active = 0;

        // --- 센서 데이터 읽기 ---
        int rfid_signal = Read_Arduino_Signal();
        int water_level = ADC1_Read_Channel(6);
        int light_level = ADC1_Read_Channel(7);

        // 실시간 모니터링 출력
        if (System_Tick - last_debug_print_time >= 1000) {
            printf("\r\n------------- [Sensor Status] -------------\r\n");
            printf(" 수위 센서 (Water) : %d\r\n", water_level);
            printf(" 조도 평균 (Light) : %d\r\n", samples_filled > 0 ? light_avg : light_level);
            printf(" 화재 상태 (Fire)  : %s\r\n", Emergency_Flag ? "[비상!] 화재 감지됨" : "안전 (정상)");
            printf(" RFID 수신 신호    : %d\r\n", rfid_signal);
            printf("-------------------------------------------\r\n");
            last_debug_print_time = System_Tick;
        }

        // 출입 통제 시스템 (RFID)
        if ((rfid_signal == 1) && (prev_rfid_signal == 0)) {
            rfid_open_until = System_Tick + 10000; 
            printf("\r\n[EVENT] RFID Approved! Door opening for 10s.\r\n");
        }
        prev_rfid_signal = rfid_signal;

        if (Emergency_Flag) {
            target_door_angle = 90; 
        } else {
            if (System_Tick < rfid_open_until) {
                target_door_angle = 90;
                is_active = 1;
            } else {
                target_door_angle = 0;
            }
        }

        if (current_door_angle != target_door_angle) {
            is_active = 1; 
            
            if (System_Tick - last_door_move_time >= 10) {
                if (current_door_angle < target_door_angle) {
                    current_door_angle += 10;
                    if (current_door_angle > target_door_angle) current_door_angle = target_door_angle;
                }
                else if (current_door_angle > target_door_angle) {
                    current_door_angle -= 10;
                    if (current_door_angle < target_door_angle) current_door_angle = target_door_angle;
                }

                if (current_door_angle > 90) current_door_angle = 90;
                if (current_door_angle < 0)  current_door_angle = 0;

                Servo_Door_Set(current_door_angle);
                last_door_move_time = System_Tick;
            }
        }

        // 비상시 아래 로직 무시
        if (Emergency_Flag) continue;

        // 수위 센서 감지 및 자동 급수 시스템
        if (water_level < 1500) {
            is_water_shortage = 1; 
        } 
        else if (water_level > 1800) {
            is_water_shortage = 0; 
        }

        // 상태 변수(is_water_shortage)를 기준으로 동작 판별
        if (is_water_shortage == 1) {
            if (System_Tick - last_buzzer_toggle >= 500) {
                buzzer_state = !buzzer_state;
                Buzzer_Set(buzzer_state ? 2000 : 0);
                last_buzzer_toggle = System_Tick;
            }
            Pump_Set(0, 0); 
            is_watering = 0;
        } else {
            Buzzer_Set(0);

            if (!is_watering && (System_Tick - last_watering_time >= 30000)) {  // 급수 주기 변경
                is_watering = 1;
                water_start_time = System_Tick;
            }

            if (is_watering) {
                is_active = 1;
                Pump_Set(1, 80);    // DC 파워 %

                if (System_Tick - last_hose_sweep >= 90) {  // 호스 스윙
                    hose_angle += (hose_dir * 5);
                    if (hose_angle >= 50 || hose_angle <= 0) hose_dir *= -1;
                    Servo_Hose_Set(hose_angle);
                    last_hose_sweep = System_Tick;
                }

                if (System_Tick - water_start_time >= 1000) {   // 급수량(시간) 
                    is_watering = 0;
                    Pump_Set(0, 0);
                    last_watering_time = System_Tick;
                }
            }
        }

        // 조도 센서 필터링 및 환경 LED/블라인드 제어
        if (System_Tick - last_light_sample_time >= 100) {
            light_sum -= light_samples[light_sample_idx];
            light_samples[light_sample_idx] = light_level;
            light_sum += light_level;
            
            light_sample_idx = (light_sample_idx + 1) % LIGHT_SAMPLE_COUNT;
            if (samples_filled < LIGHT_SAMPLE_COUNT) samples_filled++;
            
            light_avg = light_sum / samples_filled;
            last_light_sample_time = System_Tick;
        }

        // 조도량에 따른 LED 제어
        if (light_avg < 500) {
            Env_LED_Set(3);
        } else if (light_avg < 800) {
            Env_LED_Set(2);
        } else if (light_avg < 1200) {
            Env_LED_Set(1);
        } else {
            Env_LED_Set(0);
        }

        // 블라인드 제어
        if (light_avg > 1700) target_blind_state = 1;      // 닫기
        else if (light_avg < 1200) target_blind_state = 0; // 열기

        if (current_blind_state != target_blind_state) {
            is_active = 1;
            RGB_LED_Set(3); 
            
            if (target_blind_state == 1) {
                Step_Move_Angle(120); 
            } else {
                Step_Move_Angle(-120); 
            }
            current_blind_state = target_blind_state;
        }

        // 시스템 상태 표시 (RGB LED 점멸 로직)
        if (is_active) {
            if (!was_active) {
                blue_led_state = 1;
                last_blue_led_toggle = System_Tick;
                RGB_LED_Set(3);
            } else {
                if (System_Tick - last_blue_led_toggle >= 500) {
                    blue_led_state = !blue_led_state;
                    RGB_LED_Set(blue_led_state ? 3 : 0); // toogle
                    last_blue_led_toggle = System_Tick;
                }
            }
            was_active = 1;
        } else {
            // 대기 상태
            RGB_LED_Set(2);
            was_active = 0;
            blue_led_state = 0;
        }
    }
}
