#include "timer.h"
#include "led.h"
#include "usart.h"
#include "includes.h"
#include "usart3.h" 
#include "usart5.h" 

extern u8 ov_frame;
extern volatile u16 jpeg_data_len;
extern void usbapp_pulling(void); 

void write_logs(char *module, char *log, u16 size, u8 mode);

vu8 framecnt;
vu8 framecntout;

void TIM2_Init(u16 auto_data,u16 fractional)
{
        GPIO_InitTypeDef GPIO_InitStructure;
        TIM_TimeBaseInitTypeDef TIM_TimeBaseInitStructure;
        
        RCC_AHB1PeriphClockCmd(RCC_AHB1Periph_GPIOA,ENABLE);
        RCC_APB1PeriphClockCmd(RCC_APB1Periph_TIM2,ENABLE);
        
        GPIO_InitStructure.GPIO_Mode=GPIO_Mode_AF;
        GPIO_InitStructure.GPIO_OType=GPIO_OType_PP;
        GPIO_InitStructure.GPIO_Pin=GPIO_Pin_0;
        GPIO_InitStructure.GPIO_PuPd=GPIO_PuPd_UP;
        GPIO_InitStructure.GPIO_Speed=GPIO_Speed_100MHz;
        GPIO_Init(GPIOA,&GPIO_InitStructure);
        GPIO_PinAFConfig(GPIOA,GPIO_PinSource0,GPIO_AF_TIM2);
	
        TIM_TimeBaseInitStructure.TIM_ClockDivision=TIM_CKD_DIV1;
        TIM_TimeBaseInitStructure.TIM_CounterMode=TIM_CounterMode_Up;
        TIM_TimeBaseInitStructure.TIM_Period=0xFFFFFFFF;
        TIM_TimeBaseInitStructure.TIM_Prescaler=0x00;
        TIM_TimeBaseInitStructure.TIM_RepetitionCounter=0;
        TIM_TimeBaseInit(TIM2,&TIM_TimeBaseInitStructure);
        
        TIM_ITRxExternalClockConfig(TIM2,TIM_TS_ETRF);
        TIM_ETRClockMode2Config(TIM2,TIM_ExtTRGPSC_OFF,TIM_ExtTRGPolarity_NonInverted,7);
        
        TIM_SetCounter(TIM2,0);
        TIM_Cmd(TIM2,ENABLE);
}

//定时器2中断服务函数
int pluse_num=0;
int pluse_num_old=0;
int pluse_num_new=0;
void TIM2_IRQHandler(void)
{
	if(TIM_GetITStatus(TIM3,TIM_IT_Update)==SET) //溢出中断
	{

		//LED0=!LED0;//LED0翻转
  	
	}
	TIM_ClearITPendingBit(TIM3,TIM_IT_CC3|TIM_IT_Update);  //清除中断标志位
}

int exp_num=0;
extern int total_ms;
int total_msTtemp=0;
int capture1=0;
int capture2=0;
void TIM4_IRQHandler(void)
{
	if(TIM_GetITStatus(TIM4, TIM_IT_Update) != RESET)//timer溢出
	{
		exp_num++;
		total_msTtemp=exp_num*1000;
		pluse_num_new = TIM_GetCounter(TIM2);//每1s中读取脉冲数
		pluse_num =pluse_num_new - pluse_num_old;
		pluse_num_old = pluse_num_new;
	}
	if(TIM_GetITStatus(TIM4, TIM_IT_CC3) != RESET)//捕获1发生捕获事件
	{
		// LED1=!LED1;//LED1翻转
		capture1= TIM_GetCapture3(TIM4)/10;
		total_msTtemp+=capture1;
		total_msTtemp-=capture2;
		capture2 = capture1;
		total_ms=total_msTtemp;
		total_msTtemp=0;
		exp_num=0;
	}
	TIM_ClearITPendingBit(TIM4, TIM_IT_CC3|TIM_IT_Update); //清除中断标志位
}


