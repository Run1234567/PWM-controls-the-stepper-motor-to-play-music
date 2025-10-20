/******************************************************************************
 * ��Ŀ���ƣ�STM32F4 PWM���ֲ�����
 * ��Ŀ���ã�����STM32F407��PWM���ֲ���ϵͳ��ͨ����ʱ��TIM3����PWM����������������������
 *           ֧�ֶ����������ţ�������ȷ�Ľ��Ŀ��ƺ�����Ƶ������
 * ���ߣ�RUN
 * ���ʱ�䣺2023��
 * Ӳ��ƽ̨��STM32F407ZGT6
 * �������ã�PC6 - TIM3_CH1 PWM���
 ******************************************************************************/

#include "stm32f4xx.h"
#include "usart1.h"
#include "delay.h"
#include "key.h"
#include "stdio.h"

// PWMռ�ձȱ���
volatile uint16_t pwm_duty_cycle = 50;   // ��ʼռ�ձ�50%

/**
  * @brief  GPIO��ʼ������
  * @param  ��
  * @retval ��
  * @�������ܣ�����PC6����ΪTIM3ͨ��1�ĸ��ù������
  * @����ʵ����GPIO1_Init();
  * @��ע��PC6��ӦTIM3_CH1������PWM�������
  */
void GPIO1_Init(void)
{
    GPIO_InitTypeDef GPIO_InitStructure;
    
    // ʹ��GPIOCʱ��
    RCC_AHB1PeriphClockCmd(RCC_AHB1Periph_GPIOC, ENABLE);
    
    // ����PC6ΪTIM3ͨ��1���
    GPIO_InitStructure.GPIO_Pin = GPIO_Pin_6;
    GPIO_InitStructure.GPIO_Mode = GPIO_Mode_AF;        // ���ù���ģʽ
    GPIO_InitStructure.GPIO_Speed = GPIO_Speed_100MHz;  // �������
    GPIO_InitStructure.GPIO_OType = GPIO_OType_PP;      // �������
    GPIO_InitStructure.GPIO_PuPd = GPIO_PuPd_UP;        // ��������
    GPIO_Init(GPIOC, &GPIO_InitStructure);
    
    // ����PC6��TIM3��ͨ��1
    GPIO_PinAFConfig(GPIOC, GPIO_PinSource6, GPIO_AF_TIM3);
}

/**
  * @brief  TIM3 PWM��ʼ������
  * @param  ��
  * @retval ��
  * @�������ܣ���ʼ��TIM3ΪPWM���ģʽ����ʼƵ��1kHz
  * @����ʵ����TIM3_PWM_Init();
  * @��ע��ʹ��TIM3ͨ��1����PWM���Σ���������������
  */
void TIM3_PWM_Init(void)
{
    TIM_TimeBaseInitTypeDef TIM_TimeBaseStructure;
    TIM_OCInitTypeDef TIM_OCInitStructure;
    
    // ʹ��TIM3ʱ��
    RCC_APB1PeriphClockCmd(RCC_APB1Periph_TIM3, ENABLE);
    
    // ��ʱ���������� - ��ʼƵ��1kHz
    TIM_TimeBaseStructure.TIM_Prescaler = 84 - 1;        // 84MHz/84 = 1MHz
    TIM_TimeBaseStructure.TIM_CounterMode = TIM_CounterMode_Up;  // ���ϼ���ģʽ
    TIM_TimeBaseStructure.TIM_Period = 1000 - 1;         // 1MHz/1000 = 1kHz
    TIM_TimeBaseStructure.TIM_ClockDivision = TIM_CKD_DIV1;      // ʱ�Ӳ���Ƶ
    TIM_TimeBaseStructure.TIM_RepetitionCounter = 0;     // �ظ�������
    TIM_TimeBaseInit(TIM3, &TIM_TimeBaseStructure);
    
    // PWMģʽ����
    TIM_OCInitStructure.TIM_OCMode = TIM_OCMode_PWM1;    // PWMģʽ1
    TIM_OCInitStructure.TIM_OutputState = TIM_OutputState_Enable;  // ʹ�����
    TIM_OCInitStructure.TIM_OutputNState = TIM_OutputNState_Disable; // ���û������
    TIM_OCInitStructure.TIM_Pulse = pwm_duty_cycle;      // ��ʼռ�ձ�
    TIM_OCInitStructure.TIM_OCPolarity = TIM_OCPolarity_High; // ������Ը�
    TIM_OCInitStructure.TIM_OCIdleState = TIM_OCIdleState_Reset; // ����״̬��
    TIM_OC1Init(TIM3, &TIM_OCInitStructure);
    
    // ʹ��TIM3Ԥװ�ؼĴ���
    TIM_OC1PreloadConfig(TIM3, TIM_OCPreload_Enable);
    TIM_ARRPreloadConfig(TIM3, ENABLE);
    
    // ʹ��TIM3
    TIM_Cmd(TIM3, ENABLE);
}

