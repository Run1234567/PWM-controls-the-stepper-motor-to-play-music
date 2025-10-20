/******************************************************************************
 * 项目名称：STM32F4 PWM音乐播放器
 * 项目作用：基于STM32F407的PWM音乐播放系统，通过定时器TIM3产生PWM波形驱动蜂鸣器播放音乐
 *           支持多首乐曲播放，包含精确的节拍控制和音符频率生成
 * 作者：RUN
 * 完成时间：2025年10月20号
 * 硬件平台：STM32F407ZGT6
 * 引脚配置：PC6 - TIM3_CH1 PWM输出
 ******************************************************************************/

#include "stm32f4xx.h"
#include "usart1.h"
#include "delay.h"
#include "key.h"
#include "stdio.h"

// PWM占空比变量
volatile uint16_t pwm_duty_cycle = 50;   // 初始占空比50%

/**
  * @brief  GPIO初始化函数
  * @param  无
  * @retval 无
  * @函数功能：配置PC6引脚为TIM3通道1的复用功能输出
  * @函数实例：GPIO1_Init();
  * @备注：PC6对应TIM3_CH1，用于PWM波形输出
  */
void GPIO1_Init(void)
{
    GPIO_InitTypeDef GPIO_InitStructure;
    
    // 使能GPIOC时钟
    RCC_AHB1PeriphClockCmd(RCC_AHB1Periph_GPIOC, ENABLE);
    
    // 配置PC6为TIM3通道1输出
    GPIO_InitStructure.GPIO_Pin = GPIO_Pin_6;
    GPIO_InitStructure.GPIO_Mode = GPIO_Mode_AF;        // 复用功能模式
    GPIO_InitStructure.GPIO_Speed = GPIO_Speed_100MHz;  // 高速输出
    GPIO_InitStructure.GPIO_OType = GPIO_OType_PP;      // 推挽输出
    GPIO_InitStructure.GPIO_PuPd = GPIO_PuPd_UP;        // 上拉电阻
    GPIO_Init(GPIOC, &GPIO_InitStructure);
    
    // 连接PC6到TIM3的通道1
    GPIO_PinAFConfig(GPIOC, GPIO_PinSource6, GPIO_AF_TIM3);
}

/**
  * @brief  TIM3 PWM初始化函数
  * @param  无
  * @retval 无
  * @函数功能：初始化TIM3为PWM输出模式，初始频率1kHz
  * @函数实例：TIM3_PWM_Init();
  * @备注：使用TIM3通道1产生PWM波形，驱动蜂鸣器发声
  */
void TIM3_PWM_Init(void)
{
    TIM_TimeBaseInitTypeDef TIM_TimeBaseStructure;
    TIM_OCInitTypeDef TIM_OCInitStructure;
    
    // 使能TIM3时钟
    RCC_APB1PeriphClockCmd(RCC_APB1Periph_TIM3, ENABLE);
    
    // 定时器基础配置 - 初始频率1kHz
    TIM_TimeBaseStructure.TIM_Prescaler = 84 - 1;        // 84MHz/84 = 1MHz
    TIM_TimeBaseStructure.TIM_CounterMode = TIM_CounterMode_Up;  // 向上计数模式
    TIM_TimeBaseStructure.TIM_Period = 1000 - 1;         // 1MHz/1000 = 1kHz
    TIM_TimeBaseStructure.TIM_ClockDivision = TIM_CKD_DIV1;      // 时钟不分频
    TIM_TimeBaseStructure.TIM_RepetitionCounter = 0;     // 重复计数器
    TIM_TimeBaseInit(TIM3, &TIM_TimeBaseStructure);
    
    // PWM模式配置
    TIM_OCInitStructure.TIM_OCMode = TIM_OCMode_PWM1;    // PWM模式1
    TIM_OCInitStructure.TIM_OutputState = TIM_OutputState_Enable;  // 使能输出
    TIM_OCInitStructure.TIM_OutputNState = TIM_OutputNState_Disable; // 禁用互补输出
    TIM_OCInitStructure.TIM_Pulse = pwm_duty_cycle;      // 初始占空比
    TIM_OCInitStructure.TIM_OCPolarity = TIM_OCPolarity_High; // 输出极性高
    TIM_OCInitStructure.TIM_OCIdleState = TIM_OCIdleState_Reset; // 空闲状态低
    TIM_OC1Init(TIM3, &TIM_OCInitStructure);
    
    // 使能TIM3预装载寄存器
    TIM_OC1PreloadConfig(TIM3, TIM_OCPreload_Enable);
    TIM_ARRPreloadConfig(TIM3, ENABLE);
    
    // 使能TIM3
    TIM_Cmd(TIM3, ENABLE);
}

