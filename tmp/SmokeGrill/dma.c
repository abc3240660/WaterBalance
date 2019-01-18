#include "sys.h"
#include "dma.h"
#include "usart.h"
#include "string.h"

DMA_InitTypeDef DMA_InitStructure;

u8 U1_DMA_R_BUF[U1_DMA_R_LEN];
//u8 U1_R_BUF[U1_DMA_R_LEN];
u8 U1_DMA_T_BUF[U1_DMA_T_LEN];
u8 U1_DMA_SEND_FREE_FLAG = FREE;

u16 DMA1_MEM_LEN;

void Usart_DMA_Init(void)
{ 
 	NVIC_InitTypeDef NVIC_InitStructure;

 	RCC_AHBPeriphClockCmd(RCC_AHBPeriph_DMA1, ENABLE);

	DMA_DeInit(DMA1_Channel5);
	DMA_InitStructure.DMA_PeripheralBaseAddr = (u32)&USART1->DR;
	DMA_InitStructure.DMA_MemoryBaseAddr = (u32)U1_DMA_R_BUF;
	DMA_InitStructure.DMA_DIR = DMA_DIR_PeripheralSRC;
	DMA_InitStructure.DMA_BufferSize = U1_DMA_R_LEN;
	DMA_InitStructure.DMA_PeripheralInc = DMA_PeripheralInc_Disable;
	DMA_InitStructure.DMA_MemoryInc = DMA_MemoryInc_Enable;
	DMA_InitStructure.DMA_PeripheralDataSize = DMA_PeripheralDataSize_Byte;
	DMA_InitStructure.DMA_MemoryDataSize = DMA_MemoryDataSize_Byte;
	DMA_InitStructure.DMA_Mode = DMA_Mode_Normal;
	DMA_InitStructure.DMA_Priority = DMA_Priority_Medium;
	DMA_InitStructure.DMA_M2M = DMA_M2M_Disable;
	DMA_Init(DMA1_Channel5, &DMA_InitStructure);
	
	DMA_ITConfig(DMA1_Channel5,DMA_IT_TC,ENABLE);
	USART_DMACmd(USART1, USART_DMAReq_Rx, ENABLE);
	DMA_Cmd(DMA1_Channel5, ENABLE);

	NVIC_InitStructure.NVIC_IRQChannel = DMA1_Channel5_IRQn;
	NVIC_InitStructure.NVIC_IRQChannelPreemptionPriority = 3;
	NVIC_InitStructure.NVIC_IRQChannelSubPriority = 3;
	NVIC_InitStructure.NVIC_IRQChannelCmd = ENABLE;
	NVIC_Init(&NVIC_InitStructure);
	
	DMA_DeInit(DMA1_Channel4);
	DMA_InitStructure.DMA_PeripheralBaseAddr = (u32)&USART1->DR;
	DMA_InitStructure.DMA_MemoryBaseAddr = (u32)U1_DMA_T_BUF;
	DMA_InitStructure.DMA_DIR = DMA_DIR_PeripheralDST;
	DMA_InitStructure.DMA_BufferSize = U1_DMA_T_LEN;
	DMA_InitStructure.DMA_PeripheralInc = DMA_PeripheralInc_Disable;
	DMA_InitStructure.DMA_MemoryInc = DMA_MemoryInc_Enable;
	DMA_InitStructure.DMA_PeripheralDataSize = DMA_PeripheralDataSize_Byte;
	DMA_InitStructure.DMA_MemoryDataSize = DMA_MemoryDataSize_Byte;
	DMA_InitStructure.DMA_Mode = DMA_Mode_Normal;
	DMA_InitStructure.DMA_Priority = DMA_Priority_Medium;
	DMA_InitStructure.DMA_M2M = DMA_M2M_Disable;
	DMA_Init(DMA1_Channel4, &DMA_InitStructure);

	DMA_ITConfig(DMA1_Channel4, DMA_IT_TC, ENABLE);
	USART_DMACmd(USART1, USART_DMAReq_Tx, ENABLE);

	NVIC_InitStructure.NVIC_IRQChannel = DMA1_Channel4_IRQn;
	NVIC_InitStructure.NVIC_IRQChannelPreemptionPriority = 3;
	NVIC_InitStructure.NVIC_IRQChannelSubPriority = 3;
	NVIC_InitStructure.NVIC_IRQChannelCmd = ENABLE;
	NVIC_Init(&NVIC_InitStructure);
 
}
void DMA1_Channel5_IRQHandler(void)
{
#ifdef OS_TICKS_PER_SEC
	OSIntEnter();    
#endif

    if (DMA_GetFlagStatus(DMA1_FLAG_TC5) != RESET)
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
		}

#ifdef OS_TICKS_PER_SEC
	OSIntExit();  											 
#endif
}

void DMA1_Channel4_IRQHandler(void)
{
	#ifdef OS_TICKS_PER_SEC
		OSIntEnter();    
	#endif

    if (DMA_GetFlagStatus(DMA1_FLAG_TC4) != RESET)
    {
			DMA_ClearFlag(DMA1_FLAG_TC4);
			U1_DMA_SEND_FREE_FLAG = FREE;
    }

	#ifdef OS_TICKS_PER_SEC
		OSIntExit();  											 
	#endif
}
//========================================================================
void Set_Buf_Free(u8 *buf,u8 len) 
{
    u8 i;
	for(i=0;i<len;i++)buf[i]=0;
}

//========================================================================
void Uart1_DMA_Send_Array(u8 *buffer, u16 len)
{
	while (U1_DMA_SEND_FREE_FLAG == USEING);
	memcpy(U1_DMA_T_BUF, buffer, len);
	DMA_Cmd(DMA1_Channel4, DISABLE);
	DMA_SetCurrDataCounter(DMA1_Channel4, len);
	DMA_Cmd(DMA1_Channel4, ENABLE);
	U1_DMA_SEND_FREE_FLAG = USEING;
}

void Send_Model_Message(void)
{
  u8 tab[]={0xfe,0x26,0x01,0x00,0x02,0xff};
  Uart1_DMA_Send_Array(tab,6);
}

void Recv_Data_Handle(u8 *data,u16 num)
{
	u16 t = 0;
	u16 len = num;
	
	for(t=0;t<len;t++)
	{
		USART_SendData(USART1, data[t]);
		while(USART_GetFlagStatus(USART1,USART_FLAG_TC)!=SET);
	}
}
