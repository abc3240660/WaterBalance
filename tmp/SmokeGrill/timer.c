#include "timer.h"
#include "led.h"
#include "GUI.h"
#include "usart.h"
#include "includes.h"	
#include "dma.h"
//////////////////////////////////////////////////////////////////////////////////	 
//本程序只供学习使用，未经作者许可，不得用于其它任何用途
//ALIENTEK 战舰开发板
//定时器 驱动代码	   
//正点原子@ALIENTEK
//技术论坛:www.openedv.com
//创建日期:2015/1/13
//版本：V1.0
//版权所有，盗版必究。
//Copyright(C) 广州市星翼电子科技有限公司 2014-2024
//All rights reserved									  
////////////////////////////////////////////////////////////////////////////////// 	 

extern u8 g_ota_runing;
extern u32 g_ota_recv_sum;
extern u16 g_ota_one_pg_recv_tms;
extern u16 g_ota_pg_numid;

extern void counter_process();
	
void TIM3_Int_Init(u16 arr,u16 psc)
{
	TIM_TimeBaseInitTypeDef	TIM_TimeBaseInitStructure;
	NVIC_InitTypeDef NVIC_InitStructure;
	RCC_APB1PeriphClockCmd(RCC_APB1Periph_TIM3,ENABLE);//开启TIM3时钟 

	TIM_TimeBaseInitStructure.TIM_Prescaler=psc;   //分频值
	TIM_TimeBaseInitStructure.TIM_CounterMode=TIM_CounterMode_Up;	   //计数模式
	TIM_TimeBaseInitStructure.TIM_Period=arr;		   //自动重装数值
	TIM_TimeBaseInitStructure.TIM_ClockDivision=TIM_CKD_DIV1;  //设置时钟分割
	TIM_TimeBaseInit(TIM3,&TIM_TimeBaseInitStructure);
	TIM_ITConfig(TIM3,TIM_IT_Update,ENABLE);//允许更新中断

	NVIC_InitStructure.NVIC_IRQChannel=TIM3_IRQn;
	NVIC_InitStructure.NVIC_IRQChannelPreemptionPriority=0;
	NVIC_InitStructure.NVIC_IRQChannelSubPriority=3;
	NVIC_InitStructure.NVIC_IRQChannelCmd=ENABLE;
	NVIC_Init(&NVIC_InitStructure);

	TIM_Cmd(TIM3,ENABLE);		  //使能TIM3
}

void TIM3_IRQHandler(void)
{
	if(TIM_GetITStatus(TIM3,TIM_IT_Update)!=RESET)
	{
		counter_process();
		//OS_TimeMS++;
	}
	TIM_ClearITPendingBit(TIM3,TIM_IT_Update);
}

//基本定时器6中断初始化
//这里时钟选择为APB1的2倍，而APB1为36M
//arr：自动重装值。
//psc：时钟预分频数
//这里使用的是定时器6!
void TIM6_Int_Init(u16 arr,u16 psc)
{
	TIM_TimeBaseInitTypeDef TIM_TimeBaseInitStructure;
	NVIC_InitTypeDef NVIC_InitStructure;
	RCC_APB1PeriphClockCmd(RCC_APB1Periph_TIM6,ENABLE); //定时器6时钟使能
	
	TIM_TimeBaseInitStructure.TIM_Prescaler=psc;  //设置分频值，10khz的计数频率
	TIM_TimeBaseInitStructure.TIM_CounterMode=TIM_CounterMode_Up; //向上计数模式
	TIM_TimeBaseInitStructure.TIM_Period=arr;  //自动重装载值 计数到5000为500ms
	TIM_TimeBaseInitStructure.TIM_ClockDivision=0; //时钟分割:TDS=Tck_Tim
	TIM_TimeBaseInit(TIM6,&TIM_TimeBaseInitStructure);
	
	TIM_ITConfig(TIM6,TIM_IT_Update,ENABLE); //使能TIM6的更新中断

	NVIC_InitStructure.NVIC_IRQChannel=TIM6_IRQn; //TIM6中断
	NVIC_InitStructure.NVIC_IRQChannelPreemptionPriority=1; //先占优先级1级
	NVIC_InitStructure.NVIC_IRQChannelSubPriority=3;  //从优先级3级
	NVIC_InitStructure.NVIC_IRQChannelCmd=ENABLE; //使能通道
	NVIC_Init(&NVIC_InitStructure);
}


extern u32 USART_RX_STA;
void TIM6_IRQHandler(void)
{
	OSIntEnter();
	if(TIM_GetITStatus(TIM6,TIM_IT_Update)!=RESET)
	{
		if (USART_RX_STA != 0) {
			u16 re_count = 0;
		
			DMA_Cmd(DMA1_Channel5, DISABLE);
			re_count = U1_DMA_R_LEN - DMA_GetCurrDataCounter(DMA1_Channel5);
			
			if((USART_RX_STA&(1<<19))==0)
			{ 
				if(USART_RX_STA<USART_MAX_RECV_LEN)
				{
					if (re_count < (USART_MAX_RECV_LEN - USART_RX_STA)) {
						memcpy(USART_RX_BUF+(USART_RX_STA&0xFFFF), &U1_DMA_R_BUF[0], re_count);
						USART_RX_STA += re_count;
					} else {
						memcpy(USART_RX_BUF+(USART_RX_STA&0xFFFF), &U1_DMA_R_BUF[0], (USART_MAX_RECV_LEN - USART_RX_STA));
						USART_RX_STA += (USART_MAX_RECV_LEN - USART_RX_STA);
						USART_RX_STA|=1<<19;
					}
				}else 
				{
					USART_RX_STA|=1<<19;
				}
			} else {
				// Buffer Overflow
				// Error MSG printf if NEED
			}
			
			USART_RX_STA|=1<<19;
			
			g_ota_pg_numid++;
			g_ota_recv_sum += USART_RX_STA&0xFFFF;
			printf("packet(%d), size=0x%.4X, total=0x%.6X\n", g_ota_pg_numid, (USART_RX_STA&0xFFFF), g_ota_recv_sum);
		}

	    TIM_ClearITPendingBit(TIM6,TIM_IT_Update); //清除中断标志位
		
		TIM_Cmd(TIM6,DISABLE);
	}
	OSIntExit();
}