void TIM4_Init(u16 auto_data,u16 fractional)
{
	TIM_ICInitTypeDef  TIM4_ICInitStructure;
	GPIO_InitTypeDef GPIO_InitStructure;
	TIM_TimeBaseInitTypeDef  TIM_TimeBaseStructure;
	NVIC_InitTypeDef NVIC_InitStructure;

	
	RCC_APB1PeriphClockCmd(RCC_APB1Periph_TIM4,ENABLE);  	//TIM5时钟使能    
	RCC_AHB1PeriphClockCmd(RCC_AHB1Periph_GPIOD, ENABLE); 	//使能PORTA时钟	
	
	GPIO_InitStructure.GPIO_Pin = GPIO_Pin_14; //GPIOA0
	GPIO_InitStructure.GPIO_Mode = GPIO_Mode_AF;//复用功能
	GPIO_InitStructure.GPIO_Speed = GPIO_Speed_100MHz;	//速度100MHz
	GPIO_InitStructure.GPIO_OType = GPIO_OType_PP; //推挽复用输出
	GPIO_InitStructure.GPIO_PuPd = GPIO_PuPd_DOWN; //下拉
	GPIO_Init(GPIOD,&GPIO_InitStructure); //初始化PA0

	GPIO_PinAFConfig(GPIOD,GPIO_PinSource14,GPIO_AF_TIM4); //PA0复用位定时器5
  
	  
	TIM_TimeBaseStructure.TIM_Prescaler=fractional;  //定时器分频
	TIM_TimeBaseStructure.TIM_CounterMode=TIM_CounterMode_Up; //向上计数模式
	TIM_TimeBaseStructure.TIM_Period=auto_data;   //自动重装载值
	TIM_TimeBaseStructure.TIM_ClockDivision=TIM_CKD_DIV1; 
	
	TIM_TimeBaseInit(TIM4,&TIM_TimeBaseStructure);
	

	//初始化TIM5输入捕获参数
	TIM4_ICInitStructure.TIM_Channel = TIM_Channel_3; //CC1S=01 	选择输入端 IC1映射到TI1上
  TIM4_ICInitStructure.TIM_ICPolarity = TIM_ICPolarity_Rising;	//上升沿捕获
  TIM4_ICInitStructure.TIM_ICSelection = TIM_ICSelection_DirectTI; //映射到TI1上
  TIM4_ICInitStructure.TIM_ICPrescaler = TIM_ICPSC_DIV1;	 //配置输入分频,不分频 
  TIM4_ICInitStructure.TIM_ICFilter = 0x00;//IC1F=0000 配置输入滤波器 不滤波
  TIM_ICInit(TIM4, &TIM4_ICInitStructure);
		
	TIM_ITConfig(TIM4,TIM_IT_Update|TIM_IT_CC3,ENABLE);//允许更新中断 ,允许CC1IE捕获中断	
	
  TIM_Cmd(TIM4,ENABLE ); 	//使能定时器5

 
  NVIC_InitStructure.NVIC_IRQChannel = TIM4_IRQn;
	NVIC_InitStructure.NVIC_IRQChannelPreemptionPriority=2;//抢占优先级3
	NVIC_InitStructure.NVIC_IRQChannelSubPriority =0;		//子优先级3
	NVIC_InitStructure.NVIC_IRQChannelCmd = ENABLE;			//IRQ通道使能
	NVIC_Init(&NVIC_InitStructure);	//根据指定的参数初始化VIC寄存器、
}

void TIM3_IRQHandler(void)
{ 		
	OSIntEnter();   
	if(TIM_GetITStatus(TIM3,TIM_IT_Update)==SET)
	{
		if(OSRunning!=TRUE)
		{
//			usbapp_pulling();
		}
		framecntout=framecnt;
 		framecnt=0;
	}				   
		TIM_ClearITPendingBit(TIM3,TIM_IT_Update);
	OSIntExit(); 	    		  			    
}

void TIM3_Int_Init(u16 arr,u16 psc)
{
	TIM_TimeBaseInitTypeDef TIM_TimeBaseInitStructure;
	NVIC_InitTypeDef NVIC_InitStructure;
	
	RCC_APB1PeriphClockCmd(RCC_APB1Periph_TIM3,ENABLE);
	
  TIM_TimeBaseInitStructure.TIM_Period = arr;
	TIM_TimeBaseInitStructure.TIM_Prescaler=psc;
	TIM_TimeBaseInitStructure.TIM_CounterMode=TIM_CounterMode_Up;
	TIM_TimeBaseInitStructure.TIM_ClockDivision=TIM_CKD_DIV1; 
	
	TIM_TimeBaseInit(TIM3,&TIM_TimeBaseInitStructure);
	
	TIM_ITConfig(TIM3,TIM_IT_Update,ENABLE);
	TIM_Cmd(TIM3,ENABLE);
	
	NVIC_InitStructure.NVIC_IRQChannel=TIM3_IRQn;
	NVIC_InitStructure.NVIC_IRQChannelPreemptionPriority=0x02;
	NVIC_InitStructure.NVIC_IRQChannelSubPriority=0x00;
	NVIC_InitStructure.NVIC_IRQChannelCmd=ENABLE;
	NVIC_Init(&NVIC_InitStructure);							 
}

