#include "sys.h"
#include "usart.h"	 
#include "dma.h"
#include "common.h"
#include "control.h"
#include "timer.h"
////////////////////////////////////////////////////////////////////////////////// 	 
//���ʹ��os,����������ͷ�ļ�����.
#if SYSTEM_SUPPORT_OS
#include "includes.h"					//os ʹ��	  
#endif
//////////////////////////////////////////////////////////////////////////////////	 
//������ֻ��ѧϰʹ�ã�δ��������ɣ��������������κ���;
//ALIENTEK STM32������
//����1��ʼ��		   
//����ԭ��@ALIENTEK
//������̳:www.openedv.com
//�޸�����:2012/8/18
//�汾��V1.5
//��Ȩ���У�����ؾ���
//Copyright(C) ������������ӿƼ����޹�˾ 2009-2019
//All rights reserved
//********************************************************************************
//V1.3�޸�˵�� 
//֧����Ӧ��ͬƵ���µĴ��ڲ���������.
//�����˶�printf��֧��
//�����˴��ڽ��������.
//������printf��һ���ַ���ʧ��bug
//V1.4�޸�˵��
//1,�޸Ĵ��ڳ�ʼ��IO��bug
//2,�޸���USART_RX_STA,ʹ�ô����������ֽ���Ϊ2��14�η�
//3,������USART_MAX_RECV_LEN,���ڶ��崮�����������յ��ֽ���(������2��14�η�)
//4,�޸���EN_USART1_RX��ʹ�ܷ�ʽ
//V1.5�޸�˵��
//1,�����˶�UCOSII��֧��
////////////////////////////////////////////////////////////////////////////////// 	  
 

//////////////////////////////////////////////////////////////////
//�������´���,֧��printf����,������Ҫѡ��use MicroLIB	  
#if 1
#pragma import(__use_no_semihosting)             
//��׼����Ҫ��֧�ֺ���                 
struct __FILE 
{ 
	int handle; 

}; 

FILE __stdout;       
//����_sys_exit()�Ա���ʹ�ð�����ģʽ    
_sys_exit(int x) 
{ 
	x = x; 
} 
//�ض���fputc���� 
int fputc(int ch, FILE *f)
{      
	while((USART1->SR&0X40)==0);//ѭ������,ֱ���������   
    USART1->DR = (u8) ch;      
	return ch;
}
#endif 

/*ʹ��microLib�ķ���*/
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
 
#if EN_USART1_RX   //���ʹ���˽���
//����1�жϷ������
//ע��,��ȡUSARTx->SR�ܱ���Ī������Ĵ���   	
u8 USART_RX_BUF[USART_MAX_RECV_LEN_TEMP];     //���ջ���,���USART_MAX_RECV_LEN���ֽ�.
//����״̬
//bit15��	������ɱ�־
//bit14��	���յ�0x0d
//bit13~0��	���յ�����Ч�ֽ���Ŀ
u16 USART_RX_STA=0;       //����״̬���	  

extern u8 g_ota_runing;
extern u16 g_ota_recv_sum = 0;
extern u16 g_ota_one_pg_recv_len;
extern u16 g_ota_pg_numid;