/**
  * @brief  ����PWMƵ�ʺ���
  * @param  freq: �����PWMƵ��(Hz)
  * @retval ��
  * @�������ܣ���̬����PWM���Ƶ�ʣ����ڲ�����ͬ���ߵ�����
  * @����ʵ����Set_PWM_Frequency(440); // ����Ƶ��Ϊ440Hz(A4��)
  * @��ע��Ƶ��Ϊ0ʱ�ر�PWM��������Ƶ���ܶ�ʱ��ʱ������
  */
void Set_PWM_Frequency(uint32_t freq)
{
    TIM_TimeBaseInitTypeDef TIM_TimeBaseStructure;
    uint32_t timer_clock = 84000000;  // TIM3ʱ��Ƶ��84MHz
    uint32_t prescaler, period;
    
    if(freq == 0 || freq > timer_clock)
    {
        // Ƶ��Ϊ0ʱ�ر�PWM���
        TIM_Cmd(TIM3, DISABLE);
        return;
    }
    
    // ֹͣ��ʱ��
    TIM_Cmd(TIM3, DISABLE);
    
    // ����Ԥ��Ƶ��������ֵ
    prescaler = 0;
    period = (timer_clock / freq) - 1;
    
    // �������ֵ����16λ���ֵ����Ҫ����Ԥ��Ƶ
    if(period > 0xFFFF)
    {
        prescaler = (period / 0xFFFF);
        period = (timer_clock / (freq * (prescaler + 1))) - 1;
    }
    
    // �������ö�ʱ��
    TIM_TimeBaseStructure.TIM_Prescaler = prescaler;
    TIM_TimeBaseStructure.TIM_CounterMode = TIM_CounterMode_Up;
    TIM_TimeBaseStructure.TIM_Period = period;
    TIM_TimeBaseStructure.TIM_ClockDivision = TIM_CKD_DIV1;
    TIM_TimeBaseStructure.TIM_RepetitionCounter = 0;
    TIM_TimeBaseInit(TIM3, &TIM_TimeBaseStructure);
    
    // ���ֵ�ǰռ�ձ�
    TIM_SetCompare1(TIM3, (period + 1) * pwm_duty_cycle / 100);
    
    // ����ʹ�ܶ�ʱ��
    TIM_Cmd(TIM3, ENABLE);
}

/**
  * @brief  ϵͳʱ�����ú���
  * @param  ��
  * @retval ��
  * @�������ܣ�����ϵͳʱ�Ӻ�����ʱ��
  * @����ʵ����RCC_Configuration();
  * @��ע��ϵͳʱ������Ϊ168MHz��ʹ����������ʱ��
  */
void RCC_Configuration(void)
{
    // ϵͳʱ������Ϊ168MHz
    SystemInit();
    
    // ʹ������ʱ��
    RCC_AHB1PeriphClockCmd(RCC_AHB1Periph_GPIOC, ENABLE);
    RCC_APB1PeriphClockCmd(RCC_APB1Periph_TIM3, ENABLE);
}

/**
  * @brief  TIM2��ʱ����ʼ������
  * @param  ��
  * @retval ��
  * @�������ܣ���ʼ��TIM2���ڶ�ʱ�жϣ��ṩ��ȷ��ʱ���׼
  * @����ʵ����TIM2_Init();
  * @��ע��TIM2����Ϊ10ms�жϣ���������ʱ������
  */
