#include "sys.h"
#include "usart.h"	 
#include "dma.h"
#include "common.h"
#include "control.h"
#include "timer.h"
////////////////////////////////////////////////////////////////////////////////// 	 
//如果使用os,则包括下面的头文件即可.
#if SYSTEM_SUPPORT_OS
#include "includes.h"					//os 使用	  
#endif
//////////////////////////////////////////////////////////////////////////////////	 
//本程序只供学习使用，未经作者许可，不得用于其它任何用途
//ALIENTEK STM32开发板
//串口1初始化		   
//正点原子@ALIENTEK
//技术论坛:www.openedv.com
//修改日期:2012/8/18
//版本：V1.5
//版权所有，盗版必究。
//Copyright(C) 广州市星翼电子科技有限公司 2009-2019
//All rights reserved
//********************************************************************************
//V1.3修改说明 
//支持适应不同频率下的串口波特率设置.
//加入了对printf的支持
//增加了串口接收命令功能.
//修正了printf第一个字符丢失的bug
//V1.4修改说明
//1,修改串口初始化IO的bug
//2,修改了USART_RX_STA,使得串口最大接收字节数为2的14次方
//3,增加了USART_MAX_RECV_LEN,用于定义串口最大允许接收的字节数(不大于2的14次方)
//4,修改了EN_USART1_RX的使能方式
//V1.5修改说明
//1,增加了对UCOSII的支持
////////////////////////////////////////////////////////////////////////////////// 	  
 

//////////////////////////////////////////////////////////////////
//加入以下代码,支持printf函数,而不需要选择use MicroLIB	  
#if 1
#pragma import(__use_no_semihosting)             
//标准库需要的支持函数                 
struct __FILE 
{ 
	int handle; 

}; 

FILE __stdout;       
//定义_sys_exit()以避免使用半主机模式    
_sys_exit(int x) 
{ 
	x = x; 
} 
//重定义fputc函数 
int fputc(int ch, FILE *f)
{      
	while((USART1->SR&0X40)==0);//循环发送,直到发送完毕   
    USART1->DR = (u8) ch;      
	return ch;
}
#endif 

/*使用microLib的方法*/
 /* 
int fputc(int ch, FILE *f)
{
	USART_SendData(USART1, (uint8_t) ch);

	while (USART_GetFlagStatus(USART1, USART_FLAG_TC) == RESET) {}	
   
    return ch;
}
int GetKey (void)  { 

    while (!(USART1->SR & USART_FLAG_RXNE));

    return ((int)(USART1->DR & 0x1FF));
}
*/
 
#if EN_USART1_RX   //如果使能了接收
//串口1中断服务程序
//注意,读取USARTx->SR能避免莫名其妙的错误   	
u8 USART_RX_BUF[USART_MAX_RECV_LEN_TEMP];     //接收缓冲,最大USART_MAX_RECV_LEN个字节.
//接收状态
//bit15，	接收完成标志
//bit14，	接收到0x0d
//bit13~0，	接收到的有效字节数目
u16 USART_RX_STA=0;       //接收状态标记	  

extern u8 g_ota_runing;
extern u16 g_ota_recv_sum = 0;
extern u16 g_ota_one_pg_recv_len;
extern u16 g_ota_pg_numid;

