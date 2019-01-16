#include "includes.h"
#include "malloc.h"
#include "common.h"
#include "usart3.h"
#include "usart5.h"
#include "sim900a.h"
#include "can1.h"
#include "can2.h"
#include "rfid.h"
#include "vs10xx.h"
#include "mp3play.h"
#include "rtc.h"

/////////////////////////UCOSII��������///////////////////////////////////
//START ����
//�����������ȼ�
#define START_TASK_PRIO      			10 //��ʼ��������ȼ�����Ϊ���
//���������ջ��С
#define START_STK_SIZE  				64
//�����ջ��8�ֽڶ���	
__align(8) static OS_STK START_TASK_STK[START_STK_SIZE];
//������
void start_task(void *pdata);	
 			   
#define LOWER_TASK_PRIO       			7
#define LOWER_STK_SIZE  		    	128
__align(8) static OS_STK LOWER_TASK_STK[LOWER_STK_SIZE];
void lower_task(void *pdata);	

//��������
//�����������ȼ�
#define USART_TASK_PRIO       			7 
//���������ջ��С
#define USART_STK_SIZE  		    	128
//�����ջ��8�ֽڶ���	
__align(8) static OS_STK USART_TASK_STK[USART_STK_SIZE];
//������
void usart_task(void *pdata);
							 
//������
//�����������ȼ�
#define MAIN_TASK_PRIO       			6 
//���������ջ��С
#define MAIN_STK_SIZE  					1200
//�����ջ��8�ֽڶ���	
__align(8) static OS_STK MAIN_TASK_STK[MAIN_STK_SIZE];
//������
void main_task(void *pdata);

//��������
//�����������ȼ�
#define HIGHER_TASK_PRIO       			3 
//���������ջ��С
#define HIGHER_STK_SIZE  		   		256
//�����ջ��8�ֽڶ���	
__align(8) static OS_STK HIGHER_TASK_STK[HIGHER_STK_SIZE];
//������
void higher_task(void *pdata);
//////////////////////////////////////////////////////////////////////////////	 

int total_ms=0;
extern int pluse_num_new;
int g_sd_existing = 0;

OS_EVENT * sem_beep;
u8 g_logname[64] = "";
u8 g_logmsg[256] = "";

void create_logfile(void);
void write_logs(char *module, char *log, u16 size, u8 mode);

u8 g_mp3_play = 0;
u8 g_mp3_play_name[32] = "";
extern u8 g_mp3_update_name[128];
extern u8 g_mp3_update;
extern u8 g_dw_write_enable;
extern vu16 g_data_pos;
extern vu16 g_data_size;
extern u8 USART3_RX_BUF_BAK[U3_RECV_LEN_ONE];
//////////////////////////////////////////////////////////////////////////////	 

//ϵͳ��ʼ��
void system_init(void)
{
	u8 res;
	u16 temp=0;
	u32 dtsize,dfsize;
	
	u8 CAN1_mode=0; //CAN����ģʽ;0,��ͨģʽ;1,����ģʽ
	u8 CAN2_mode=0; //CAN����ģʽ;0,��ͨģʽ;1,����ģʽ	
	
	delay_init(168);			//��ʱ��ʼ��  
	uart_init(115200);		//��ʼ�����ڲ�����Ϊ115200
	usart3_init(115200);		//��ʼ������3������Ϊ115200
	usart5_init(115200);

 	LED_Init();					//��ʼ��LED 
 	KEY_Init();					//������ʼ�� 

#if 0
	W25QXX_Init();				//��ʼ��W25Q128

	g_sys_env.charge_times = 123;
	
	sys_env_init();

	// Get g_bms_charged_times
	sys_env_dump();
#endif

	My_RTC_Init();		 		//��ʼ��RTC
	RTC_Set_WakeUp(RTC_WakeUpClock_CK_SPRE_16bits,0);		//����WAKE UP�ж�,1�����ж�һ��
		
	printf("SmartMotor Starting...\n");
	CAN1_Mode_Init(CAN1_mode);//CAN��ʼ����ͨģʽ,������250Kbps
	CAN2_Mode_Init(CAN2_mode);//CAN��ʼ����ͨģʽ,������500Kbps 
  
	my_mem_init(SRAMIN);		//��ʼ���ڲ��ڴ��
	my_mem_init(SRAMCCM);		//��ʼ��CCM�ڴ�� 

	TIM2_Init(9999,8399);	
	TIM4_Init(9999,8399);
	
	VS_Init();	  				//��ʼ��VS1053
#if 1
	delay_ms(1500);
	
 	exfuns_init();// alloc for fats
	// Call SD_Init internally
  f_mount(fs[0],"0:",1);
	
	temp=0;	
 	do {
		temp++;
 		res=exf_getfree("0:",&dtsize,&dfsize);
		delay_ms(200);		   
	} while(res&&temp<5);
	
 	if(res==0) {
		g_sd_existing = 1;
		printf("Read SD OK!\r\n");
	} else {
		printf("Read SD Failed!\r\n");
	}
	
	create_logfile();
	
#if 0
	u16 xx = 0;
	delay_ms(1000);
	xx = VS_Ram_Test();
	delay_ms(1000);
	music_play();
	
	while(1)
	{
		delay_ms(1000);
 		//LED1=0; 	   
 		VS_Sine_Test();	   	 
		delay_ms(1000);
		//LED1=1;
	}
#endif
#endif
}   