void TIM2_Init(void)
{
    TIM_TimeBaseInitTypeDef TIM_TimeBaseStructure;
    NVIC_InitTypeDef NVIC_InitStructure;
    
    // ʹ��TIM2ʱ��
    RCC_APB1PeriphClockCmd(RCC_APB1Periph_TIM2, ENABLE);
    
    // ��ʱ���������� - 10ms�ж�
    // 84MHz / 8400 = 10kHz, 10kHz / 100 = 100Hz (10ms)
    TIM_TimeBaseStructure.TIM_Prescaler = 8400 - 1;      // ��Ƶ��10kHz
    TIM_TimeBaseStructure.TIM_CounterMode = TIM_CounterMode_Up;
    TIM_TimeBaseStructure.TIM_Period = 100 - 1;          // 100������ = 10ms
    TIM_TimeBaseStructure.TIM_ClockDivision = TIM_CKD_DIV1;
    TIM_TimeBaseStructure.TIM_RepetitionCounter = 0;
    TIM_TimeBaseInit(TIM2, &TIM_TimeBaseStructure);
    
    // ʹ��TIM2�����ж�
    TIM_ITConfig(TIM2, TIM_IT_Update, ENABLE);
    
    // ����NVIC
    NVIC_InitStructure.NVIC_IRQChannel = TIM2_IRQn;
    NVIC_InitStructure.NVIC_IRQChannelPreemptionPriority = 1;
    NVIC_InitStructure.NVIC_IRQChannelSubPriority = 0;
    NVIC_InitStructure.NVIC_IRQChannelCmd = ENABLE;
    NVIC_Init(&NVIC_InitStructure);
    
    // ʹ��TIM2
    TIM_Cmd(TIM2, ENABLE);
}

// TIM2�жϼ�����
volatile uint32_t timer2_counter = 0;

/**
  * @brief  ���뼶��ʱ����
  * @param  ms: Ҫ��ʱ�ĺ�����
  * @retval ��
  * @�������ܣ�����TIM2��ʱ���ľ�ȷ������ʱ
  * @����ʵ����delay_ms(500); // ��ʱ500ms
  * @��ע������TIM2��10ms�жϣ����ȡ�10ms
  */
void delay_ms(uint32_t ms)
{
    uint32_t start_time = timer2_counter;
    
    // ������Ҫ�ȴ��Ķ�ʱ��������
    // ÿ10msһ�����ģ�����ms/10������Ҫ�Ľ�����
    uint32_t ticks_to_wait = (ms + 9) / 10;  // ����ȡ��
    
    // �ȴ��㹻��ʱ�����
    while ((timer2_counter - start_time) < ticks_to_wait)
    {
        // ��ѭ���ȴ�
    }
}

/**
  * @brief  �뼶��ʱ����
  * @param  seconds: Ҫ��ʱ������
  * @retval ��
  * @�������ܣ�����delay_ms���뼶��ʱ
  * @����ʵ����delay_s(2); // ��ʱ2��
  * @��ע������delay_msʵ�֣�������delay_ms��ͬ
  */
void delay_s(uint32_t seconds)
{
    delay_ms(seconds * 1000);
}

// ============================ ����Ƶ�ʶ��� ============================
// C3��B5����Ƶ�ʶ��� (��λ: Hz)

// ������ C3-B3
#define C3  130
#define C3s 138
#define D3  146
#define D3s 155
#define E3  164
#define F3  174
#define F3s 185
#define G3  196
#define G3s 207
#define A3  220
#define A3s 233
#define B3  247

// ������ C4-B4
#define C4  261
#define C4s 277
#define D4  293
#define D4s 311
#define E4  329
#define F4  349
#define F4s 370
#define G4  392
#define G4s 415
#define A4  440
#define A4s 466
#define B4  494

// ������ C5-B5
#define C5  523
#define C5s 554
#define D5  587
#define D5s 622
#define E5  659
#define F5  698
#define F5s 740
#define G5  784
#define G5s 831
#define A5  880
#define A5s 932
#define B5  988

// ============================ ���������궨�� ============================
// ��������ļ�����������

// ����������
#define One_Zhong   Set_PWM_Frequency(C4)  // ����1
#define Two_Zhong   Set_PWM_Frequency(D4)  // ����2
#define Three_Zhong Set_PWM_Frequency(E4)  // ����3
#define Four_Zhong  Set_PWM_Frequency(F4)  // ����4
#define Five_Zhong  Set_PWM_Frequency(G4)  // ����5
#define Six_Zhong   Set_PWM_Frequency(A4)  // ����6
#define Seven_Zhong Set_PWM_Frequency(B4)  // ����7

// ����������
#define One_High    Set_PWM_Frequency(C5)  // ����1
#define Two_High    Set_PWM_Frequency(D5)  // ����2
#define Three_High  Set_PWM_Frequency(E5)  // ����3
#define Four_High   Set_PWM_Frequency(F5)  // ����4
#define Five_High   Set_PWM_Frequency(G5)  // ����5
#define Six_High    Set_PWM_Frequency(A5)  // ����6
#define Seven_High  Set_PWM_Frequency(B5)  // ����7

