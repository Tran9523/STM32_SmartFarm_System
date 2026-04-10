#include "device_driver.h"
#include <stdio.h>

#define LIGHT_SAMPLE_COUNT 50  // 100ms마다 샘플링 시 5초간의 데이터

// [본체 정의] 여기서 메모리가 할당됩니다.
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
    // printf 버퍼링 끄기 (TeraTerm 즉시 출력)
    setvbuf(stdout, NULL, _IONBF, 0); 
    
    TIM4_Repeat_Interrupt_Enable(1, 1); // 1ms 시스템 틱
    
    printf("\r\n====================================\r\n");
    printf("     Smart Farm System Started!     \r\n");
    printf("====================================\r\n");

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
    int was_active = 0; // 이전 루프의 동작 상태 기억

    Servo_Door_Set(0); 

    while (1)
    {
        // 매 루프마다 동작 플래그 초기화
        int is_active = 0;

        // --- 센서 데이터 읽기 ---
        int rfid_signal = Read_Arduino_Signal();
        int water_level = ADC1_Read_Channel(6);
        int light_level = ADC1_Read_Channel(7);

        // ----------------------------------------------------
        // 실시간 모니터링 출력
        // ----------------------------------------------------
        if (System_Tick - last_debug_print_time >= 1000) {
            printf("\r\n------------- [Sensor Status] -------------\r\n");
            printf(" 💧 수위 센서 (Water) : %d\r\n", water_level);
            printf(" ☀️ 조도 평균 (Light) : %d\r\n", samples_filled > 0 ? light_avg : light_level);
            printf(" 🔥 화재 상태 (Fire)  : %s\r\n", Emergency_Flag ? "[비상!] 화재 감지됨" : "안전 (정상)");
            printf(" 🚪 출입문 현재 각도  : %d 도\r\n", current_door_angle);
            printf(" 📡 RFID 수신 신호    : %d\r\n", rfid_signal);
            printf("-------------------------------------------\r\n");
            last_debug_print_time = System_Tick;
        }

        // ----------------------------------------------------
        // [1번 기능] 출입 통제 시스템 (RFID)
        // ----------------------------------------------------
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
                is_active = 1; // [추가] 문이 열려있는 10초 동안 동작 상태 마킹
            } else {
                target_door_angle = 0;
            }
        }

        // 문이 부드럽게 이동 중일 때도 동작 상태 마킹
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

        // 🚨 비상시 아래 로직(급수, 블라인드) 무시
        if (Emergency_Flag) continue;

        // ----------------------------------------------------
        // [3번 기능] 수위 경보 및 [2번 기능] 자동 급수 시스템
        // ----------------------------------------------------
        
        // 💡 [히스테리시스 로직]
        // 1500 미만으로 떨어지면 물 부족 경보 시작
        if (water_level < 1500) {
            is_water_shortage = 1; 
        } 
        // 역류(Backflow)로 인해 1500~1600이 되더라도 무시함.
        // 완전히 물을 보충하여 1800을 넘겨야만 정상 상태로 복귀!
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
            is_watering = 0; // 하던 급수도 즉시 강제 종료
        } else {
            Buzzer_Set(0); // 경고음 해제

            if (!is_watering && (System_Tick - last_watering_time >= 30000)) {  // 급수 주기 변경
                is_watering = 1;
                water_start_time = System_Tick;
            }

            if (is_watering) {
                is_active = 1; // 급수 펌프 동작 중 마킹
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

        // ----------------------------------------------------
        // [조도 센서 필터링 및 환경 LED/블라인드 제어
        // ----------------------------------------------------
        if (System_Tick - last_light_sample_time >= 100) {
            light_sum -= light_samples[light_sample_idx];
            light_samples[light_sample_idx] = light_level;
            light_sum += light_level;
            
            light_sample_idx = (light_sample_idx + 1) % LIGHT_SAMPLE_COUNT;
            if (samples_filled < LIGHT_SAMPLE_COUNT) samples_filled++;
            
            light_avg = light_sum / samples_filled;
            last_light_sample_time = System_Tick;
        }

        // 💡 조도량에 따른 3단계 점진적 LED 제어
        if (light_avg < 500) {
            Env_LED_Set(3); // 매우 어두움: LED 3개 모두 켬
        } else if (light_avg < 800) {
            Env_LED_Set(2); // 조금 어두움: LED 2개 켬
        } else if (light_avg < 1200) {
            Env_LED_Set(1); // 약간 어두움: LED 1개 켬
        } else {
            Env_LED_Set(0); // 충분히 밝음: LED 모두 끔
        }

        // 🚪 블라인드 타겟 결정 (아주 밝아지면 닫기)
        // LED가 모두 꺼진(0) 상태에서 빛이 더 강해지면 블라인드가 작동
        if (light_avg > 1700) target_blind_state = 1;      // 너무 밝음 -> 닫기
        else if (light_avg < 1200) target_blind_state = 0; // 어두워짐 -> 열기

        // 블라인드 이동 로직
        if (current_blind_state != target_blind_state) {
            is_active = 1; // 블라인드 이동 중 마킹
            
            // 이동하는 동안 즉시 파란불이 켜지도록 강제 세팅
            RGB_LED_Set(3); 
            
            if (target_blind_state == 1) {
                printf("\r\n[BLIND] 자연스럽게 닫히는 중...\r\n");
                Step_Move_Angle(120); 
            } else {
                printf("\r\n[BLIND] 자연스럽게 열리는 중...\r\n");
                Step_Move_Angle(-120); 
            }
            current_blind_state = target_blind_state;
        }

        // ----------------------------------------------------
        // [5번 기능] 시스템 상태 표시 (RGB LED 점멸 로직)
        // ----------------------------------------------------
        if (is_active) {
            if (!was_active) {
                // 방금 동작을 시작했으면 즉시 파란불 켜기
                blue_led_state = 1;
                last_blue_led_toggle = System_Tick;
                RGB_LED_Set(3); // 3: Blue
            } else {
                // 계속 동작 중이면 500ms마다 깜빡이기 (Toggle)
                if (System_Tick - last_blue_led_toggle >= 500) {
                    blue_led_state = !blue_led_state;
                    RGB_LED_Set(blue_led_state ? 3 : 0); // 3: Blue, 0: OFF
                    last_blue_led_toggle = System_Tick;
                }
            }
            was_active = 1;
        } else {
            // 아무것도 동작하지 않는 대기 상태
            RGB_LED_Set(2); // 2: Green
            was_active = 0;
            blue_led_state = 0;
        }
    }
}