//初始化IO 串口1 
//bound:波特率
void uart_init(u32 bound){
    //GPIO端口设置
    GPIO_InitTypeDef GPIO_InitStructure;
	USART_InitTypeDef USART_InitStructure;
	NVIC_InitTypeDef NVIC_InitStructure;
	 
	RCC_APB2PeriphClockCmd(RCC_APB2Periph_USART1|RCC_APB2Periph_GPIOA, ENABLE);	//使能USART1，GPIOA时钟
 	USART_DeInit(USART1);  //复位串口1
	//USART1_TX   PA.9
    GPIO_InitStructure.GPIO_Pin = GPIO_Pin_9; //PA.9
    GPIO_InitStructure.GPIO_Speed = GPIO_Speed_50MHz;
    GPIO_InitStructure.GPIO_Mode = GPIO_Mode_AF_PP;	//复用推挽输出
    GPIO_Init(GPIOA, &GPIO_InitStructure); //初始化PA9
   
    //USART1_RX	  PA.10
    GPIO_InitStructure.GPIO_Pin = GPIO_Pin_10;
    GPIO_InitStructure.GPIO_Mode = GPIO_Mode_IN_FLOATING;//浮空输入
    GPIO_Init(GPIOA, &GPIO_InitStructure);  //初始化PA10

   //Usart1 NVIC 配置

    NVIC_InitStructure.NVIC_IRQChannel = USART1_IRQn;
	NVIC_InitStructure.NVIC_IRQChannelPreemptionPriority=3 ;//抢占优先级3
	NVIC_InitStructure.NVIC_IRQChannelSubPriority = 3;		//子优先级3
	NVIC_InitStructure.NVIC_IRQChannelCmd = ENABLE;			//IRQ通道使能
	NVIC_Init(&NVIC_InitStructure);	//根据指定的参数初始化VIC寄存器
  
   //USART 初始化设置
	USART_InitStructure.USART_BaudRate = bound;//一般设置为9600;
	USART_InitStructure.USART_WordLength = USART_WordLength_8b;//字长为8位数据格式
	USART_InitStructure.USART_StopBits = USART_StopBits_1;//一个停止位
	USART_InitStructure.USART_Parity = USART_Parity_No;//无奇偶校验位
	USART_InitStructure.USART_HardwareFlowControl = USART_HardwareFlowControl_None;//无硬件数据流控制
	USART_InitStructure.USART_Mode = USART_Mode_Rx | USART_Mode_Tx;	//收发模式

    USART_Init(USART1, &USART_InitStructure); //初始化串口
    //USART_ITConfig(USART1, USART_IT_RXNE, ENABLE);//开启中断
		USART_ITConfig(USART1, USART_IT_IDLE, ENABLE);//开启空闲中断
    USART_Cmd(USART1, ENABLE);                    //使能串口 
		
		while(USART_GetFlagStatus(USART1,USART_FLAG_TC) == RESET);
		
		// 100ms
		TIM6_Int_Init(999, 7199);
		
		TIM_Cmd(TIM6, DISABLE); //关闭定时器7
		
		USART_RX_STA=0;
}
extern int g_temp_center;
extern TEMP_VAL g_temp_val_new;
extern EVENT_VAL g_event_val_new;
extern int g_direct_shutwown;
extern int g_direct_startup;
extern int g_direct_feed;

extern int g_feed_mode;
extern int g_shutdown_mode;
extern int g_startup_mode;
extern int g_run_mode;

extern int g_temp1_error;
extern int g_temp2_error;
extern int g_temp3_error;
extern int g_temp4_error;
extern int g_temp5_error;

