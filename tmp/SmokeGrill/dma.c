#include "sys.h"
#include "dma.h"
#include "usart.h"
#include "string.h"

DMA_InitTypeDef DMA_InitStructure;

u8 U1_DMA_R_BUF[U1_DMA_R_LEN];
//u8 U1_R_BUF[U1_DMA_R_LEN];
u8 U1_DMA_T_BUF[U1_DMA_T_LEN];
u8 U1_DMA_SEND_FREE_FLAG = FREE;

u16 DMA1_MEM_LEN;//����DMAÿ�����ݴ��͵ĳ��� 	 

extern u16 g_ota_pg_numid;

//����1��DMA����
void Usart_DMA_Init(void)
{ 
 	NVIC_InitTypeDef NVIC_InitStructure;

 	RCC_AHBPeriphClockCmd(RCC_AHBPeriph_DMA1, ENABLE);	//ʹ��DMA����

//��Ӧ��DMA����,����DMA
	DMA_DeInit(DMA1_Channel5);   //��DMA��ͨ��5�Ĵ�������Ϊȱʡֵ  ����1��Ӧ����DMAͨ��5
	DMA_InitStructure.DMA_PeripheralBaseAddr = (u32)&USART1->DR;  //DMA����ADC����ַ
	DMA_InitStructure.DMA_MemoryBaseAddr = (u32)U1_DMA_R_BUF;  //DMA�ڴ����ַ
	DMA_InitStructure.DMA_DIR = DMA_DIR_PeripheralSRC;  //���ݴ��䷽�򣬴������ȡ���͵��ڴ�
	DMA_InitStructure.DMA_BufferSize = U1_DMA_R_LEN;  //DMAͨ����DMA����Ĵ�С
	DMA_InitStructure.DMA_PeripheralInc = DMA_PeripheralInc_Disable;  //�����ַ�Ĵ�������
	DMA_InitStructure.DMA_MemoryInc = DMA_MemoryInc_Enable;  //�ڴ��ַ�Ĵ�������
	DMA_InitStructure.DMA_PeripheralDataSize = DMA_PeripheralDataSize_Byte;  //���ݿ��Ϊ8λ
	DMA_InitStructure.DMA_MemoryDataSize = DMA_MemoryDataSize_Byte; //���ݿ��Ϊ8λ
	DMA_InitStructure.DMA_Mode = DMA_Mode_Normal;  //��������������ģʽ
	DMA_InitStructure.DMA_Priority = DMA_Priority_Medium; //DMAͨ�� xӵ�������ȼ� 
	DMA_InitStructure.DMA_M2M = DMA_M2M_Disable;  //DMAͨ��xû������Ϊ�ڴ浽�ڴ洫��
	DMA_Init(DMA1_Channel5, &DMA_InitStructure);  //����DMA_InitStruct��ָ���Ĳ�����ʼ��DMA��ͨ��USART1_Tx_DMA_Channel����ʶ�ļĴ���
	
	DMA_ITConfig(DMA1_Channel5,DMA_IT_TC,ENABLE);
	USART_DMACmd(USART1, USART_DMAReq_Rx, ENABLE);   //ʹ�ܴ���1 DMA����
	DMA_Cmd(DMA1_Channel5, ENABLE);  //��ʽ����DMA����

	NVIC_InitStructure.NVIC_IRQChannel = DMA1_Channel5_IRQn;
	NVIC_InitStructure.NVIC_IRQChannelPreemptionPriority = 3;//��ռ���ȼ�3
	NVIC_InitStructure.NVIC_IRQChannelSubPriority = 3;		//�����ȼ�3
	NVIC_InitStructure.NVIC_IRQChannelCmd = ENABLE;			//IRQͨ��ʹ��
	NVIC_Init(&NVIC_InitStructure);	//����ָ���Ĳ�����ʼ��VIC�Ĵ���}	
	
	//��Ӧ��DMA����,����DMA
	DMA_DeInit(DMA1_Channel4);   //��DMA��ͨ��4�Ĵ�������Ϊȱʡֵ  ����1��Ӧ����DMAͨ��4
	DMA_InitStructure.DMA_PeripheralBaseAddr = (u32)&USART1->DR;  //DMA����ADC����ַ
	DMA_InitStructure.DMA_MemoryBaseAddr = (u32)U1_DMA_T_BUF;  //DMA�ڴ����ַ
	DMA_InitStructure.DMA_DIR = DMA_DIR_PeripheralDST;  //���ݴ��䷽�򣬴������ȡ���͵��ڴ�
	DMA_InitStructure.DMA_BufferSize = U1_DMA_T_LEN;  //DMAͨ����DMA����Ĵ�С
	DMA_InitStructure.DMA_PeripheralInc = DMA_PeripheralInc_Disable;  //�����ַ�Ĵ�������
	DMA_InitStructure.DMA_MemoryInc = DMA_MemoryInc_Enable;  //�ڴ��ַ�Ĵ�������
	DMA_InitStructure.DMA_PeripheralDataSize = DMA_PeripheralDataSize_Byte;  //���ݿ��Ϊ8λ
	DMA_InitStructure.DMA_MemoryDataSize = DMA_MemoryDataSize_Byte; //���ݿ��Ϊ8λ
	DMA_InitStructure.DMA_Mode = DMA_Mode_Normal;  //��������������ģʽ
	DMA_InitStructure.DMA_Priority = DMA_Priority_Medium; //DMAͨ�� xӵ�������ȼ� 
	DMA_InitStructure.DMA_M2M = DMA_M2M_Disable;  //DMAͨ��xû������Ϊ�ڴ浽�ڴ洫��
	DMA_Init(DMA1_Channel4, &DMA_InitStructure);  //����DMA_InitStruct��ָ���Ĳ�����ʼ��DMA��ͨ��USART1_Tx_DMA_Channel����ʶ�ļĴ���

	DMA_ITConfig(DMA1_Channel4, DMA_IT_TC, ENABLE);	//���������ж�
	USART_DMACmd(USART1, USART_DMAReq_Tx, ENABLE);   //ʹ�ܴ���1 DMA����

	NVIC_InitStructure.NVIC_IRQChannel = DMA1_Channel4_IRQn;
	NVIC_InitStructure.NVIC_IRQChannelPreemptionPriority = 3;//��ռ���ȼ�3
	NVIC_InitStructure.NVIC_IRQChannelSubPriority = 3;		//�����ȼ�3
	NVIC_InitStructure.NVIC_IRQChannelCmd = ENABLE;			//IRQͨ��ʹ��
	NVIC_Init(&NVIC_InitStructure);	//����ָ���Ĳ�����ʼ��VIC�Ĵ���}	  
 
}
void DMA1_Channel5_IRQHandler(void)
{
#ifdef OS_TICKS_PER_SEC	 	//���ʱ�ӽ�����������,˵��Ҫʹ��ucosII��.
	OSIntEnter();    
#endif

    if (DMA_GetFlagStatus(DMA1_FLAG_TC5) != RESET)//�ȴ�ͨ��4�������
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

#ifdef OS_TICKS_PER_SEC	 	//���ʱ�ӽ�����������,˵��Ҫʹ��ucosII��.
	OSIntExit();  											 
#endif
}