void SoftReset(void)
{  
	while (1) {
		delay_ms(1000);
	}
	__set_FAULTMASK(1);
 	NVIC_SystemReset();
}

//main����	  					
int main(void)
{ 	
//	SCB->VTOR = *((u32 *)0x0800FFF8);
	NVIC_PriorityGroupConfig(NVIC_PriorityGroup_2);//����ϵͳ�ж����ȼ�����2
  system_init();		//ϵͳ��ʼ�� 
 	OSInit();   
 	OSTaskCreate(start_task,(void *)0,(OS_STK *)&START_TASK_STK[START_STK_SIZE-1],START_TASK_PRIO );//������ʼ����
	OSStart();	  						    
}
//extern OS_EVENT * audiombox;	//��Ƶ������������
//��ʼ����
void start_task(void *pdata)
{  
	OS_CPU_SR cpu_sr=0;
	pdata = pdata; 	   
	sem_beep=OSSemCreate(1);
	OSStatInit();		//��ʼ��ͳ������.�������ʱ1��������	
// 	app_srand(OSTime);
	
	OS_ENTER_CRITICAL();//�����ٽ���(�޷����жϴ��)    
 	OSTaskCreate(main_task,(void *)0,(OS_STK*)&MAIN_TASK_STK[MAIN_STK_SIZE-1],MAIN_TASK_PRIO);						   
 	OSTaskCreate(usart_task,(void *)0,(OS_STK*)&USART_TASK_STK[USART_STK_SIZE-1],USART_TASK_PRIO);						   
	OSTaskCreate(higher_task,(void *)0,(OS_STK*)&HIGHER_TASK_STK[HIGHER_STK_SIZE-1],HIGHER_TASK_PRIO); 					   
	OSTaskCreate(lower_task,(void *)0,(OS_STK*)&LOWER_TASK_STK[LOWER_STK_SIZE-1],LOWER_TASK_PRIO); 					   
	OSTaskSuspend(START_TASK_PRIO);	//������ʼ����.
	OS_EXIT_CRITICAL();	//�˳��ٽ���(���Ա��жϴ��)
} 

// Play MP3 task
void lower_task(void *pdata)
{
	while(1) {
		if (g_mp3_play) {
			music_play((const char*)g_mp3_play_name);

			g_mp3_play = 0;
			memset(g_mp3_play_name, 0, 32);
		}

    	OSTimeDlyHMSM(0,0,0,500);// 500ms
	}
}

// MPU6050 Check and BT Data Parse
void main_task(void *pdata)
{
	while(1) {
		// TBD: Add MPU6050 Check
		// TBD: Add Invalid Moving Check
		// TBD: Add BT Data Parse
    	OSTimeDlyHMSM(0,0,0,500);// 500ms
	}
}

void create_logfile(void)
{
	if (0 == g_sd_existing) {
		return;
	} else {
		u8 res;
		FIL f_txt;
		RTC_TimeTypeDef RTC_TimeStruct;
		RTC_DateTypeDef RTC_DateStruct;
		
		RTC_GetTime(RTC_Format_BIN,&RTC_TimeStruct);
		RTC_GetDate(RTC_Format_BIN, &RTC_DateStruct);
		
		sprintf((char*)g_logname,"0:/LOG/20%02d%02d%02d_%02d%02d%02d.log",RTC_DateStruct.RTC_Year,RTC_DateStruct.RTC_Month,RTC_DateStruct.RTC_Date,RTC_TimeStruct.RTC_Hours,RTC_TimeStruct.RTC_Minutes,RTC_TimeStruct.RTC_Seconds);
		
		res = f_open(&f_txt,(const TCHAR*)g_logname,FA_READ|FA_WRITE|FA_CREATE_ALWAYS);
		if (0 == res) {
			f_close(&f_txt);
		}
	}
}