void TIM14_PWM_Init(u32 arr,u32 psc)
{		 					 
	GPIO_InitTypeDef GPIO_InitStructure;
	TIM_TimeBaseInitTypeDef  TIM_TimeBaseStructure;
	TIM_OCInitTypeDef  TIM_OCInitStructure;
	
	RCC_APB1PeriphClockCmd(RCC_APB1Periph_TIM14,ENABLE);
	RCC_AHB1PeriphClockCmd(RCC_AHB1Periph_GPIOF, ENABLE);
	
	GPIO_PinAFConfig(GPIOF,GPIO_PinSource9,GPIO_AF_TIM14);
	
	GPIO_InitStructure.GPIO_Pin = GPIO_Pin_9;
	GPIO_InitStructure.GPIO_Mode = GPIO_Mode_AF;
	GPIO_InitStructure.GPIO_Speed = GPIO_Speed_100MHz;
	GPIO_InitStructure.GPIO_OType = GPIO_OType_PP;
	GPIO_InitStructure.GPIO_PuPd = GPIO_PuPd_UP;
	GPIO_Init(GPIOF,&GPIO_InitStructure);
	  
	TIM_TimeBaseStructure.TIM_Prescaler=psc;
	TIM_TimeBaseStructure.TIM_CounterMode=TIM_CounterMode_Up;
	TIM_TimeBaseStructure.TIM_Period=arr;
	TIM_TimeBaseStructure.TIM_ClockDivision=TIM_CKD_DIV1; 
	
	TIM_TimeBaseInit(TIM14,&TIM_TimeBaseStructure);
	
	TIM_OCInitStructure.TIM_OCMode = TIM_OCMode_PWM1;
 	TIM_OCInitStructure.TIM_OutputState = TIM_OutputState_Enable;
	TIM_OCInitStructure.TIM_OCPolarity = TIM_OCPolarity_Low;
	TIM_OC1Init(TIM14, &TIM_OCInitStructure);

	TIM_OC1PreloadConfig(TIM14, TIM_OCPreload_Enable);
 
  TIM_ARRPreloadConfig(TIM14,ENABLE);
	
	TIM_Cmd(TIM14, ENABLE);
}  

TIM_ICInitTypeDef  TIM5_ICInitStructure;
void TIM5_CH1_Cap_Init(u32 arr,u16 psc)
{		 
	GPIO_InitTypeDef GPIO_InitStructure;
	TIM_TimeBaseInitTypeDef  TIM_TimeBaseStructure;
	NVIC_InitTypeDef NVIC_InitStructure;

	
	RCC_APB1PeriphClockCmd(RCC_APB1Periph_TIM5,ENABLE);
	RCC_AHB1PeriphClockCmd(RCC_AHB1Periph_GPIOA, ENABLE);
	
	GPIO_InitStructure.GPIO_Pin = GPIO_Pin_0;
	GPIO_InitStructure.GPIO_Mode = GPIO_Mode_AF;
	GPIO_InitStructure.GPIO_Speed = GPIO_Speed_100MHz;
	GPIO_InitStructure.GPIO_OType = GPIO_OType_PP;
	GPIO_InitStructure.GPIO_PuPd = GPIO_PuPd_DOWN;
	GPIO_Init(GPIOA,&GPIO_InitStructure);

	GPIO_PinAFConfig(GPIOA,GPIO_PinSource0,GPIO_AF_TIM5);
  
	  
	TIM_TimeBaseStructure.TIM_Prescaler=psc;
	TIM_TimeBaseStructure.TIM_CounterMode=TIM_CounterMode_Up;
	TIM_TimeBaseStructure.TIM_Period=arr;
	TIM_TimeBaseStructure.TIM_ClockDivision=TIM_CKD_DIV1; 
	
	TIM_TimeBaseInit(TIM5,&TIM_TimeBaseStructure);
	

	TIM5_ICInitStructure.TIM_Channel = TIM_Channel_1;
  TIM5_ICInitStructure.TIM_ICPolarity = TIM_ICPolarity_Rising;
  TIM5_ICInitStructure.TIM_ICSelection = TIM_ICSelection_DirectTI;
  TIM5_ICInitStructure.TIM_ICPrescaler = TIM_ICPSC_DIV1;
  TIM5_ICInitStructure.TIM_ICFilter = 0x00;
  TIM_ICInit(TIM5, &TIM5_ICInitStructure);
		
	TIM_ITConfig(TIM5,TIM_IT_Update|TIM_IT_CC1,ENABLE);
	
  TIM_Cmd(TIM5,ENABLE );

 
  NVIC_InitStructure.NVIC_IRQChannel = TIM5_IRQn;
	NVIC_InitStructure.NVIC_IRQChannelPreemptionPriority=2;
	NVIC_InitStructure.NVIC_IRQChannelSubPriority =0;
	NVIC_InitStructure.NVIC_IRQChannelCmd = ENABLE;
	NVIC_Init(&NVIC_InitStructure);
}

