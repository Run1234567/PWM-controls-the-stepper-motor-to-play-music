


#include "stm32f4xx.h"
#include "key.h"
/*
*--------------------------------------------------------------------------------------------------------
* Function:       key_init
* Description:    
* Input:          none
* Output:         none
* Return:         none
* Created by:     alvan 
* Created date:   2014-07-29
* Others:         	
*---------------------------------------------------------------------------------------------------------
*/
void  key_init (void)
{ 
	GPIO_InitTypeDef GPIO_InitStructure;
	
    RCC_AHB1PeriphClockCmd(RCC_AHB1Periph_GPIOE, ENABLE);  /* 使能端口时钟                        */ 

    GPIO_InitStructure.GPIO_Pin   = GPIO_Pin_4;	                             
    GPIO_InitStructure.GPIO_Speed = GPIO_Speed_2MHz;
    GPIO_InitStructure.GPIO_Mode  = GPIO_Mode_IN;
    GPIO_Init(GPIOE, &GPIO_InitStructure);                 /* 配置引脚为上拉输入 	*/	 
	
	
	RCC_AHB1PeriphClockCmd(RCC_AHB1Periph_GPIOA, ENABLE);  /* 使能端口时钟                        */ 

    GPIO_InitStructure.GPIO_Pin   = GPIO_Pin_0;	                             
    GPIO_InitStructure.GPIO_Speed = GPIO_Speed_2MHz;
    GPIO_InitStructure.GPIO_Mode  = GPIO_PuPd_DOWN ;
    GPIO_Init(GPIOA, &GPIO_InitStructure);    
}