// ����������
#define One_Low     Set_PWM_Frequency(C3)  // ����1
#define Two_Low     Set_PWM_Frequency(D3)  // ����2
#define Three_Low   Set_PWM_Frequency(E3)  // ����3
#define Four_Low    Set_PWM_Frequency(F3)  // ����4
#define Five_Low    Set_PWM_Frequency(G3)  // ����5
#define Six_Low     Set_PWM_Frequency(A3)  // ����6
#define Seven_Low   Set_PWM_Frequency(B3)  // ����7

// ��ֹ�����ر�������
#define REST Set_PWM_Frequency(0)

// ============================ ����ʱ�䶨�� ============================
// ʹ�þ�׼��ʱ���¶������ʱ�䣨��λ�����룩
#define WHOLE_NOTE      500   // ȫ������500ms
#define QUARTER_NOTE    WHOLE_NOTE/4   // �ķ�������125ms
#define HALF_NOTE       WHOLE_NOTE/2   // ����������250ms
#define EIGHTH_NOTE     WHOLE_NOTE/8   // �˷�������62.5ms

/**
  * @brief  ������������
  * @param  arr: �������ݶ�ά����
  * @param  rows: ������������
  * @retval ��
  * @�������ܣ�����������������ݲ�������������
  * @����ʵ����BoFangGeQu(xiaoXingXing, sizeof(xiaoXingXing)/sizeof(xiaoXingXing[0]));
  * @��ע���������ݸ�ʽΪ[����, ����, ʱֵ]����ֹ��Ϊ[0,0,ʱֵ]
  */
void BoFangGeQu(int arr[][3], int rows) 
{
    uint32_t duration;
    // ��������Ƶ�ʱ� [����][����] -> Ƶ��
    uint16_t note_freq[3][7] = {
        // ������ (0)
        {C3, D3, E3, F3, G3, A3, B3},
        // ������ (1)  
        {C4, D4, E4, F4, G4, A4, B4},
        // ������ (2)
        {C5, D5, E5, F5, G5, A5, B5}
    };
    
    // ����ʱֵӳ��� [ʱֵ] -> ����ʱ��(ms)
    // ʱֵ1: ȫ������ʱֵ2: ����������ʱֵ4: �ķ�������ʱֵ8: �˷�����
    uint32_t duration_map[] = {
        0,      // 0 - ��Ч
        WHOLE_NOTE,     // 1 - ȫ����
        HALF_NOTE,      // 2 - ��������  
        HALF_NOTE + QUARTER_NOTE, // 3 - ��������+�ķ��������������������
        QUARTER_NOTE,   // 4 - �ķ�����
        0,              // 5 - ��Ч
        0,              // 6 - ��Ч
        0,              // 7 - ��Ч
        EIGHTH_NOTE     // 8 - �˷�����
    };
    int i;
    
    // ������ά�����е�ÿ������
    for ( i = 0; i < rows; i++) {
        int yinQu = arr[i][0];     // ������0-������1-������2-����
        int yinDiao = arr[i][1];   // ������1-7��Ӧdo,re,mi,fa,sol,la,si
        int shiZhi = arr[i][2];    // ʱֵ��1-ȫ������2-����������4-�ķ�������8-�˷�����
        
        // �������ʱ��
        if (shiZhi == 3) {
            duration = HALF_NOTE + QUARTER_NOTE;  // �����������
        } else {
            duration = duration_map[shiZhi];
        }
        
        // ����Ƿ�Ϊ��ֹ�� (����0��ʾ��ֹ��)
        if (yinQu == 0 && yinDiao == 0) {
            // ��ֹ�� - ֱ����ʱ
            REST;
            delay_ms(duration);
            continue; // ������������
        }
        
        // ��֤��������ĺϷ���
        if (yinQu < 0 || yinQu > 2) {
            // ��Ч������������ֹ��
            REST;
            delay_ms(duration);
        }
        else if (yinDiao < 1 || yinDiao > 7) {
            // ��Ч������������ֹ��
            REST;
            delay_ms(duration);
        }
        else if (shiZhi != 1 && shiZhi != 2 && shiZhi != 4 && shiZhi != 8) {
            // ��Чʱֵ��ʹ��Ĭ���ķ�����
            Set_PWM_Frequency(note_freq[yinQu][yinDiao-1]);
            delay_ms(QUARTER_NOTE);
        }
        else {
            // ������Ч����
            Set_PWM_Frequency(note_freq[yinQu][yinDiao-1]);
            delay_ms(duration);
            
            // ��ѡ��������֮����Ӷ��ݵļ����ʹ���ָ�����
            if (i < rows - 1) { // �������һ������
                REST;
                delay_ms(20); // 20ms�ļ��
            }
        }
    }
    
    // ���Ž�����ر�����
    REST;
}