u8  TIM5CH1_CAPTURE_STA=0;
u32	TIM5CH1_CAPTURE_VAL;
void TIM5_IRQHandler(void)
{ 		    
	if((TIM5CH1_CAPTURE_STA&0X80)==0)
	{
		if(TIM_GetITStatus(TIM5, TIM_IT_Update) != RESET)
		{	     
			if(TIM5CH1_CAPTURE_STA&0X40)
			{
				if((TIM5CH1_CAPTURE_STA&0X3F)==0X3F)
				{
					TIM5CH1_CAPTURE_STA|=0X80;
					TIM5CH1_CAPTURE_VAL=0XFFFFFFFF;
				}else TIM5CH1_CAPTURE_STA++;
			}	 
		}
		if(TIM_GetITStatus(TIM5, TIM_IT_CC1) != RESET)
		{	
			if(TIM5CH1_CAPTURE_STA&0X40)
			{	  			
				TIM5CH1_CAPTURE_STA|=0X80;
			  TIM5CH1_CAPTURE_VAL=TIM_GetCapture1(TIM5);
	 			TIM_OC1PolarityConfig(TIM5,TIM_ICPolarity_Rising);
			}else
			{
				TIM5CH1_CAPTURE_STA=0;
				TIM5CH1_CAPTURE_VAL=0;
				TIM5CH1_CAPTURE_STA|=0X40;
				TIM_Cmd(TIM5,ENABLE );
	 			TIM_SetCounter(TIM5,0);
	 			TIM_OC1PolarityConfig(TIM5,TIM_ICPolarity_Falling);
				TIM_Cmd(TIM5,ENABLE );
			}		    
		}			     	    					   
 	}
	TIM_ClearITPendingBit(TIM5, TIM_IT_CC1|TIM_IT_Update);
} 
void TIM9_CH2_PWM_Init(u16 arr,u16 psc)
{		 					 
	GPIO_InitTypeDef GPIO_InitStructure;
	TIM_TimeBaseInitTypeDef  TIM_TimeBaseStructure;
	TIM_OCInitTypeDef  TIM_OCInitStructure;
	
	
	RCC_APB2PeriphClockCmd(RCC_APB2Periph_TIM9,ENABLE);
	RCC_AHB1PeriphClockCmd(RCC_AHB1Periph_GPIOA, ENABLE);
	
	GPIO_InitStructure.GPIO_Pin = GPIO_Pin_3;
	GPIO_InitStructure.GPIO_Mode = GPIO_Mode_AF;
	GPIO_InitStructure.GPIO_Speed = GPIO_Speed_100MHz;
	GPIO_InitStructure.GPIO_OType = GPIO_OType_PP;
	GPIO_InitStructure.GPIO_PuPd = GPIO_PuPd_UP;
	GPIO_Init(GPIOA,&GPIO_InitStructure);
 
	GPIO_PinAFConfig(GPIOA,GPIO_PinSource3,GPIO_AF_TIM9);

	TIM_TimeBaseStructure.TIM_Prescaler=psc;
	TIM_TimeBaseStructure.TIM_CounterMode=TIM_CounterMode_Up;
	TIM_TimeBaseStructure.TIM_Period=arr;
	TIM_TimeBaseStructure.TIM_ClockDivision=TIM_CKD_DIV1; 
	TIM_TimeBaseInit(TIM9,&TIM_TimeBaseStructure);

	TIM_OCInitStructure.TIM_OCMode = TIM_OCMode_PWM1;
 	TIM_OCInitStructure.TIM_OutputState = TIM_OutputState_Enable;
	TIM_OCInitStructure.TIM_OCPolarity = TIM_OCPolarity_High;
	TIM_OCInitStructure.TIM_Pulse=0;
	TIM_OC2Init(TIM9, &TIM_OCInitStructure);

	TIM_OC2PreloadConfig(TIM9, TIM_OCPreload_Enable);
 
  TIM_ARRPreloadConfig(TIM9,ENABLE);
	
	TIM_Cmd(TIM9, ENABLE);
}  
extern vu16 UART5_RX_STA;
void TIM6_DAC_IRQHandler(void)
{
	OSIntEnter();    		    
	if(TIM_GetITStatus(TIM6,TIM_IT_Update)==SET)
	{ 
		if (UART5_RX_STA != 0) {
			UART5_RX_STA|=1<<15;
			UART5_RX_BUF[UART5_RX_STA&0X7FFF]=0;
			//printf("RFID RECVED:");
			//for (i=0; i<(UART5_RX_STA&0X7FFF); i++) {
			//	printf("%.2X ", UART5_RX_BUF[i]);
			//}
			//printf("\n");
		}

		TIM_ClearITPendingBit(TIM6,TIM_IT_Update);
		TIM_Cmd(TIM6,DISABLE);
	}				   
	OSIntExit();  											 
}
void TIM6_Int_Init(u16 arr,u16 psc)
{
	TIM_TimeBaseInitTypeDef TIM_TimeBaseInitStructure;
	NVIC_InitTypeDef NVIC_InitStructure;
	
	RCC_APB1PeriphClockCmd(RCC_APB1Periph_TIM6,ENABLE);
	
	TIM_TimeBaseInitStructure.TIM_Period = arr;
	TIM_TimeBaseInitStructure.TIM_Prescaler=psc;
	TIM_TimeBaseInitStructure.TIM_CounterMode=TIM_CounterMode_Up;
	TIM_TimeBaseInitStructure.TIM_ClockDivision=TIM_CKD_DIV1; 
	
	TIM_TimeBaseInit(TIM6,&TIM_TimeBaseInitStructure);
	
	TIM_ITConfig(TIM6,TIM_IT_Update,ENABLE);
	TIM_Cmd(TIM6,ENABLE);
	
	NVIC_InitStructure.NVIC_IRQChannel=TIM6_DAC_IRQn;  
	NVIC_InitStructure.NVIC_IRQChannelPreemptionPriority=0x00;
	NVIC_InitStructure.NVIC_IRQChannelSubPriority=0x03;
	NVIC_InitStructure.NVIC_IRQChannelCmd=ENABLE;
	NVIC_Init(&NVIC_InitStructure); 									 
}
extern u8 U3_RX_ID;// 0~3
extern vu16 USART3_RX_STA[4];
extern void USART3_Get_Free_Buf(void);
void TIM7_IRQHandler(void)
{ 	
	OSIntEnter();    		    
	if(TIM_GetITStatus(TIM7,TIM_IT_Update)==SET)
	{	 			   
		if (USART3_RX_STA[U3_RX_ID] != 0) {
			USART3_RX_STA[U3_RX_ID] |= 1<<15;
			USART3_RX_BUF[U3_RECV_LEN_ONE*U3_RX_ID + USART3_RX_STA[U3_RX_ID]&0X7FFF] = 0;
			printf("SIM7K RECVED(%dB): %s\n", USART3_RX_STA[U3_RX_ID]&0X7FFF, USART3_RX_BUF+U3_RECV_LEN_ONE*U3_RX_ID);
			//printf("SIM7000E Recv Data %s\n", USART3_RX_BUF+U3_RECV_LEN_ONE*U3_RX_ID);
			//write_logs("SIM7000E", (char*)USART3_RX_BUF+U3_RECV_LEN_ONE*U3_RX_ID, USART3_RX_STA[U3_RX_ID]&0X7FFF, 0);

			// Not Busy:    0->1->0->1->0->1->... (always switch between 0 and 1)
			// little Busy: 0->1->2->0->1->0->1->... (may use till 2)
			// very Busy:   0->1->2->3->0->1->0->1->... (may use till 3)
			USART3_Get_Free_Buf();// Get first free buf id for next use
		}

		TIM_ClearITPendingBit(TIM7,TIM_IT_Update);
		TIM_Cmd(TIM7,DISABLE);
	}	    
	OSIntExit();  											 
} 
void TIM7_Int_Init(u16 arr,u16 psc)
{	 
	TIM_TimeBaseInitTypeDef TIM_TimeBaseInitStructure;
	NVIC_InitTypeDef NVIC_InitStructure;
	
	RCC_APB1PeriphClockCmd(RCC_APB1Periph_TIM7,ENABLE);
	
  	TIM_TimeBaseInitStructure.TIM_Period = arr;
	TIM_TimeBaseInitStructure.TIM_Prescaler=psc;
	TIM_TimeBaseInitStructure.TIM_CounterMode=TIM_CounterMode_Up;
	TIM_TimeBaseInitStructure.TIM_ClockDivision=TIM_CKD_DIV1; 
	
	TIM_TimeBaseInit(TIM7,&TIM_TimeBaseInitStructure);
	
	TIM_ITConfig(TIM7,TIM_IT_Update,ENABLE);
	TIM_Cmd(TIM7,ENABLE);
	
	NVIC_InitStructure.NVIC_IRQChannel=TIM7_IRQn;  
	NVIC_InitStructure.NVIC_IRQChannelPreemptionPriority=0x00;
	NVIC_InitStructure.NVIC_IRQChannelSubPriority=0x01;
	NVIC_InitStructure.NVIC_IRQChannelCmd=ENABLE;
	NVIC_Init(&NVIC_InitStructure); 							 
} 
void TIM12_CH2_PWM_Init(u16 arr,u16 psc)
{		 					 
 
	GPIO_InitTypeDef GPIO_InitStructure;
	TIM_TimeBaseInitTypeDef  TIM_TimeBaseStructure;
	TIM_OCInitTypeDef  TIM_OCInitStructure;
	
	
	RCC_APB1PeriphClockCmd(RCC_APB1Periph_TIM12,ENABLE);
	RCC_AHB1PeriphClockCmd(RCC_AHB1Periph_GPIOB, ENABLE);
	
	GPIO_InitStructure.GPIO_Pin = GPIO_Pin_15;
	GPIO_InitStructure.GPIO_Mode = GPIO_Mode_AF;
	GPIO_InitStructure.GPIO_Speed = GPIO_Speed_100MHz;
	GPIO_InitStructure.GPIO_OType = GPIO_OType_PP;
	GPIO_InitStructure.GPIO_PuPd = GPIO_PuPd_UP;
	GPIO_Init(GPIOB,&GPIO_InitStructure);
 
	GPIO_PinAFConfig(GPIOB,GPIO_PinSource15,GPIO_AF_TIM12);

	TIM_TimeBaseStructure.TIM_Prescaler=psc;
	TIM_TimeBaseStructure.TIM_CounterMode=TIM_CounterMode_Up;
	TIM_TimeBaseStructure.TIM_Period=arr;
	TIM_TimeBaseStructure.TIM_ClockDivision=TIM_CKD_DIV1; 
	TIM_TimeBaseInit(TIM12,&TIM_TimeBaseStructure);

	TIM_OCInitStructure.TIM_OCMode = TIM_OCMode_PWM1;
 	TIM_OCInitStructure.TIM_OutputState = TIM_OutputState_Enable;
	TIM_OCInitStructure.TIM_OCPolarity = TIM_OCPolarity_High;
	TIM_OCInitStructure.TIM_Pulse=0;
	TIM_OC2Init(TIM12, &TIM_OCInitStructure);

	TIM_OC2PreloadConfig(TIM12, TIM_OCPreload_Enable);
 
  TIM_ARRPreloadConfig(TIM12,ENABLE);
	
	TIM_Cmd(TIM12, ENABLE);
 
}
