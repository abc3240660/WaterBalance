#include "sys.h"
#include "usart3.h"	  
#include "stdarg.h"	 	 
#include "stdio.h"	 	 
#include "string.h"
#include "timer.h"
#include "ucos_ii.h"
#include "malloc.h"

__align(8) u8 USART3_TX_BUF[USART3_MAX_SEND_LEN];

// TBD: 2048B is too large for static array
//u8 USART3_RX_BUF[USART3_MAX_RECV_LEN];
u8* USART3_RX_BUF = NULL;
u8 U3_RX_ID = 0;// 0~3
vu16 USART3_RX_STA[4] = {0};

// Get free buf id for next use
// >>> U3_RX_ID <<<
// Not Busy:    0->1->0->1->0->1->... (always switch between 0 and 1)
// little Busy: 0->1->2->0->1->0->1->... (may use till 2)
// very Busy:   0->1->2->3->0->1->0->1->... (may use till 3)
void USART3_Get_Free_Buf(void)
{
	u8 i = 0;
	//U3_RX_ID = 0;
	for (i=0; i<U3_RECV_BUF_CNT; i++) {
		if ((USART3_RX_STA[i]&(1<<15)) == 0) {// first free Buf
			U3_RX_ID = i;
			printf("switch to buf id = %d\n", i);
			break;
		}
	}
}

void USART3_IRQHandler(void)
{
	u8 res = 0;

	OSIntEnter();

	if (USART_GetFlagStatus(USART3, USART_FLAG_RXNE) != RESET) {
		res = USART_ReceiveData(USART3);
		if ((USART3_RX_STA[U3_RX_ID]&(1<<15)) == 0) {// frame not complete
			if (USART3_RX_STA[U3_RX_ID] < USART3_MAX_RECV_LEN) {
				TIM_SetCounter(TIM7, 0);
				if (USART3_RX_STA[U3_RX_ID] == 0) {
					TIM_Cmd(TIM7, ENABLE);
				}
				USART3_RX_BUF[U3_RECV_LEN_ONE*U3_RX_ID + USART3_RX_STA[U3_RX_ID]++] = res;
			} else {
				USART3_RX_STA[U3_RX_ID] |= 1<<15;
			}
		}
	}

	OSIntExit();
}   

void usart3_init(u32 bound)
{  	
	NVIC_InitTypeDef NVIC_InitStructure;
	GPIO_InitTypeDef GPIO_InitStructure;
	USART_InitTypeDef USART_InitStructure;
 
	USART_DeInit(USART3);  //��λ����3
	
	RCC_AHB1PeriphClockCmd(RCC_AHB1Periph_GPIOB,ENABLE); //ʹ��GPIOBʱ��
	RCC_APB1PeriphClockCmd(RCC_APB1Periph_USART3,ENABLE);//ʹ��USART3ʱ��
	
 
	 GPIO_InitStructure.GPIO_Pin = GPIO_Pin_11 | GPIO_Pin_10; //GPIOB11��GPIOB10��ʼ��
	GPIO_InitStructure.GPIO_Mode = GPIO_Mode_AF;//���ù���
	GPIO_InitStructure.GPIO_Speed = GPIO_Speed_50MHz;	//�ٶ�50MHz
	GPIO_InitStructure.GPIO_OType = GPIO_OType_PP; //���츴�����
	GPIO_InitStructure.GPIO_PuPd = GPIO_PuPd_UP; //����
	GPIO_Init(GPIOB,&GPIO_InitStructure); //��ʼ��GPIOB11����GPIOB10
	
	
	GPIO_PinAFConfig(GPIOB,GPIO_PinSource11,GPIO_AF_USART3); //GPIOB11����ΪUSART3
	GPIO_PinAFConfig(GPIOB,GPIO_PinSource10,GPIO_AF_USART3); //GPIOB10����ΪUSART3	  
	
	USART_InitStructure.USART_BaudRate = bound;//������һ������Ϊ9600;
	USART_InitStructure.USART_WordLength = USART_WordLength_8b;//�ֳ�Ϊ8λ���ݸ�ʽ
	USART_InitStructure.USART_StopBits = USART_StopBits_1;//һ��ֹͣλ
	USART_InitStructure.USART_Parity = USART_Parity_No;//����żУ��λ
	USART_InitStructure.USART_HardwareFlowControl = USART_HardwareFlowControl_None;//��Ӳ������������
	USART_InitStructure.USART_Mode = USART_Mode_Rx | USART_Mode_Tx;	//�շ�ģʽ
	USART_Init(USART3, &USART_InitStructure); //��ʼ������3
	
	USART_ITConfig(USART3, USART_IT_RXNE, ENABLE);//�����ж�  
		
	USART_Cmd(USART3, ENABLE);                    //ʹ�ܴ��� 
	
 
	NVIC_InitStructure.NVIC_IRQChannel = USART3_IRQn;
	NVIC_InitStructure.NVIC_IRQChannelPreemptionPriority=2 ;//��ռ���ȼ�2
	NVIC_InitStructure.NVIC_IRQChannelSubPriority = 3;		//�����ȼ�3
	NVIC_InitStructure.NVIC_IRQChannelCmd = ENABLE;			//IRQͨ��ʹ��
	NVIC_Init(&NVIC_InitStructure);	//����ָ���Ĳ�����ʼ��VIC�Ĵ���
	
	TIM7_Int_Init(1000-1,8400-1);	//10ms�ж�һ��
	
  TIM_Cmd(TIM7, DISABLE); //�رն�ʱ��7
	
	USART3_RX_BUF = mymalloc(SRAMIN, USART3_MAX_RECV_LEN);
	
	USART3_RX_STA[0] = 0;
	USART3_RX_STA[1] = 0;
	USART3_RX_STA[2] = 0;
	USART3_RX_STA[3] = 0;
}

//����3,printf ����
//ȷ��һ�η������ݲ�����USART3_MAX_SEND_LEN�ֽ�
void u3_printf(char* fmt,...)  
{  
	u16 i,j;
	va_list ap;
	va_start(ap,fmt);
	vsprintf((char*)USART3_TX_BUF,fmt,ap);
	va_end(ap);
	i=strlen((const char*)USART3_TX_BUF);//�˴η������ݵĳ���
	for(j=0;j<i;j++)//ѭ����������
	{
		while(USART_GetFlagStatus(USART3,USART_FLAG_TC)==RESET);//ѭ������,ֱ���������   
		USART_SendData(USART3,(uint8_t)USART3_TX_BUF[j]);   
	}
}



