// ============================ �������ݶ��� ============================

/**
  * ��С���ǡ���������
  * ��ʽ��[����, ����, ʱֵ]
  * ����: 1-����, 2-����
  * ����: 1-7����do,re,mi,fa,sol,la,si
  * ʱֵ: 4-�ķ�����, 2-��������
  */
int xiaoXingXing[][3] = {
    {1, 1, 4},  // ����1���ķ�����
    {1, 1, 4},  // ����1���ķ�����
    {2, 5, 4},  // ����5���ķ�����
    {2, 5, 4},  // ����5���ķ�����
    {2, 6, 4},  // ����6���ķ�����
    {2, 6, 4},  // ����6���ķ�����
    {2, 5, 2},  // ����5����������
    
    {2, 4, 4},  // ����4���ķ�����
    {2, 4, 4},  // ����4���ķ�����
    {2, 3, 4},  // ����3���ķ�����
    {2, 3, 4},  // ����3���ķ�����
    {2, 2, 4},  // ����2���ķ�����
    {2, 2, 4},  // ����2���ķ�����
    {1, 1, 2},  // ����1����������
};

/**
  * ������̽���ϡ���������������
  */
		// ������������
// ÿ������ֵ��[����, ����, ʱֵ]
// ����: 0-����, 1-����, 2-����
// ����: 1-7����do, re, mi, fa, sol, la, si
// ʱֵ: ��ʾ�������������ʱ���ǻ�׼���ĵļ���֮һ
int KeNan[][3]={
               {1700,0,0},
							 {1,1,2},
							 {0,7,2},
							 {0,6,1},
							 {1,3,1},
							 {0,0,1},
							 {1,1,2},
							 {0,6,2},
							 {0,7,1},
							 {1,4,1},
							 {1,3,2},
							 {1,2,1},
							 {1,1,2},
							 {1,2,2},
							 {1,1,2},
							 {1,2,2},
							 {1,3,1},
							 {1,1,2},
							 {0,7,2},
							 {0,6,1},
							 {1,2,1},
							 {1,1,2},
							 {0,7,2},
							 {0,0,2},
							 {0,6,4},
							 {0,7,4},
							 {1,1,1},
							 {0,6,1},
							 {1,3,1},
							 {1,1,1},
							 {1,2,1},
							 {1,6,1},
							 {1,5,1},
							 {1,4,2},
							 {1,3,4},
							 {1,2,4},
							 {1,3,1},
							 {0,0,1},
							 {0,0,1},
							 {1,1,2},
							 {0,7,2},
							 {0,6,1},
							 {1,3,1},
							 {0,0,1},
							 {1,1,2},
							 {0,6,2},
							 {0,7,1},
							 {1,4,1},
							 {1,3,2},
							 {1,2,1},
							 {1,1,2},
							 {1,2,2},
							 {1,1,2},
							 {1,2,2},
							 {1,3,1},
							 {1,1,2},
							 {0,7,2},
							 {0,6,1},
							 {1,2,1},
							 {1,1,2},
							 {0,7,2},
							 {0,0,2},
							 {0,6,4},
							 {0,7,4},
							 {1,1,1},
							 {0,6,1},
							 {1,3,1},
							 {1,1,1},
							 {1,2,1},
							 {1,6,1},
							 {1,5,1},
							 {1,4,2},
							 {1,3,4},
							 {1,2,4},
							 {1,3,1},
							 {0,0,1},
							 {0,0,1},
							 {1,3,2},
							 {1,3,2},
							 {2,1,1},
							 {1,6,1},
							 {1,5,2},
							 {1,6,2},
							 {2,1,2},
							 {1,7,2},
							 {1,6,1},
							 {1,5,2},
							 {1,5,1},
							 {1,6,1},
							 {1,1,2},
							 {1,2,1},
							 {1,1,2},
							 {1,2,2},
							 {1,3,2},
							 {0,0,1},
							 {0,0,1},
							 {1,1,2},
							 {1,2,2},
							 {1,3,2},
							 {1,4,1},
							 {1,3,2},
							 {1,2,2},
							 {0,0,1},
							 {1,6,1},
							 {1,5,2},
							 {1,4,2},
							 {1,5,2},
							 {1,6,1},
							 {1,7,2},
							 {1,7,1},
							 {0,0,1},
							 {0,0,1},
							 {1,1,2},
							 {0,7,2},
							 {0,6,1},
							 {1,3,1},
							 {0,0,1},
							 {1,1,2},
							 {0,6,2},
							 {0,7,1},
							 {1,4,1},
							 {1,3,2},
							 {1,2,1},
							 {1,1,2},
							 {1,2,2},
							 {1,1,2},
							 {1,2,2},
							 {1,3,2},
							 {1,3,2},
							 {1,4,2},
							 {1,5,2},
							 {1,6,1},
							 {2,1,1},
							 {1,7,1},
							 {1,5,1},
							 {1,6,1},
							 {0,0,1},
							 {0,0,2},
							 {1,5,4},
							 {1,5,4},
							 {1,6,2},
							 {1,3,2},
							 {1,4,2},
							 {1,2,2},
							 {1,3,2},
							 {0,7,2},
							 {1,1,2},
							 {0,6,2}
              };