/**
  * @brief  设置PWM频率函数
  * @param  freq: 所需的PWM频率(Hz)
  * @retval 无
  * @函数功能：动态设置PWM输出频率，用于产生不同音高的音符
  * @函数实例：Set_PWM_Frequency(440); // 设置频率为440Hz(A4音)
  * @备注：频率为0时关闭PWM输出，最大频率受定时器时钟限制
  */
void Set_PWM_Frequency(uint32_t freq)
{
    TIM_TimeBaseInitTypeDef TIM_TimeBaseStructure;
    uint32_t timer_clock = 84000000;  // TIM3时钟频率84MHz
    uint32_t prescaler, period;
    
    if(freq == 0 || freq > timer_clock)
    {
        // 频率为0时关闭PWM输出
        TIM_Cmd(TIM3, DISABLE);
        return;
    }
    
    // 停止定时器
    TIM_Cmd(TIM3, DISABLE);
    
    // 计算预分频器和周期值
    prescaler = 0;
    period = (timer_clock / freq) - 1;
    
    // 如果周期值超过16位最大值，需要增加预分频
    if(period > 0xFFFF)
    {
        prescaler = (period / 0xFFFF);
        period = (timer_clock / (freq * (prescaler + 1))) - 1;
    }
    
    // 重新配置定时器
    TIM_TimeBaseStructure.TIM_Prescaler = prescaler;
    TIM_TimeBaseStructure.TIM_CounterMode = TIM_CounterMode_Up;
    TIM_TimeBaseStructure.TIM_Period = period;
    TIM_TimeBaseStructure.TIM_ClockDivision = TIM_CKD_DIV1;
    TIM_TimeBaseStructure.TIM_RepetitionCounter = 0;
    TIM_TimeBaseInit(TIM3, &TIM_TimeBaseStructure);
    
    // 保持当前占空比
    TIM_SetCompare1(TIM3, (period + 1) * pwm_duty_cycle / 100);
    
    // 重新使能定时器
    TIM_Cmd(TIM3, ENABLE);
}

/**
  * @brief  系统时钟配置函数
  * @param  无
  * @retval 无
  * @函数功能：配置系统时钟和外设时钟
  * @函数实例：RCC_Configuration();
  * @备注：系统时钟配置为168MHz，使能所需外设时钟
  */
void RCC_Configuration(void)
{
    // 系统时钟配置为168MHz
    SystemInit();
    
    // 使能外设时钟
    RCC_AHB1PeriphClockCmd(RCC_AHB1Periph_GPIOC, ENABLE);
    RCC_APB1PeriphClockCmd(RCC_APB1Periph_TIM3, ENABLE);
}

/**
  * @brief  TIM2定时器初始化函数
  * @param  无
  * @retval 无
  * @函数功能：初始化TIM2用于定时中断，提供精确的时间基准
  * @函数实例：TIM2_Init();
  * @备注：TIM2配置为10ms中断，用于音符时长控制
  */