//��ʼ��IO ����1 
//bound:������
void uart_init(u32 bound){
    //GPIO�˿�����
    GPIO_InitTypeDef GPIO_InitStructure;
	USART_InitTypeDef USART_InitStructure;
	NVIC_InitTypeDef NVIC_InitStructure;
	 
	RCC_APB2PeriphClockCmd(RCC_APB2Periph_USART1|RCC_APB2Periph_GPIOA, ENABLE);	//ʹ��USART1��GPIOAʱ��
 	USART_DeInit(USART1);  //��λ����1
	//USART1_TX   PA.9
    GPIO_InitStructure.GPIO_Pin = GPIO_Pin_9; //PA.9
    GPIO_InitStructure.GPIO_Speed = GPIO_Speed_50MHz;
    GPIO_InitStructure.GPIO_Mode = GPIO_Mode_AF_PP;	//�����������
    GPIO_Init(GPIOA, &GPIO_InitStructure); //��ʼ��PA9
   
    //USART1_RX	  PA.10
    GPIO_InitStructure.GPIO_Pin = GPIO_Pin_10;
    GPIO_InitStructure.GPIO_Mode = GPIO_Mode_IN_FLOATING;//��������
    GPIO_Init(GPIOA, &GPIO_InitStructure);  //��ʼ��PA10

   //Usart1 NVIC ����

    NVIC_InitStructure.NVIC_IRQChannel = USART1_IRQn;
	NVIC_InitStructure.NVIC_IRQChannelPreemptionPriority=3 ;//��ռ���ȼ�3
	NVIC_InitStructure.NVIC_IRQChannelSubPriority = 3;		//�����ȼ�3
	NVIC_InitStructure.NVIC_IRQChannelCmd = ENABLE;			//IRQͨ��ʹ��
	NVIC_Init(&NVIC_InitStructure);	//����ָ���Ĳ�����ʼ��VIC�Ĵ���
  
   //USART ��ʼ������
	USART_InitStructure.USART_BaudRate = bound;//һ������Ϊ9600;
	USART_InitStructure.USART_WordLength = USART_WordLength_8b;//�ֳ�Ϊ8λ���ݸ�ʽ
	USART_InitStructure.USART_StopBits = USART_StopBits_1;//һ��ֹͣλ
	USART_InitStructure.USART_Parity = USART_Parity_No;//����żУ��λ
	USART_InitStructure.USART_HardwareFlowControl = USART_HardwareFlowControl_None;//��Ӳ������������
	USART_InitStructure.USART_Mode = USART_Mode_Rx | USART_Mode_Tx;	//�շ�ģʽ

    USART_Init(USART1, &USART_InitStructure); //��ʼ������
    //USART_ITConfig(USART1, USART_IT_RXNE, ENABLE);//�����ж�
		USART_ITConfig(USART1, USART_IT_IDLE, ENABLE);//���������ж�
    USART_Cmd(USART1, ENABLE);                    //ʹ�ܴ��� 
		
		while(USART_GetFlagStatus(USART1,USART_FLAG_TC) == RESET);
		
		// 100ms
		TIM6_Int_Init(999, 7199);
		
		TIM_Cmd(TIM6, DISABLE); //�رն�ʱ��7
		
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
		return ret;//�Ƿ�cmd
	switch(data[1])
	{
		case 0x01://�趨�¶�
			g_temp_val_new.target_val = data[3]*100 + data[4]*10 +data[5];
			g_temp_val_new.target_update = 1;
			memcpy(out,data,num);
			ret = num;
			break;
		case 0x02://�趨smoke
			g_temp_val_new.target_smoke = data[2];
			g_temp_val_new.target_update = 1;
			memcpy(out,data,num);
			ret = num;
			break;
		case 0x03://�趨
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
		case 0x0b://��ȡ״̬
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
		case 0x06://��������
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
	i=strlen((const char*)UART1_TX_BUF);//�˴η������ݵĳ���
	for(j=0;j<i;j++)//ѭ����������
	{
		while(USART_GetFlagStatus(UART1,USART_FLAG_TC)==RESET);//ѭ������,ֱ���������   
		USART_SendData(UART5,(uint8_t)UART1_TX_BUF[j]);   
	}
}

void USART1_IRQHandler(void)                	//����1�жϷ������
{
#ifdef SYSTEM_SUPPORT_OS	 	
	OSIntEnter();    
#endif

		 if (USART_GetITStatus(USART1, USART_IT_IDLE) != RESET)  
	    {
				
				TIM_SetCounter(TIM6,0);
				if(USART_RX_STA==0) 				//ʹ�ܶ�ʱ��7���ж� 
				{
					TIM_Cmd(TIM6, ENABLE); 	    			//ʹ�ܶ�ʱ��7
				}

	            USART_ClearITPendingBit(USART1, USART_IT_IDLE);         //����жϱ�־
	            USART_ReceiveData(USART1);//��ȡ���� ע�⣺������Ҫ�������ܹ�����жϱ�־λ��
	    }

#ifdef SYSTEM_SUPPORT_OS	 
	OSIntExit();  											 
#endif
} 
#endif	