/**
  * ��JIALEBI����������
  */
int JIALEBI[][3]={
   {1,3,2},
	 {1,5,2},
	 {1,6,1},
	 {1,6,1},
	 {1,6,2},
	 {1,7,2},
	 {2,1,1},
	 {2,1,1},
	 {2,1,2},
	 {2,2,2},
	 {1,7,1},
	 {1,7,1},
	 {1,6,2},
	 {1,5,2},
	 {1,5,2},
	 {1,6,2},
	 {0,0,1},
	 
	 {1,3,2},
	 {1,5,2},
	 {1,6,1},
	 {1,6,1},
	 {1,6,2},
	 {1,7,2},
	 {2,1,1},
	 {2,1,1},
	 {2,1,2},
	 {2,2,2},
	 {1,7,1},
	 {1,7,1},
	 {1,6,2},
	 {1,5,2},
	 {1,6,2},
	 {0,0,1},
	 
	 {1,3,2},
	 {1,5,2},
	 {1,6,1},
	 {1,6,1},
	 {1,6,2},
	 {2,1,2},
	 {2,2,1},
	 {2,2,1},
	 {2,2,2},
	 {2,2,3},
	 {2,4,1},
	 {2,4,1},
	 {2,3,2},
	 {2,2,2},
	 {2,3,2},
	 {1,6,2},
	 {0,0,1},
	 
	 {1,6,2},
	 {1,7,2},
	 {2,1,1},
	 {2,1,1},
	 {2,2,1},
	 {2,3,2},
	 {1,6,2},
	 {0,0,1},
	 
	 {2,6,2},
	 {2,1,2},
   {1,7,1},
   {1,7,1},
   {2,1,2},
{1,6,2},
{1,7,1},
{0,0,1},


	 
	 
};

/**
  * @brief  ������
  * @param  ��
  * @retval int ����ִ��״̬
  * @�������ܣ�������ڣ���ʼ��Ӳ������������
  * @����ʵ������
  * @��ע������ִ����Ϻ��������ѭ��
  */
int main(void)
{
    // ϵͳ��ʼ��
    RCC_Configuration();
    
    // GPIO��ʼ��
    GPIO1_Init();
    
    // ������ʼ��
    key_init();
    
    // PWM��ʼ��
    TIM3_PWM_Init();
    TIM2_Init();
    Serial_Init();
    
    // ���ó�ʼռ�ձ�
    TIM_SetCompare1(TIM3, 50);
    
    // ����JIALEBI����
    BoFangGeQu(JIALEBI, 200);
    
    // �ر�PWM
    Set_PWM_Frequency(0);
    
    // ��ѭ��
    while(1)
    {
        // ��ѭ�� - ������ɺ󱣳־���״̬
    }
}

/**
  * @brief  TIM2�жϷ�����
  * @param  ��
  * @retval ��
  * @�������ܣ�TIM2��ʱ���жϴ���ÿ10msִ��һ��
  * @����ʵ�����Զ����ã������ֶ�����
  * @��ע������ά��ʱ���������֧�־�ȷ��ʱ����
  */
void TIM2_IRQHandler(void)
{
    if (TIM_GetITStatus(TIM2, TIM_IT_Update) != RESET)
    {
        TIM_ClearITPendingBit(TIM2, TIM_IT_Update);
        timer2_counter++;  // ���������������ھ�ȷ��ʱ
    }
}