void TIM2_Init(void)
{
    TIM_TimeBaseInitTypeDef TIM_TimeBaseStructure;
    NVIC_InitTypeDef NVIC_InitStructure;
    
    // 使能TIM2时钟
    RCC_APB1PeriphClockCmd(RCC_APB1Periph_TIM2, ENABLE);
    
    // 定时器基础配置 - 10ms中断
    // 84MHz / 8400 = 10kHz, 10kHz / 100 = 100Hz (10ms)
    TIM_TimeBaseStructure.TIM_Prescaler = 8400 - 1;      // 分频到10kHz
    TIM_TimeBaseStructure.TIM_CounterMode = TIM_CounterMode_Up;
    TIM_TimeBaseStructure.TIM_Period = 100 - 1;          // 100个计数 = 10ms
    TIM_TimeBaseStructure.TIM_ClockDivision = TIM_CKD_DIV1;
    TIM_TimeBaseStructure.TIM_RepetitionCounter = 0;
    TIM_TimeBaseInit(TIM2, &TIM_TimeBaseStructure);
    
    // 使能TIM2更新中断
    TIM_ITConfig(TIM2, TIM_IT_Update, ENABLE);
    
    // 配置NVIC
    NVIC_InitStructure.NVIC_IRQChannel = TIM2_IRQn;
    NVIC_InitStructure.NVIC_IRQChannelPreemptionPriority = 1;
    NVIC_InitStructure.NVIC_IRQChannelSubPriority = 0;
    NVIC_InitStructure.NVIC_IRQChannelCmd = ENABLE;
    NVIC_Init(&NVIC_InitStructure);
    
    // 使能TIM2
    TIM_Cmd(TIM2, ENABLE);
}

// TIM2中断计数器
volatile uint32_t timer2_counter = 0;

/**
  * @brief  毫秒级延时函数
  * @param  ms: 要延时的毫秒数
  * @retval 无
  * @函数功能：基于TIM2定时器的精确毫秒延时
  * @函数实例：delay_ms(500); // 延时500ms
  * @备注：依赖TIM2的10ms中断，精度±10ms
  */
void delay_ms(uint32_t ms)
{
    uint32_t start_time = timer2_counter;
    
    // 计算需要等待的定时器节拍数
    // 每10ms一个节拍，所以ms/10就是需要的节拍数
    uint32_t ticks_to_wait = (ms + 9) / 10;  // 向上取整
    
    // 等待足够的时间节拍
    while ((timer2_counter - start_time) < ticks_to_wait)
    {
        // 空循环等待
    }
}

/**
  * @brief  秒级延时函数
  * @param  seconds: 要延时的秒数
  * @retval 无
  * @函数功能：基于delay_ms的秒级延时
  * @函数实例：delay_s(2); // 延时2秒
  * @备注：调用delay_ms实现，精度与delay_ms相同
  */
void delay_s(uint32_t seconds)
{
    delay_ms(seconds * 1000);
}

// ============================ 音符频率定义 ============================
// C3到B5音符频率定义 (单位: Hz)

// 低音区 C3-B3
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

// 中音区 C4-B4
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

// 高音区 C5-B5
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

// ============================ 简谱音符宏定义 ============================
// 方便演奏的简谱音符定义

// 中音区音符
#define One_Zhong   Set_PWM_Frequency(C4)  // 中音1
#define Two_Zhong   Set_PWM_Frequency(D4)  // 中音2
#define Three_Zhong Set_PWM_Frequency(E4)  // 中音3
#define Four_Zhong  Set_PWM_Frequency(F4)  // 中音4
#define Five_Zhong  Set_PWM_Frequency(G4)  // 中音5
#define Six_Zhong   Set_PWM_Frequency(A4)  // 中音6
#define Seven_Zhong Set_PWM_Frequency(B4)  // 中音7

// 高音区音符
#define One_High    Set_PWM_Frequency(C5)  // 高音1
#define Two_High    Set_PWM_Frequency(D5)  // 高音2
#define Three_High  Set_PWM_Frequency(E5)  // 高音3
#define Four_High   Set_PWM_Frequency(F5)  // 高音4
#define Five_High   Set_PWM_Frequency(G5)  // 高音5
#define Six_High    Set_PWM_Frequency(A5)  // 高音6
#define Seven_High  Set_PWM_Frequency(B5)  // 高音7