void write_logs(char *module, char *log, u16 size, u8 mode)
{
	if (0 == g_sd_existing) {
		return;
	} else {
		u8 err;
		u8 res;
		u32 br;
		//char log_buf[] = "hello log"; 
		FIL f_txt;
		RTC_TimeTypeDef RTC_TimeStruct;
		
		RTC_GetTime(RTC_Format_BIN,&RTC_TimeStruct);
		
		memset(g_logmsg, 0, 256);
		
		if (0 == mode) {
			sprintf((char*)g_logmsg,"%02d%02d%02d:RECV Data(%s) %s\n",RTC_TimeStruct.RTC_Hours,RTC_TimeStruct.RTC_Minutes,RTC_TimeStruct.RTC_Seconds,module,log);
		} else if (1 == mode) {
			sprintf((char*)g_logmsg,"%02d%02d%02d:SEND Data(%s) %s\n",RTC_TimeStruct.RTC_Hours,RTC_TimeStruct.RTC_Minutes,RTC_TimeStruct.RTC_Seconds,module,log);
		} else if (2 == mode) {
			sprintf((char*)g_logmsg,"%02d%02d%02d:IMPT Data(%s) %s\n",RTC_TimeStruct.RTC_Hours,RTC_TimeStruct.RTC_Minutes,RTC_TimeStruct.RTC_Seconds,module,log);
		} else {
			sprintf((char*)g_logmsg,"%02d%02d%02d:OTHR Data(%s) %s\n",RTC_TimeStruct.RTC_Hours,RTC_TimeStruct.RTC_Minutes,RTC_TimeStruct.RTC_Seconds,module,log);
		}
		
		printf("%s", g_logmsg);
		
		OSSemPend(sem_beep,0,&err);
		res= f_open(&f_txt,(const TCHAR*)g_logname,FA_READ|FA_WRITE);
		if(res==0)
		{			
			f_lseek(&f_txt, f_txt.fsize);
			f_write(&f_txt,g_logmsg, strlen((const char*)g_logmsg), (UINT*)&br);
			f_close(&f_txt);
		}
		OSSemPost(sem_beep);
	}
}

// READ RFID and other task
void usart_task(void *pdata)
{
	u8 loop_cnt = 0;

	while(1) {
		if((UART5_RX_STA&(1<<15)) != 0) {
			cpr74_read_calypso();
			UART5_RX_STA = 0;
		}
		
		if (loop_cnt++ == 3) {
			loop_cnt = 0;
			printf("Hall Counter = %d\n", g_dw_write_enable);
		}

#if 1
		if (1 == g_dw_write_enable) {
			u32 br = 0;
			u8 res = 0;
			FIL f_txt;
			
			res = f_open(&f_txt,(const TCHAR*)g_mp3_update_name,FA_READ|FA_WRITE);
			if (0 == res) {
				f_lseek(&f_txt, f_txt.fsize);
				f_write(&f_txt, USART3_RX_BUF_BAK+g_data_pos, g_data_size, (UINT*)&br);
				f_close(&f_txt);
			}
			
			g_dw_write_enable = 0;
		}
#endif

    	OSTimeDlyHMSM(0,0,0,500);// 500ms
    	OSTimeDlyHMSM(0,0,0,500);// 500ms
	}
}

//��������
void higher_task(void *pdata)
{
	while (1) {
		sim7500e_communication_loop(0,NULL,NULL);
    	OSTimeDlyHMSM(0,0,0,500);// 500ms
	}
}

//Ӳ��������
void HardFault_Handler(void)
{
	u32 i;
	u8 t=0;
	u32 temp;
	temp=SCB->CFSR;					//fault״̬�Ĵ���(@0XE000ED28)����:MMSR,BFSR,UFSR
 	printf("CFSR:%8X\r\n",temp);	//��ʾ����ֵ
	temp=SCB->HFSR;					//Ӳ��fault״̬�Ĵ���
 	printf("HFSR:%8X\r\n",temp);	//��ʾ����ֵ
 	temp=SCB->DFSR;					//����fault״̬�Ĵ���
 	printf("DFSR:%8X\r\n",temp);	//��ʾ����ֵ
   	temp=SCB->AFSR;					//����fault״̬�Ĵ���
 	printf("AFSR:%8X\r\n",temp);	//��ʾ����ֵ
 	// LED1=!LED1;
 	while(t<5)
	{
		t++;
		LED2=!LED2;
		//BEEP=!BEEP;
		for(i=0;i<0X1FFFFF;i++);
 	}
}