//DMA�����жϳ���
void DMA1_Channel4_IRQHandler(void)
{
	#ifdef OS_TICKS_PER_SEC	 	//���ʱ�ӽ�����������,˵��Ҫʹ��ucosII��.
		OSIntEnter();    
	#endif

    if (DMA_GetFlagStatus(DMA1_FLAG_TC4) != RESET)//�ȴ�ͨ��4�������
    {
			DMA_ClearFlag(DMA1_FLAG_TC4);//���ͨ��4������ɱ�־
			U1_DMA_SEND_FREE_FLAG = FREE;
    }

	#ifdef OS_TICKS_PER_SEC	 	//���ʱ�ӽ�����������,˵��Ҫʹ��ucosII��.
		OSIntExit();  											 
	#endif
}
//========================================================================
// ����:  void set_buf_free(u8 *buf,u8 len)   
// ����: �������
// ����: 
// ����: ��
//========================================================================
void Set_Buf_Free(u8 *buf,u8 len) 
{
    u8 i;
	for(i=0;i<len;i++)buf[i]=0;
}

//========================================================================
// ����:  void Uart1_DMA_Send_Array(u8 *buffer, u16 len)   
// ����: ����1��������
// ����: buffer���飬len���鳤��
// ����: ��
//========================================================================
void Uart1_DMA_Send_Array(u8 *buffer, u16 len)
{
	while (U1_DMA_SEND_FREE_FLAG == USEING);//�Ƿ���Ҫ����
	memcpy(U1_DMA_T_BUF, buffer, len);
	DMA_Cmd(DMA1_Channel4, DISABLE);  //�ر�USART1 TX DMA1 ��ָʾ��ͨ��      
	DMA_SetCurrDataCounter(DMA1_Channel4, len);//DMAͨ����DMA����Ĵ�С
	DMA_Cmd(DMA1_Channel4, ENABLE);  //ʹ��USART1 TX DMA1 ��ָʾ��ͨ�� 
	U1_DMA_SEND_FREE_FLAG = USEING;
}

void Send_Model_Message(void)
{
  u8 tab[]={0xfe,0x26,0x01,0x00,0x02,0xff};//����APP�ͺ�   102 wf3
  Uart1_DMA_Send_Array(tab,6);
}

void Recv_Data_Handle(u8 *data,u16 num)
{
	u16 t = 0;
	u16 len = num;
	
	for(t=0;t<len;t++)
	{
		USART_SendData(USART1, data[t]);//�򴮿�1��������
		while(USART_GetFlagStatus(USART1,USART_FLAG_TC)!=SET);//�ȴ����ͽ���
	}
}