// 低音区音符
#define One_Low     Set_PWM_Frequency(C3)  // 低音1
#define Two_Low     Set_PWM_Frequency(D3)  // 低音2
#define Three_Low   Set_PWM_Frequency(E3)  // 低音3
#define Four_Low    Set_PWM_Frequency(F3)  // 低音4
#define Five_Low    Set_PWM_Frequency(G3)  // 低音5
#define Six_Low     Set_PWM_Frequency(A3)  // 低音6
#define Seven_Low   Set_PWM_Frequency(B3)  // 低音7

// 休止符（关闭声音）
#define REST Set_PWM_Frequency(0)

// ============================ 节拍时间定义 ============================
// 使用精准延时重新定义节拍时间（单位：毫秒）
#define WHOLE_NOTE      500   // 全音符：500ms
#define QUARTER_NOTE    WHOLE_NOTE/4   // 四分音符：125ms
#define HALF_NOTE       WHOLE_NOTE/2   // 二分音符：250ms
#define EIGHTH_NOTE     WHOLE_NOTE/8   // 八分音符：62.5ms

/**
  * @brief  播放乐曲函数
  * @param  arr: 乐曲数据二维数组
  * @param  rows: 乐曲数据行数
  * @retval 无
  * @函数功能：根据输入的乐曲数据播放完整的音乐
  * @函数实例：BoFangGeQu(xiaoXingXing, sizeof(xiaoXingXing)/sizeof(xiaoXingXing[0]));
  * @备注：乐曲数据格式为[音区, 音调, 时值]，休止符为[0,0,时值]
  */
void BoFangGeQu(int arr[][3], int rows) 
{
    uint32_t duration;
    // 定义音符频率表 [音区][音调] -> 频率
    uint16_t note_freq[3][7] = {
        // 低音区 (0)
        {C3, D3, E3, F3, G3, A3, B3},
        // 中音区 (1)  
        {C4, D4, E4, F4, G4, A4, B4},
        // 高音区 (2)
        {C5, D5, E5, F5, G5, A5, B5}
    };
    
    // 定义时值映射表 [时值] -> 持续时间(ms)
    // 时值1: 全音符，时值2: 二分音符，时值4: 四分音符，时值8: 八分音符
    uint32_t duration_map[] = {
        0,      // 0 - 无效
        WHOLE_NOTE,     // 1 - 全音符
        HALF_NOTE,      // 2 - 二分音符  
        HALF_NOTE + QUARTER_NOTE, // 3 - 二分音符+四分音符（附点二分音符）
        QUARTER_NOTE,   // 4 - 四分音符
        0,              // 5 - 无效
        0,              // 6 - 无效
        0,              // 7 - 无效
        EIGHTH_NOTE     // 8 - 八分音符
    };
    int i;
    
    // 遍历二维数组中的每个音符
    for ( i = 0; i < rows; i++) {
        int yinQu = arr[i][0];     // 音区：0-低音，1-中音，2-高音
        int yinDiao = arr[i][1];   // 音调：1-7对应do,re,mi,fa,sol,la,si
        int shiZhi = arr[i][2];    // 时值：1-全音符，2-二分音符，4-四分音符，8-八分音符
        
        // 计算持续时间
        if (shiZhi == 3) {
            duration = HALF_NOTE + QUARTER_NOTE;  // 附点二分音符
        } else {
            duration = duration_map[shiZhi];
        }
        
        // 检查是否为休止符 (两个0表示休止符)
        if (yinQu == 0 && yinDiao == 0) {
            // 休止符 - 直接延时
            REST;
            delay_ms(duration);
            continue; // 跳过后续处理
        }
        
        // 验证输入参数的合法性
        if (yinQu < 0 || yinQu > 2) {
            // 无效音区，播放休止符
            REST;
            delay_ms(duration);
        }
        else if (yinDiao < 1 || yinDiao > 7) {
            // 无效音调，播放休止符
            REST;
            delay_ms(duration);
        }
        else if (shiZhi != 1 && shiZhi != 2 && shiZhi != 4 && shiZhi != 8) {
            // 无效时值，使用默认四分音符
            Set_PWM_Frequency(note_freq[yinQu][yinDiao-1]);
            delay_ms(QUARTER_NOTE);
        }
        else {
            // 播放有效音符
            Set_PWM_Frequency(note_freq[yinQu][yinDiao-1]);
            delay_ms(duration);
            
            // 可选：在音符之间添加短暂的间隔，使音乐更清晰
            if (i < rows - 1) { // 不是最后一个音符
                REST;
                delay_ms(20); // 20ms的间隔
            }
        }
    }
    
    // 播放结束后关闭声音
    REST;
}