extern int g_fatal_error;
int run_cmd_from_usart(u8 *data, u16 num, u8 *out, u16 outnum)
{
	int ret=0;
	if(data[0] != 0xfe)
		return ret;//非法cmd
	switch(data[1])
	{
		case 0x01://设定温度
			g_temp_val_new.target_val = data[3]*100 + data[4]*10 +data[5];
			g_temp_val_new.target_update = 1;
			memcpy(out,data,num);
			ret = num;
			break;
		case 0x02://设定smoke
			g_temp_val_new.target_smoke = data[2];
			g_temp_val_new.target_update = 1;
			memcpy(out,data,num);
			ret = num;
			break;
		case 0x03://设定
			switch(data[2])
			{
				case 0x00://shutdown
					g_direct_shutwown=1;
					break;
				case 0x01://startup
					g_direct_startup=1;
					break;
				case 0x02://feed
					g_direct_feed=1;
					break;
			}
			memcpy(out,data,num);
			ret = num;
			break;
		case 0x0b://获取状态
			memset(out,0x00,outnum);
			out[0]=0xFE;
			out[1]=0x0B;
			out[74]=0xFF;
		
			out[2]=g_temp_val_new.temp5/100;
			out[3]=(g_temp_val_new.temp5%100)/10;
			out[4]=g_temp_val_new.temp5%10;
		
			out[5]=g_temp_val_new.target_val/100;
			out[6]=(g_temp_val_new.target_val%100)/10;
			out[7]=g_temp_val_new.target_val%10;
		
			out[8]=0;
			out[9]=0;
			out[10]=0;
			out[11]=0;
			out[12]=0;
			out[13]=0;
		
			out[14]=g_temp_val_new.temp1/100;
			out[15]=(g_temp_val_new.temp1%100)/10;
			out[16]=g_temp_val_new.temp1%10;

			out[17]=g_temp_val_new.temp2/100;
			out[18]=(g_temp_val_new.temp2%100)/10;
			out[19]=g_temp_val_new.temp2%10;
			
			out[20]=g_temp_val_new.temp3/100;
			out[21]=(g_temp_val_new.temp3%100)/10;
			out[22]=g_temp_val_new.temp3%10;
			
			out[23]=g_temp_val_new.temp4/100;
			out[24]=(g_temp_val_new.temp4%100)/10;
			out[25]=g_temp_val_new.temp4%10;
			
			out[26]=0;
			
			out[27]=g_temp_val_new.target_smoke;
			
			out[28]=g_temp_val_new.temp_unit==0? 1:2;//1 F 2 C
			
			out[29]=g_temp1_error==0? 1:2;
			
			out[30]=g_fatal_error==1? 1:2;//over temp
			
			out[31]=0;
			out[32]=0;
			out[33]=0;
			
			out[34]=g_temp1_error==0? 1:0;
			out[35]=g_temp2_error==0? 1:0;
			out[36]=g_temp3_error==0? 1:0;
			out[37]=g_temp4_error==0? 1:0;
			
			out[38]=MOT_I == Control_ON? 1:0;//mod
			out[39]=HOT_I == Control_ON? 1:0;//hot
			out[40]=FAN_I == Control_ON? 1:0;//fun
			
			ret = 75;
			break;
		case 0x06://出厂设置
			g_feed_mode = 0;
			g_shutdown_mode = 0;
			g_startup_mode = 0;
			g_run_mode = 0;
			
			g_temp_val_new.temp_unit = 0;
			memcpy(out,data,num);
			ret = num;
			break;
		case 0xFF:// OTA
			switch(data[2])
			{
				case 0x01:// OTA Notify
					// FE FF 01 FF
					// Clost FAN / MOT / HOT
					g_ota_runing = 1;
					memcpy(out, data, num);
					ret = num;
					break;
				case 0x02:// Total Packages & MD5
					// FE FF 02 + 3B-Size + 1B-TotalPackagesNum + MD5 + FF
					// Clost FAN / MOT / HOT
					memcpy(out, data, num);
					ret = num;
					break;
				case 0x03:// Every Package & MD5
					// FE FF 03 + 1B-PackageNum + MD5 + FF
					g_ota_pg_numid = data[3];
					memcpy(out, data, num);
					ret = num;
					break;
				default:
					break;
			}
			break;
		default:
			break;
	}
	return ret;
}

void u1_printf(char* fmt,...)  
{  
	u16 i,j;
	va_list ap;
	va_start(ap,fmt);
	vsprintf((char*)UART1_TX_BUF,fmt,ap);
	va_end(ap);
	i=strlen((const char*)UART1_TX_BUF);//此次发送数据的长度
	for(j=0;j<i;j++)//循环发送数据
	{
		while(USART_GetFlagStatus(UART1,USART_FLAG_TC)==RESET);//循环发送,直到发送完毕   
		USART_SendData(UART5,(uint8_t)UART1_TX_BUF[j]);   
	}
}

void USART1_IRQHandler(void)                	//串口1中断服务程序
{
#ifdef SYSTEM_SUPPORT_OS	 	
	OSIntEnter();    
#endif

		 if (USART_GetITStatus(USART1, USART_IT_IDLE) != RESET)  
	    {
				
				TIM_SetCounter(TIM6,0);
				if(USART_RX_STA==0) 				//使能定时器7的中断 
				{
					TIM_Cmd(TIM6, ENABLE); 	    			//使能定时器7
				}

	            USART_ClearITPendingBit(USART1, USART_IT_IDLE);         //清除中断标志
	            USART_ReceiveData(USART1);//读取数据 注意：这句必须要，否则不能够清除中断标志位。
	    }

#ifdef SYSTEM_SUPPORT_OS	 
	OSIntExit();  											 
#endif
} 
#endif	

