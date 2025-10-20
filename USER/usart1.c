
#include "stm32f4xx.h"                  // Device header for F4
#include <stdio.h>
#include <stdarg.h>
#include "usart1.h"

char Serial_RxPacket[100];              // 接收数据包数组，格式: @MSG\r\n
uint8_t Serial_RxFlag = 0;              // 接收数据包标志位

/**
  * 函数名：串口初始化
  * 参数：无
  * 返回值：无
  */
void Serial_Init(void)
{
    /* 所有变量声明放在函数开头 */
    GPIO_InitTypeDef GPIO_InitStructure;
    USART_InitTypeDef USART_InitStructure;
    NVIC_InitTypeDef NVIC_InitStructure;

    /* 开启时钟 */
    RCC_APB2PeriphClockCmd(RCC_APB2Periph_USART1, ENABLE);   // 开启USART1的时钟
    RCC_AHB1PeriphClockCmd(RCC_AHB1Periph_GPIOA, ENABLE);    // 开启GPIOA的时钟
    
    /* GPIO初始化 */
    // TX (PA9) - 复用推挽输出
    GPIO_InitStructure.GPIO_Mode = GPIO_Mode_AF;
    GPIO_InitStructure.GPIO_Pin = GPIO_Pin_9;
    GPIO_InitStructure.GPIO_Speed = GPIO_Speed_100MHz;
    GPIO_InitStructure.GPIO_OType = GPIO_OType_PP;
    GPIO_InitStructure.GPIO_PuPd = GPIO_PuPd_UP;
    GPIO_Init(GPIOA, &GPIO_InitStructure);
    
    // RX (PA10) - 浮空输入
    GPIO_InitStructure.GPIO_Mode = GPIO_Mode_AF;
    GPIO_InitStructure.GPIO_Pin = GPIO_Pin_10;
    GPIO_InitStructure.GPIO_PuPd = GPIO_PuPd_NOPULL;
    GPIO_Init(GPIOA, &GPIO_InitStructure);
    
    // 配置引脚复用
    GPIO_PinAFConfig(GPIOA, GPIO_PinSource9, GPIO_AF_USART1);
    GPIO_PinAFConfig(GPIOA, GPIO_PinSource10, GPIO_AF_USART1);
    
    /* USART初始化 */
    USART_InitStructure.USART_BaudRate = 115200;             // 波特率
    USART_InitStructure.USART_HardwareFlowControl = USART_HardwareFlowControl_None; // 硬件流控制，不需要
    USART_InitStructure.USART_Mode = USART_Mode_Tx | USART_Mode_Rx; // 模式，发送模式和接收模式均选择
    USART_InitStructure.USART_Parity = USART_Parity_No;      // 奇偶校验，不需要
    USART_InitStructure.USART_StopBits = USART_StopBits_1;   // 停止位，选择1位
    USART_InitStructure.USART_WordLength = USART_WordLength_8b;     // 字长，选择8位
    USART_Init(USART1, &USART_InitStructure);                // 配置USART1
    
    /* 中断输出配置 */
    USART_ITConfig(USART1, USART_IT_RXNE, ENABLE);           // 开启串口接收数据的中断
    
    /* NVIC中断分组 */
    NVIC_PriorityGroupConfig(NVIC_PriorityGroup_2);          // 配置NVIC为分组2
    
    /* NVIC配置 */
    NVIC_InitStructure.NVIC_IRQChannel = USART1_IRQn;        // 选择配置NVIC的USART1线
    NVIC_InitStructure.NVIC_IRQChannelCmd = ENABLE;          // 指定NVIC线路使能
    NVIC_InitStructure.NVIC_IRQChannelPreemptionPriority = 1; // 指定NVIC线路的抢占优先级为1
    NVIC_InitStructure.NVIC_IRQChannelSubPriority = 1;       // 指定NVIC线路的响应优先级为1
    NVIC_Init(&NVIC_InitStructure);                          // 配置NVIC外设
    
    /* USART使能 */
    USART_Cmd(USART1, ENABLE);                               // 使能USART1，串口开始运行
}
/**
  * 函数名：串口发送一个字节
  * 参数：Byte 要发送的一个字节
  * 返回值：无
  */
void Serial_SendByte(uint8_t Byte)
{
    USART_SendData(USART1, Byte);        // 将字节数据写入数据寄存器，写入后USART自动生成时序波形
    while (USART_GetFlagStatus(USART1, USART_FLAG_TXE) == RESET); // 等待发送完成
    /* 下次写入数据寄存器会自动清除发送完成标志位，故此循环后，无需清除标志位 */
}

/**
  * 函数名：串口发送一个数组
  * 参数：Array 要发送数组的首地址
  * 参数：Length 要发送数组的长度
  * 返回值：无
  */
void Serial_SendArray(uint8_t *Array, uint16_t Length)
{
    uint16_t i;
    for (i = 0; i < Length; i ++)       // 遍历数组
    {
        Serial_SendByte(Array[i]);      // 依次调用Serial_SendByte发送每个字节数据
    }
}