// ============================ 乐曲数据定义 ============================

/**
  * 《小星星》乐曲数据
  * 格式：[音区, 音符, 时值]
  * 音区: 1-中音, 2-高音
  * 音符: 1-7代表do,re,mi,fa,sol,la,si
  * 时值: 4-四分音符, 2-二分音符
  */
int xiaoXingXing[][3] = {
    {1, 1, 4},  // 中音1，四分音符
    {1, 1, 4},  // 中音1，四分音符
    {2, 5, 4},  // 高音5，四分音符
    {2, 5, 4},  // 高音5，四分音符
    {2, 6, 4},  // 高音6，四分音符
    {2, 6, 4},  // 高音6，四分音符
    {2, 5, 2},  // 高音5，二分音符
    
    {2, 4, 4},  // 高音4，四分音符
    {2, 4, 4},  // 高音4，四分音符
    {2, 3, 4},  // 高音3，四分音符
    {2, 3, 4},  // 高音3，四分音符
    {2, 2, 4},  // 高音2，四分音符
    {2, 2, 4},  // 高音2，四分音符
    {1, 1, 2},  // 中音1，二分音符
};

/**
  * 《名侦探柯南》主题曲乐曲数据
  */
		// 乐曲数据数组
// 每行三个值：[音区, 音符, 时值]
// 音区: 0-低音, 1-中音, 2-高音
// 音符: 1-7代表do, re, mi, fa, sol, la, si
// 时值: 表示这个音符持续的时间是基准节拍的几分之一
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
  * 《JIALEBI》乐曲数据
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
  * @brief  主函数
  * @param  无
  * @retval int 程序执行状态
  * @函数功能：程序入口，初始化硬件并播放音乐
  * @函数实例：无
  * @备注：程序执行完毕后进入无限循环
  */
int main(void)
{
    // 系统初始化
    RCC_Configuration();
    
    // GPIO初始化
    GPIO1_Init();
    
    // 按键初始化
    key_init();
    
    // PWM初始化
    TIM3_PWM_Init();
    TIM2_Init();
    Serial_Init();
    
    // 设置初始占空比
    TIM_SetCompare1(TIM3, 50);
    
    // 播放JIALEBI乐曲
    BoFangGeQu(JIALEBI, 200);
    
    // 关闭PWM
    Set_PWM_Frequency(0);
    
    // 主循环
    while(1)
    {
        // 主循环 - 播放完成后保持静音状态
    }
}

/**
  * @brief  TIM2中断服务函数
  * @param  无
  * @retval 无
  * @函数功能：TIM2定时器中断处理，每10ms执行一次
  * @函数实例：自动调用，无需手动调用
  * @备注：用于维护时间计数器，支持精确延时功能
  */
void TIM2_IRQHandler(void)
{
    if (TIM_GetITStatus(TIM2, TIM_IT_Update) != RESET)
    {
        TIM_ClearITPendingBit(TIM2, TIM_IT_Update);
        timer2_counter++;  // 计数器递增，用于精确延时
    }
}
