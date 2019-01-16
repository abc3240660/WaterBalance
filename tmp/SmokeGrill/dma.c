#include "sys.h"
#include "dma.h"
#include "usart.h"
#include "string.h"

DMA_InitTypeDef DMA_InitStructure;

u8 U1_DMA_R_BUF[U1_DMA_R_LEN];
//u8 U1_R_BUF[U1_DMA_R_LEN];
u8 U1_DMA_T_BUF[U1_DMA_T_LEN];
u8 U1_DMA_SEND_FREE_FLAG = FREE;

u16 DMA1_MEM_LEN;//保存DMA每次数据传送的长度 	 

extern u16 g_ota_pg_numid;

//串口1的DMA设置
void Usart_DMA_Init(void)
{ 
 	NVIC_InitTypeDef NVIC_InitStructure;

 	RCC_AHBPeriphClockCmd(RCC_AHBPeriph_DMA1, ENABLE);	//使能DMA传输

//相应的DMA配置,接收DMA
	DMA_DeInit(DMA1_Channel5);   //将DMA的通道5寄存器重设为缺省值  串口1对应的是DMA通道5
	DMA_InitStructure.DMA_PeripheralBaseAddr = (u32)&USART1->DR;  //DMA外设ADC基地址
	DMA_InitStructure.DMA_MemoryBaseAddr = (u32)U1_DMA_R_BUF;  //DMA内存基地址
	DMA_InitStructure.DMA_DIR = DMA_DIR_PeripheralSRC;  //数据传输方向，从外设读取发送到内存
	DMA_InitStructure.DMA_BufferSize = U1_DMA_R_LEN;  //DMA通道的DMA缓存的大小
	DMA_InitStructure.DMA_PeripheralInc = DMA_PeripheralInc_Disable;  //外设地址寄存器不变
	DMA_InitStructure.DMA_MemoryInc = DMA_MemoryInc_Enable;  //内存地址寄存器递增
	DMA_InitStructure.DMA_PeripheralDataSize = DMA_PeripheralDataSize_Byte;  //数据宽度为8位
	DMA_InitStructure.DMA_MemoryDataSize = DMA_MemoryDataSize_Byte; //数据宽度为8位
	DMA_InitStructure.DMA_Mode = DMA_Mode_Normal;  //工作在正常缓存模式
	DMA_InitStructure.DMA_Priority = DMA_Priority_Medium; //DMA通道 x拥有中优先级 
	DMA_InitStructure.DMA_M2M = DMA_M2M_Disable;  //DMA通道x没有设置为内存到内存传输
	DMA_Init(DMA1_Channel5, &DMA_InitStructure);  //根据DMA_InitStruct中指定的参数初始化DMA的通道USART1_Tx_DMA_Channel所标识的寄存器
	
	DMA_ITConfig(DMA1_Channel5,DMA_IT_TC,ENABLE);
	USART_DMACmd(USART1, USART_DMAReq_Rx, ENABLE);   //使能串口1 DMA接收
	DMA_Cmd(DMA1_Channel5, ENABLE);  //正式驱动DMA传输

	NVIC_InitStructure.NVIC_IRQChannel = DMA1_Channel5_IRQn;
	NVIC_InitStructure.NVIC_IRQChannelPreemptionPriority = 3;//抢占优先级3
	NVIC_InitStructure.NVIC_IRQChannelSubPriority = 3;		//子优先级3
	NVIC_InitStructure.NVIC_IRQChannelCmd = ENABLE;			//IRQ通道使能
	NVIC_Init(&NVIC_InitStructure);	//根据指定的参数初始化VIC寄存器}	
	
	//相应的DMA配置,发送DMA
	DMA_DeInit(DMA1_Channel4);   //将DMA的通道4寄存器重设为缺省值  串口1对应的是DMA通道4
	DMA_InitStructure.DMA_PeripheralBaseAddr = (u32)&USART1->DR;  //DMA外设ADC基地址
	DMA_InitStructure.DMA_MemoryBaseAddr = (u32)U1_DMA_T_BUF;  //DMA内存基地址
	DMA_InitStructure.DMA_DIR = DMA_DIR_PeripheralDST;  //数据传输方向，从外设读取发送到内存
	DMA_InitStructure.DMA_BufferSize = U1_DMA_T_LEN;  //DMA通道的DMA缓存的大小
	DMA_InitStructure.DMA_PeripheralInc = DMA_PeripheralInc_Disable;  //外设地址寄存器不变
	DMA_InitStructure.DMA_MemoryInc = DMA_MemoryInc_Enable;  //内存地址寄存器递增
	DMA_InitStructure.DMA_PeripheralDataSize = DMA_PeripheralDataSize_Byte;  //数据宽度为8位
	DMA_InitStructure.DMA_MemoryDataSize = DMA_MemoryDataSize_Byte; //数据宽度为8位
	DMA_InitStructure.DMA_Mode = DMA_Mode_Normal;  //工作在正常缓存模式
	DMA_InitStructure.DMA_Priority = DMA_Priority_Medium; //DMA通道 x拥有中优先级 
	DMA_InitStructure.DMA_M2M = DMA_M2M_Disable;  //DMA通道x没有设置为内存到内存传输
	DMA_Init(DMA1_Channel4, &DMA_InitStructure);  //根据DMA_InitStruct中指定的参数初始化DMA的通道USART1_Tx_DMA_Channel所标识的寄存器

	DMA_ITConfig(DMA1_Channel4, DMA_IT_TC, ENABLE);	//开启发送中断
	USART_DMACmd(USART1, USART_DMAReq_Tx, ENABLE);   //使能串口1 DMA发送

	NVIC_InitStructure.NVIC_IRQChannel = DMA1_Channel4_IRQn;
	NVIC_InitStructure.NVIC_IRQChannelPreemptionPriority = 3;//抢占优先级3
	NVIC_InitStructure.NVIC_IRQChannelSubPriority = 3;		//子优先级3
	NVIC_InitStructure.NVIC_IRQChannelCmd = ENABLE;			//IRQ通道使能
	NVIC_Init(&NVIC_InitStructure);	//根据指定的参数初始化VIC寄存器}	  
 
}
void DMA1_Channel5_IRQHandler(void)
{
#ifdef OS_TICKS_PER_SEC	 	//如果时钟节拍数定义了,说明要使用ucosII了.
	OSIntEnter();    
#endif

    if (DMA_GetFlagStatus(DMA1_FLAG_TC5) != RESET)//等待通道4传输完成
    {
			DMA_Cmd (DMA1_Channel5,DISABLE);
			DMA_ClearFlag(DMA1_FLAG_TC5);
			DMA_SetCurrDataCounter(DMA1_Channel5,512);
			DMA_Cmd (DMA1_Channel5,ENABLE);
			
			if((USART_RX_STA&(1<<19))==0)
			{ 
				if(USART_RX_STA<USART_MAX_RECV_LEN)
				{
					if (U1_DMA_R_LEN < (USART_MAX_RECV_LEN - USART_RX_STA)) {
						memcpy(USART_RX_BUF+(USART_RX_STA&0xFFFF), &U1_DMA_R_BUF[0], U1_DMA_R_LEN);
						USART_RX_STA += U1_DMA_R_LEN;
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
			
			//printf("USART_RX_STA = %x\n", USART_RX_STA);
			//g_ota_pg_numid++;
		}

#ifdef OS_TICKS_PER_SEC	 	//如果时钟节拍数定义了,说明要使用ucosII了.
	OSIntExit();  											 
#endif
}

//DMA发送中断程序
void DMA1_Channel4_IRQHandler(void)
{
	#ifdef OS_TICKS_PER_SEC	 	//如果时钟节拍数定义了,说明要使用ucosII了.
		OSIntEnter();    
	#endif

    if (DMA_GetFlagStatus(DMA1_FLAG_TC4) != RESET)//等待通道4传输完成
    {
			DMA_ClearFlag(DMA1_FLAG_TC4);//清除通道4传输完成标志
			U1_DMA_SEND_FREE_FLAG = FREE;
    }

	#ifdef OS_TICKS_PER_SEC	 	//如果时钟节拍数定义了,说明要使用ucosII了.
		OSIntExit();  											 
	#endif
}
//========================================================================
// 函数:  void set_buf_free(u8 *buf,u8 len)   
// 描述: 清空数组
// 参数: 
// 返回: 无
//========================================================================
void Set_Buf_Free(u8 *buf,u8 len) 
{
    u8 i;
	for(i=0;i<len;i++)buf[i]=0;
}

//========================================================================
// 函数:  void Uart1_DMA_Send_Array(u8 *buffer, u16 len)   
// 描述: 串口1发送数组
// 参数: buffer数组，len数组长度
// 返回: 无
//========================================================================
void Uart1_DMA_Send_Array(u8 *buffer, u16 len)
{
	while (U1_DMA_SEND_FREE_FLAG == USEING);//是否需要锁死
	memcpy(U1_DMA_T_BUF, buffer, len);
	DMA_Cmd(DMA1_Channel4, DISABLE);  //关闭USART1 TX DMA1 所指示的通道      
	DMA_SetCurrDataCounter(DMA1_Channel4, len);//DMA通道的DMA缓存的大小
	DMA_Cmd(DMA1_Channel4, ENABLE);  //使能USART1 TX DMA1 所指示的通道 
	U1_DMA_SEND_FREE_FLAG = USEING;
}

void Send_Model_Message(void)
{
  u8 tab[]={0xfe,0x26,0x01,0x00,0x02,0xff};//发送APP型号   102 wf3
  Uart1_DMA_Send_Array(tab,6);
}

void Recv_Data_Handle(u8 *data,u16 num)
{
	u16 t = 0;
	u16 len = num;
	
	for(t=0;t<len;t++)
	{
		USART_SendData(USART1, data[t]);//向串口1发送数据
		while(USART_GetFlagStatus(USART1,USART_FLAG_TC)!=SET);//等待发送结束
	}
}