/**
  * 函数名：串口发送一个字符串
  * 参数：String 要发送字符串的首地址
  * 返回值：无
  */
void Serial_SendString(char *String)
{
    uint8_t i;
    for (i = 0; String[i] != '\0'; i ++) // 遍历字符数组（字符串），遇到字符串结束标志后停止
    {
        Serial_SendByte(String[i]);     // 依次调用Serial_SendByte发送每个字节数据
    }
}

/**
  * 函数名：次方函数（内部使用）
  * 返回值：返回X的Y次方
  */
uint32_t Serial_Pow(uint32_t X, uint32_t Y)
{
    uint32_t Result = 1;    // 设置结果初值为1
    while (Y --)            // 执行Y次
    {
        Result *= X;        // 将X累乘到结果
    }
    return Result;
}

/**
  * 函数名：串口发送数字
  * 参数：Number 要发送的数字，范围：0~4294967295
  * 参数：Length 要发送数字的长度，范围：0~10
  * 返回值：无
  */
void Serial_SendNumber(uint32_t Number, uint8_t Length)
{
    uint8_t i;
    for (i = 0; i < Length; i ++)       // 根据数字长度遍历数字的每一位
    {
        Serial_SendByte(Number / Serial_Pow(10, Length - i - 1) % 10 + '0'); // 依次调用Serial_SendByte发送每位数字
    }
}

/**
  * 函数名：使用printf需要重定向的底层函数
  * 参数：保持原始格式即可，无需变动
  * 返回值：保持原始格式即可，无需变动
  */
int fputc(int ch, FILE *f)
{
    Serial_SendByte(ch);            // 将printf的底层重定向到自己的发送字节函数
    return ch;
}

/**
  * 函数名：自己封装的printf函数
  * 参数：format 格式化字符串
  * 参数：... 可变的参数列表
  * 返回值：无
  */
void Serial_Printf(char *format, ...)
{
    char String[100];               // 定义字符数组
    va_list arg;                    // 定义可参数列表数据类型的变量arg
    va_start(arg, format);          // 从format开始，接收参数列表到arg变量
    vsprintf(String, format, arg);  // 使用vsprintf打印格式化字符串和参数列表到字符数组
    va_end(arg);                    // 结束变量arg
    Serial_SendString(String);      // 串口发送字符数组（字符串）
}

/**
  * 函数名：USART1中断函数
  * 参数：无
  * 返回值：无
  * 注意事项：此函数为中断函数，无需调用，中断触发后自动执行
  *           函数名为预留的指定名称，可以从启动文件复制
  *           请确保函数名正确，不能有任何差异，否则中断函数将不能进入
  */
void USART1_IRQHandler(void)
{
    static uint8_t RxState = 0;     // 定义表示当前状态机状态的静态变量
    static uint8_t pRxPacket = 0;   // 定义表示当前接收数据位置的静态变量
    
    // 检查接收中断标志
    if (USART_GetITStatus(USART1, USART_IT_RXNE) == SET)
    {
        uint8_t RxData = USART_ReceiveData(USART1);         // 读取数据寄存器，存放在接收的数据变量
        
        /* 使用状态机的思路，依次处理数据包的不同部分 */
        
        /* 当前状态为0，接收数据包包头 */
        if (RxState == 0)
        {
            if (RxData == '@' && Serial_RxFlag == 0)        // 如果数据确实是包头，并且上一个数据包已处理完
            {
                RxState = 1;            // 设置下一个状态
                pRxPacket = 0;          // 数据包的位置归零
            }
        }
        /* 当前状态为1，接收数据包数据，同时判断是否接收到了第一个包尾 */
        else if (RxState == 1)
        {
            if (RxData == '\r')         // 如果收到了第一个包尾
            {
                RxState = 2;            // 设置下一个状态
            }
            else                        // 接收到了正常的数据
            {
                Serial_RxPacket[pRxPacket] = RxData;        // 将数据存入数据包数组的指定位置
                pRxPacket ++;           // 数据包的位置自增
            }
        }
        /* 当前状态为2，接收数据包第二个包尾 */
        else if (RxState == 2)
        {
            if (RxData == '\n')         // 如果收到了第二个包尾
            {
                RxState = 0;            // 状态归0
                Serial_RxPacket[pRxPacket] = '\0';          // 将接收的字符数据包添加一个字符串结束标志
                Serial_RxFlag = 1;      // 接收数据包标志置1，成功接收一个数据包
            }
        }
        
        USART_ClearITPendingBit(USART1, USART_IT_RXNE);     // 清除标志位
    }
    
    // 处理过载错误（F4特有）
    if (USART_GetITStatus(USART1, USART_IT_ORE) == SET)
    {
        // 必须先读取SR寄存器，然后读取DR寄存器才能清除ORE标志
        (void)USART1->SR; // 读取状态寄存器
        (void)USART1->DR; // 读取数据寄存器
        USART_ClearITPendingBit(USART1, USART_IT_ORE); // 清除中断标志
    }
}