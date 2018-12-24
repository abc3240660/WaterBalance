#include "led.h"
#include "delay.h"
#include "sys.h"
#include "Tiky_LCD.h" 
#include "usart.h"
#include "sram.h"
#include "timer.h"
#include "malloc.h"
#include "GUI.h"
#include "GUIDEMO.h"
#include "includes.h"
#include "ec11key.h"
#include "adc.h"
#include "temp.h"
#include "control.h"
#include "PID.h"
#include "common.h"
#include "dma.h"

//START����
//������������ȼ�
#define START_TASK_PRIO				3
//�����ջ��С 
#define START_STK_SIZE			  256
//������ƿ�
OS_TCB StartTaskTCB;
//�����ջ	
CPU_STK START_TASK_STK[START_STK_SIZE];
//������
void start_task(void *p_arg);

//TOUCH����
//�����������ȼ�
#define TOUCH_TASK_PRIO				4
//�����ջ��С
#define TOUCH_STK_SIZE				128
//������ƿ�
OS_TCB TouchTaskTCB;
//�����ջ
CPU_STK TOUCH_TASK_STK[TOUCH_STK_SIZE];
//touch����
void touch_task(void *p_arg);

//TEMP����
//�����������ȼ�
#define TEMP_TASK_PRIO 				5
//�����ջ��С
#define TEMP_STK_SIZE				64
//������ƿ�
OS_TCB TempTaskTCB;
//�����ջ
CPU_STK TEMP_TASK_STK[TEMP_STK_SIZE];
//led0����
void temp_watch_task(void *p_arg);

//EMWINDEMO����
//�����������ȼ�
#define EMWINDEMO_TASK_PRIO			6
//�����ջ��С
#define EMWINDEMO_STK_SIZE			2048
//������ƿ�
OS_TCB EmwindemoTaskTCB;
//�����ջ
CPU_STK EMWINDEMO_TASK_STK[EMWINDEMO_STK_SIZE];
//emwindemo_task����
void emwindemo_task(void *p_arg);

//Run����
//�����������ȼ�
#define RUN_TASK_PRIO 				7
//�����ջ��С
#define RUN_STK_SIZE				64
//������ƿ�
OS_TCB RUNTaskTCB;
//�����ջ
CPU_STK RUN_TASK_STK[RUN_STK_SIZE];
//led0����
void run_task(void *p_arg);


////////////////////////////////////////////////////////
OS_TMR 	tmr1;		//��ʱ��1
OS_TMR	tmr2;		//��ʱ��2
void tmr1_callback(void *p_tmr, void *p_arg); 	//��ʱ��1�ص�����
void tmr2_callback(void *p_tmr, void *p_arg);	//��ʱ��2�ص�����

u32 Check_Sys=0;

// Target�¶�ֵ
extern const short baseTempMax[];
extern EVENT_VAL g_event_val_new;// ��ǰ״̬
extern EVENT_VAL g_event_val_last;// ��һ��״̬
extern TEMP_VAL g_temp_val_new;// ��ǰ�¶�
extern TEMP_VAL g_temp_val_last;// ��һ���¶�

//
// User Interface
//

// �Ƿ������ش���
// 0-OK 1-FLAME ERROR 2-SENSOR ERROR
extern int g_fatal_error;

/*
0 idle
1 start up
2 run
3 feed
4 shutdown
*/
extern int g_current_mode;//
extern int g_current_mode_old;//

// ����ģʽ�Ŀ�ʼ�ͽ�����־
// 0-end 1-start
extern int g_startup_mode;
extern int g_run_mode;
extern int g_feed_mode;
extern int g_shutdown_mode;

int g_startup_mode_last;
int g_run_mode_last;
int g_feed_mode_last;
int g_shutdown_mode_last;

// ����ģʽ�ļ�ʱ��
extern u16 g_startup_mode_counter;
extern u16 g_run_mode_counter_mins;// minutes
extern u16 g_run_mode_counter_sec;// seconds
extern u16 g_run_mode_counter_hour;// hours
extern u16 g_feed_mode_counter;
extern u16 g_shutdown_mode_counter;


// RUNģʽ����ʱ��ʱ�������Լ�ʹ��
// ��λseconds(Ĭ��480s)
extern int g_run_timer_setting;

// 0-���϶�(Ĭ��) 1���϶�
// g_temp_val_new.temp_unit

// �Ҳ��¶���(��Ӧ�¶Ȱ�1234)
// g_temp_val_new.temp1
// g_temp_val_new.temp2
// g_temp_val_new.temp3
// g_temp_val_new.temp4

// RUN�������������¶�
// g_temp_val_new.temp5

// �¶Ȱ��Ƿ���쳣
// 0-OK(default) 1-�¶Ȱ��쳣
extern int g_temp1_error;
extern int g_temp2_error;
extern int g_temp3_error;
extern int g_temp4_error;
extern int g_temp5_error;

// ��ʾ�Ƿ��ʼ��
// 0-δ��ʼ��(default), 1-�ѳ�ʼ��
extern int g_factory_reseted;

// ����startup����run��־
// 0-run, 1-startup mode(default)
extern int g_startup_enable;

// smoke�������ٷֱȣ�Ĭ��ֵΪ50(%)
extern int g_smoke_val_percent;
extern int g_target_temp_val;
extern int g_set_temp;

extern int g_temp_center;

// EMWIN�������
extern void GUIDEMO_AfterLogo(void);
extern void GUIDEMO_UpdateTemp(int *temp_val);

extern int g_time_remain;

u8 g_ota_runing = 0;
u16 g_ota_recv_sum = 0;
u16 g_ota_one_pg_recv_len = 0;
u16 g_ota_pg_numid = 0;

#if 1
// LOGOͼƬת������
extern const unsigned char gImage_camp[1308];

// ��ȡÿ�����ص��2�ֽ���ɫ��Ϣ
u16 image_getcolor(u8 mode,u8 *str)
{
	u16 color;
	if(mode)
	{
		color=((u16)*str++)<<8;
		color|=*str;
	}else
	{
		color=*str++;
		color|=((u16)*str)<<8;
	}
	return color;	
}

// д��һ������յ����꣬Ȼ��д����������(��ɫ��Ϣ)
void image_show(u16 xsta,u16 ysta,u16 xend,u16 yend,u8 scan,u8 *p)
{  
	u32 i;
	u32 len=0;
	
	BlockWrite(xsta,xend,ysta,yend);
	
	len=(xend-xsta+1)*(yend-ysta+1);	//д������ݳ���
	for(i=0;i<len;i++)
	{
		*(__IO u16 *) (Bank1_LCD_D) = image_getcolor(scan&(1<<4),p);
		p+=2;
	}	    					  	    
} 

void image_display(u16 x,u16 y,u8 * imgx)
{
	HEADCOLOR *imginfo;
 	u8 ifosize=sizeof(HEADCOLOR);//�õ�HEADCOLOR�ṹ��Ĵ�С
	imginfo=(HEADCOLOR*)imgx;
 	image_show(x,y,x+imginfo->w-1,y+imginfo->h-1,imginfo->scan,imgx+ifosize);
}

// ��ָ��λ�ÿ�ʼ��ʾһ��ͼƬ
void disp_img(u16 x, u16 y)
{
	image_display(x,y,(u8*)gImage_camp);//��ָ����ַ��ʾͼƬ
}
#endif

int main(void)
{
	OS_ERR err;
	CPU_SR_ALLOC();	
	
	delay_init();	    	//��ʱ������ʼ��	  
	NVIC_PriorityGroupConfig(NVIC_PriorityGroup_2); 	//����NVIC�жϷ���2:2λ��ռ���ȼ���2λ��Ӧ���ȼ�
#ifdef SRAM_MEMDEV
	FSMC_SRAM_Init();		//��ʼ��SRAM
#endif
 	LED_Init();			    //LED�˿ڳ�ʼ��
	Lcd_Initialize();
	
	// ������ɨ�跽ʽ����
	LCD_Display_Dir(HORIZON_DISPLAY);

	EC11_EXTI_Init();//EC11��������ʼ��
	my_mem_init(SRAMIN); 	//��ʼ���ڲ��ڴ��
	
	Adc_Init();
	Control_Init();

	//PID_Init();
	
	TIM3_Int_Init(9999, 7199);
	uart_init(115200);	 	//���ڳ�ʼ��Ϊ115200
	Usart_DMA_Init();
	
	OSInit(&err);		//��ʼ��UCOSIII
	OS_CRITICAL_ENTER();//�����ٽ���
	//������ʼ����
	OSTaskCreate((OS_TCB 	* )&StartTaskTCB,		//������ƿ�
				 (CPU_CHAR	* )"start task", 		//��������
                 (OS_TASK_PTR )start_task, 			//������
                 (void		* )0,					//���ݸ��������Ĳ���
                 (OS_PRIO	  )START_TASK_PRIO,     //�������ȼ�
                 (CPU_STK   * )&START_TASK_STK[0],	//�����ջ����ַ
                 (CPU_STK_SIZE)START_STK_SIZE/10,	//�����ջ�����λ
                 (CPU_STK_SIZE)START_STK_SIZE,		//�����ջ��С
                 (OS_MSG_QTY  )0,					//�����ڲ���Ϣ�����ܹ����յ������Ϣ��Ŀ,Ϊ0ʱ��ֹ������Ϣ
                 (OS_TICK	  )0,					//��ʹ��ʱ��Ƭ��תʱ��ʱ��Ƭ���ȣ�Ϊ0ʱΪĬ�ϳ��ȣ�
                 (void   	* )0,					//�û�����Ĵ洢��
                 (OS_OPT      )OS_OPT_TASK_STK_CHK|OS_OPT_TASK_STK_CLR, //����ѡ��
                 (OS_ERR 	* )&err);				//��Ÿú�������ʱ�ķ���ֵ
	OS_CRITICAL_EXIT();	//�˳��ٽ���	 
	OSStart(&err);  //����UCOSIII
							 
}

//��ʼ������
void start_task(void *p_arg)
{
	OS_ERR err;
	CPU_SR_ALLOC();
	p_arg = p_arg;

	CPU_Init();
#if OS_CFG_STAT_TASK_EN > 0u
   OSStatTaskCPUUsageInit(&err);  	//ͳ������                
#endif
	
#ifdef CPU_CFG_INT_DIS_MEAS_EN		//���ʹ���˲����жϹر�ʱ��
    CPU_IntDisMeasMaxCurReset();	
#endif

#if	OS_CFG_SCHED_ROUND_ROBIN_EN  //��ʹ��ʱ��Ƭ��ת��ʱ��
	 //ʹ��ʱ��Ƭ��ת���ȹ���,ʱ��Ƭ����Ϊ1��ϵͳʱ�ӽ��ģ���1*5=5ms
	OSSchedRoundRobinCfg(DEF_ENABLED,1,&err);  
#endif		
	RCC_AHBPeriphClockCmd(RCC_AHBPeriph_CRC,ENABLE);//����CRCʱ��
#if 1
	WM_SetCreateFlags(WM_CF_MEMDEV);
	GUI_Init();  			//STemWin��ʼ��
#if 0
	//������ʱ��1
	OSTmrCreate((OS_TMR		*)&tmr1,		//��ʱ��1
                (CPU_CHAR	*)"tmr1",		//��ʱ������
                (OS_TICK	 )0,			//0
                (OS_TICK	 )10,          //100*10=1000ms
                (OS_OPT		 )OS_OPT_TMR_PERIODIC, //����ģʽ
                (OS_TMR_CALLBACK_PTR)tmr1_callback,//��ʱ��1�ص�����
                (void	    *)0,			//����Ϊ0
                (OS_ERR	    *)&err);		//���صĴ�����
				
				
	//������ʱ��2
	OSTmrCreate((OS_TMR		*)&tmr2,		
                (CPU_CHAR	*)"tmr2",		
                (OS_TICK	 )200,			//200*10=2000ms	
                (OS_TICK	 )0,   					
                (OS_OPT		 )OS_OPT_TMR_ONE_SHOT, 	//���ζ�ʱ��
                (OS_TMR_CALLBACK_PTR)tmr2_callback,	//��ʱ��2�ص�����
                (void	    *)0,			
                (OS_ERR	    *)&err);
	OSTmrStart(&tmr1,&err);	//������ʱ��1
	OSTmrStart(&tmr2,&err);	//������ʱ��2
#endif
	
	OS_CRITICAL_ENTER();	//�����ٽ���
	// STemWin UI����	
	OSTaskCreate((OS_TCB*     )&EmwindemoTaskTCB,		
				 (CPU_CHAR*   )"Emwindemo task", 		
                 (OS_TASK_PTR )emwindemo_task, 			
                 (void*       )0,					
                 (OS_PRIO	  )EMWINDEMO_TASK_PRIO,     
                 (CPU_STK*    )&EMWINDEMO_TASK_STK[0],	
                 (CPU_STK_SIZE)EMWINDEMO_STK_SIZE/10,	
                 (CPU_STK_SIZE)EMWINDEMO_STK_SIZE,		
                 (OS_MSG_QTY  )0,					
                 (OS_TICK	  )0,  					
                 (void*       )0,					
                 (OS_OPT      )OS_OPT_TASK_STK_CHK|OS_OPT_TASK_STK_CLR,
                 (OS_ERR*     )&err);
#endif
#if 1
	// EC11��������
	OSTaskCreate((OS_TCB*     )&TouchTaskTCB,		
				 (CPU_CHAR*   )"Touch task", 		
                 (OS_TASK_PTR )touch_task, 			
                 (void*       )0,					
                 (OS_PRIO	  )TOUCH_TASK_PRIO,     
                 (CPU_STK*    )&TOUCH_TASK_STK[0],	
                 (CPU_STK_SIZE)TOUCH_STK_SIZE/10,	
                 (CPU_STK_SIZE)TOUCH_STK_SIZE,		
                 (OS_MSG_QTY  )0,					
                 (OS_TICK	  )0,  					
                 (void*       )0,					
                 (OS_OPT      )OS_OPT_TASK_STK_CHK|OS_OPT_TASK_STK_CLR,
                 (OS_ERR*     )&err);		
#endif
#if 1
	// �¶ȶ�ȡ����
	OSTaskCreate((OS_TCB*     )&TempTaskTCB,		
				 (CPU_CHAR*   )"Temp task", 		
                 (OS_TASK_PTR )temp_watch_task, 			
                 (void*       )0,					
                 (OS_PRIO	  )TEMP_TASK_PRIO,     
                 (CPU_STK*    )&TEMP_TASK_STK[0],	
                 (CPU_STK_SIZE)TEMP_STK_SIZE/10,	
                 (CPU_STK_SIZE)TEMP_STK_SIZE,		
                 (OS_MSG_QTY  )0,					
                 (OS_TICK	  )0,  					
                 (void*       )0,					
                 (OS_OPT      )OS_OPT_TASK_STK_CHK|OS_OPT_TASK_STK_CLR,
                 (OS_ERR*     )&err);
#endif	
#if 1
	// RUN����
	OSTaskCreate((OS_TCB*     )&RUNTaskTCB,		
				 (CPU_CHAR*   )"RUN task", 		
                 (OS_TASK_PTR )run_task, 			
                 (void*       )0,					
                 (OS_PRIO	  )RUN_TASK_PRIO,     
                 (CPU_STK*    )&RUN_TASK_STK[0],	
                 (CPU_STK_SIZE)RUN_STK_SIZE/10,	
                 (CPU_STK_SIZE)RUN_STK_SIZE,		
                 (OS_MSG_QTY  )0,					
                 (OS_TICK	  )0,  					
                 (void*       )0,					
                 (OS_OPT      )OS_OPT_TASK_STK_CHK|OS_OPT_TASK_STK_CLR,
                 (OS_ERR*     )&err);
#endif								 
								 
	OS_TaskSuspend((OS_TCB*)&StartTaskTCB,&err);		//����ʼ����			 
	OS_CRITICAL_EXIT();	//�˳��ٽ���
}

//��ʱ��1�Ļص����� 100ms
void tmr1_callback(void *p_tmr, void *p_arg)
{
	static u8 tmr1_num_s=0;//1S
	tmr1_num_s++;
	if(tmr1_num_s>=10){
//		if(g_startup_mode) g_startup_mode_counter++;
//		else g_startup_mode_counter=0;
		
//		if(g_feed_mode) g_feed_mode_counter++;
//		else g_feed_mode_counter=0;
		
//		if(g_shutdown_mode) g_shutdown_mode_counter++;
//		else g_shutdown_mode_counter=0;
		
		tmr1_num_s=0;
	}

/*	static u32 tmr1_num=0;
//	LCD_ShowxNum(62,111,tmr1_num,3,16,0x80); //��ʾ��ʱ��1��ִ�д���
//	LCD_Fill(6,131,114,313,lcd_discolor[tmr1_num%14]);//�������
	tmr1_num++;		//��ʱ��1ִ�д�����1*/
//	Check_Sys++;
}

//��ʱ��2�Ļص�����
void tmr2_callback(void *p_tmr,void *p_arg)
{
/*	static u8 tmr2_num = 0;
	tmr2_num++;		//��ʱ��2ִ�д�����1
	LCD_ShowxNum(182,111,tmr2_num,3,16,0x80);  //��ʾ��ʱ��1ִ�д���
	LCD_Fill(126,131,233,313,lcd_discolor[tmr2_num%14]); //�������
	LED1 = ~LED1;
	printf("��ʱ��2���н���\r\n");*/
}
extern int GUIDEMO_FeedCompleteUI(void);
extern int GUIDEMO_ShutdownCompleteUI(void);
extern int GUIDEMO_FeedShutdownTmerUI(int mode);
extern int GUIDEMO_FeedInitialUI(void);
extern int GUIDEMO_StartupTimerUI(void);
extern int GUIDEMO_MainMenu(int sel);
extern void GUIDEMO_DayNightSwitch(void);
extern void GUIDEMO_DayModeSet(void);
extern int GUIDEMO_StartupInitialUI(int mode);
extern int GUIDEMO_BigCircleUI(int is_smoke_ui);
extern void GUIDEMO_LeftOneExitUI(int time1, int time2,int ui_sel);
extern int GUIDEMO_ShutdownInitialUI(void);
extern int GUIDEMO_SetupSubItemsUI(int index_new, int index_last);
extern int GUIDEMO_ResetInitialUI(void);
extern int GUIDEMO_error(int err);
extern int GUIDEMO_StartupTimerBypassedUI(void);

// EMWIN�������
int g_GUI=0;
int g_GUI_last=0;
int g_GUI_Sel=0;
void emwindemo_task(void *p_arg)
{
					int sel1=0;
					int sel2=0;
	EVENT_VAL evt_val = g_event_val_last;
	TEMP_VAL temp_val = g_temp_val_last;
		EC11_STA ec_sta = EC11_IDLE;	
	int flag=0;
	int time_remain = 10;
		int temp_val1[5]={11,12,13,14};
	// �ô��¶�Ӧ��Ϊ���¶Ȱ�����
	// ����¶Ȱ�û�ӻ��߶�������������Ϊ0���¶Ȼ���ʾΪ"-"
	// �ô�Ϊ��һ����ʾ��������temp_watch_task�и����¶���Ϣ����
	temp_val.temp1 = 0;
	temp_val.temp2 = 0;
	temp_val.temp3 = 0;
	temp_val.temp4 = 0;
	temp_val.temp5 = 0;
	
	// Ĭ��ֵ������������

	g_temp_val_new.target_smoke=1;
	g_temp_val_new.target_val=155;

	//g_temp_val_new = temp_val;
	g_event_val_new = evt_val;
	
	LCD_Clear(WHITE);

#if 1
	disp_img(50, 100);
	delay_ms(5000);
#endif
	
	while(1)
	{
		if ((1 == g_run_mode) && (GUI_MAIN == g_GUI)) {
			g_GUI = GUI_RUN;
		}
		
		switch(g_GUI)
		{
			case GUI_MAIN:
				g_GUI_Sel = GUIDEMO_MainMenu(g_GUI_Sel);
				switch(g_GUI_Sel)
				{
					case 0 :g_GUI = GUI_TEMP_SET;break;
					case 1 :g_GUI = GUI_FEED;break;
					case 2 :g_GUI = GUI_SHUTWOWN;break;
					case 3 :g_GUI = GUI_SUB_MAIN;break;
					case 4 :g_GUI = GUI_MAIN;break;
					case 5 : g_GUI=(g_current_mode==SHUT)? GUI_SHUTWOWN:GUI_RUN;break;
					case GUI_ERROR_OCCUR	:g_GUI = GUI_ERROR_OCCUR;break;
					case DIRECT_SHUT_DOWN	:g_GUI = GUI_SHUTWOWN_TIMER;break;
					case DIRECT_STARTUP  	:g_GUI = GUI_START_UP;g_GUI_Sel=0;break;
					case DIRECT_FEED		:g_GUI = GUI_FEED_TIMER;break;
					case DIRECT_SHUT_COMPELED:g_GUI = GUI_SHUTDOWN_COMPELE;break;
					
					default :break;
				}
				break;
			case GUI_TEMP_SET:
				g_GUI_Sel = GUIDEMO_BigCircleUI(0);
				switch(g_GUI_Sel)
				{
					case 0:g_GUI = GUI_START_UP;g_GUI_Sel=0;break;
					case 1:g_GUI = GUI_START_UP;g_GUI_Sel=1;break;
					case 2:g_GUI = GUI_SMOKE_SET;break;
					case GUI_ERROR_OCCUR	:g_GUI = GUI_ERROR_OCCUR;break;
					case DIRECT_SHUT_DOWN	:g_GUI = GUI_SHUTWOWN_TIMER;break;
					case DIRECT_STARTUP  	:g_GUI = GUI_START_UP;g_GUI_Sel=0;break;
					case DIRECT_FEED		:g_GUI = GUI_FEED_TIMER;break;
					case DIRECT_SHUT_COMPELED:g_GUI = GUI_SHUTDOWN_COMPELE;break;
				}
				break;
			case GUI_SMOKE_SET:
				g_GUI_Sel = GUIDEMO_BigCircleUI(1);
				switch(g_GUI_Sel)
				{
					case 0:g_GUI = GUI_START_UP;g_GUI_Sel=0;break;
					case 1:g_GUI = GUI_START_UP;g_GUI_Sel=1;break;
					case GUI_ERROR_OCCUR	:g_GUI = GUI_ERROR_OCCUR;break;
					case DIRECT_SHUT_DOWN	:g_GUI = GUI_SHUTWOWN_TIMER;break;
					case DIRECT_STARTUP  	:g_GUI = GUI_START_UP;g_GUI_Sel=0;break;
					case DIRECT_FEED		:g_GUI = GUI_FEED_TIMER;break;
					case DIRECT_SHUT_COMPELED:g_GUI = GUI_SHUTDOWN_COMPELE;break;
				}		
				break;
			case GUI_START_UP:
				g_GUI_Sel = GUIDEMO_StartupInitialUI(g_GUI_Sel);
				switch(g_GUI_Sel)
				{
					case 0: g_GUI=GUI_START_UP_TIMER;break;
					case 1: g_GUI=GUI_RUN;break;
					case 2: g_GUI=GUI_RUN;break;
					case 3: g_GUI=(g_current_mode==RUN)? GUI_RUN:GUI_MAIN;g_GUI_Sel=0;break;
					//case 3: g_GUI=GUI_SHUTWOWN;break;
					case GUI_ERROR_OCCUR	:g_GUI = GUI_ERROR_OCCUR;break;
					case DIRECT_SHUT_DOWN	:g_GUI = GUI_SHUTWOWN_TIMER;break;
					case DIRECT_STARTUP  	:g_GUI = GUI_START_UP;g_GUI_Sel=0;break;
					case DIRECT_FEED		:g_GUI = GUI_FEED_TIMER;break;
					case DIRECT_SHUT_COMPELED:g_GUI = GUI_SHUTDOWN_COMPELE;break;
				}
				break;
			case GUI_START_UP_TIMER:
				g_GUI_Sel=GUIDEMO_StartupTimerUI();
				switch(g_GUI_Sel)
				{
					case 0: g_GUI= GUI_START_UP_BYPASS;break;
					//case 1: g_GUI= GUI_MAIN;g_GUI_Sel=0;break;
					case 1: 
						if(g_current_mode!=SHUT)
							{
								g_shutdown_mode_counter=1200;
								g_current_mode=SHUT;
							}	
							
							g_GUI=GUI_SHUTWOWN;
							g_GUI_Sel=0;
					break;
					case 2: g_GUI= GUI_RUN;break;
					case GUI_ERROR_OCCUR	:g_GUI = GUI_ERROR_OCCUR;break;
					case DIRECT_SHUT_DOWN	:g_GUI = GUI_SHUTWOWN_TIMER;break;
					case DIRECT_STARTUP  	:g_GUI = GUI_START_UP;g_GUI_Sel=0;break;
					case DIRECT_FEED		:g_GUI = GUI_FEED_TIMER;break;
					case DIRECT_SHUT_COMPELED:g_GUI = GUI_SHUTDOWN_COMPELE;break;
				}
				break;
			case GUI_RUN:
				g_GUI_Sel=GUIDEMO_BigCircleUI(2);
				switch(g_GUI_Sel)
				{
					case 0: g_GUI = GUI_MAIN;g_GUI_Sel=0;break;
					case GUI_ERROR_OCCUR	:g_GUI = GUI_ERROR_OCCUR;break;
					case DIRECT_SHUT_DOWN	:g_GUI = GUI_SHUTWOWN_TIMER;break;
					case DIRECT_STARTUP  	:g_GUI = GUI_START_UP;g_GUI_Sel=0;break;
					case DIRECT_FEED		:g_GUI = GUI_FEED_TIMER;break;
					case DIRECT_SHUT_COMPELED:g_GUI = GUI_SHUTDOWN_COMPELE;break;
				}
				break;
			case GUI_START_UP_BYPASS:
				g_GUI_Sel=GUIDEMO_StartupTimerBypassedUI();
				switch(g_GUI_Sel)
				{
					case 0: g_GUI= GUI_RUN;break;
					case 1: g_GUI= GUI_START_UP_TIMER;break;
					case 2: g_GUI= GUI_RUN;break;
					case GUI_ERROR_OCCUR	:g_GUI = GUI_ERROR_OCCUR;break;
					case DIRECT_SHUT_DOWN	:g_GUI = GUI_SHUTWOWN_TIMER;break;
					case DIRECT_STARTUP  	:g_GUI = GUI_START_UP;g_GUI_Sel=0;break;
					case DIRECT_FEED		:g_GUI = GUI_FEED_TIMER;break;
					case DIRECT_SHUT_COMPELED:g_GUI = GUI_SHUTDOWN_COMPELE;break;
				}
				break;
			case GUI_FEED:
				g_GUI_Sel = GUIDEMO_FeedInitialUI();
				switch(g_GUI_Sel)
				{
					case 0: g_GUI= GUI_FEED_TIMER;break;
					case 1: g_GUI= GUI_MAIN;g_GUI_Sel=1;break;
					case GUI_ERROR_OCCUR	:g_GUI = GUI_ERROR_OCCUR;break;
					case DIRECT_SHUT_DOWN	:g_GUI = GUI_SHUTWOWN_TIMER;break;
					case DIRECT_STARTUP  	:g_GUI = GUI_START_UP;g_GUI_Sel=0;break;
					case DIRECT_FEED		:g_GUI = GUI_FEED_TIMER;break;
					case DIRECT_SHUT_COMPELED:g_GUI = GUI_SHUTDOWN_COMPELE;break;
				}
				break;
			case GUI_FEED_TIMER:
				g_GUI_Sel = GUIDEMO_FeedShutdownTmerUI(0);
				switch(g_GUI_Sel)
				{
					case 0: g_GUI= GUI_MAIN;g_GUI_Sel=1;break;
					case 1: g_GUI= GUI_FEED_COMPELE;break;
					case GUI_ERROR_OCCUR	:g_GUI = GUI_ERROR_OCCUR;break;
					case DIRECT_SHUT_DOWN	:g_GUI = GUI_SHUTWOWN_TIMER;break;
					case DIRECT_STARTUP  	:g_GUI = GUI_START_UP;g_GUI_Sel=0;break;
					case DIRECT_FEED		:g_GUI = GUI_FEED_TIMER;break;
					case DIRECT_SHUT_COMPELED:g_GUI = GUI_SHUTDOWN_COMPELE;break;
				}
				break;
			case GUI_FEED_COMPELE:
				g_GUI_Sel = GUIDEMO_FeedCompleteUI();
				switch(g_GUI_Sel)
				{
					case 0: g_GUI= GUI_MAIN;g_GUI_Sel=1;break;
					//case 1: g_GUI= GUI_FEED_COMPELE;break;
					case GUI_ERROR_OCCUR	:g_GUI = GUI_ERROR_OCCUR;break;
					case DIRECT_SHUT_DOWN	:g_GUI = GUI_SHUTWOWN_TIMER;break;
					case DIRECT_STARTUP  	:g_GUI = GUI_START_UP;g_GUI_Sel=0;break;
					case DIRECT_FEED		:g_GUI = GUI_FEED_TIMER;break;
					case DIRECT_SHUT_COMPELED:g_GUI = GUI_SHUTDOWN_COMPELE;break;
				}
				break;
			case GUI_SHUTWOWN:
				if(g_current_mode==SHUT)
				{
					g_GUI=GUI_SHUTWOWN_TIMER;
					break;
				}
				g_GUI_Sel = GUIDEMO_ShutdownInitialUI();
				switch(g_GUI_Sel)
				{
					case 0: g_GUI= GUI_SHUTWOWN_TIMER;break;
					case 1: g_GUI= GUI_MAIN;g_GUI_Sel=2;break;
					case GUI_ERROR_OCCUR	:g_GUI = GUI_ERROR_OCCUR;break;
					case DIRECT_SHUT_DOWN	:g_GUI = GUI_SHUTWOWN_TIMER;break;
					case DIRECT_STARTUP  	:g_GUI = GUI_START_UP;g_GUI_Sel=0;break;
					case DIRECT_FEED		:g_GUI = GUI_FEED_TIMER;break;
					case DIRECT_SHUT_COMPELED:g_GUI = GUI_SHUTDOWN_COMPELE;break;
				}
				break;
			case GUI_SHUTWOWN_TIMER:
				g_GUI_Sel = GUIDEMO_FeedShutdownTmerUI(1);
				switch(g_GUI_Sel)
				{
					case 0: g_GUI= GUI_MAIN;g_GUI_Sel=2;break;
					case 1: g_GUI= GUI_SHUTDOWN_COMPELE;break;
					case GUI_ERROR_OCCUR	:g_GUI = GUI_ERROR_OCCUR;break;
					case DIRECT_SHUT_DOWN	:g_GUI = GUI_SHUTWOWN_TIMER;break;
					case DIRECT_STARTUP  	:g_GUI = GUI_START_UP;g_GUI_Sel=0;break;
					case DIRECT_FEED		:g_GUI = GUI_FEED_TIMER;break;
					case DIRECT_SHUT_COMPELED:g_GUI = GUI_SHUTDOWN_COMPELE;break;
				}
				break;
			case GUI_SHUTDOWN_COMPELE:
				GUIDEMO_ShutdownCompleteUI();
				break;
			case GUI_SUB_MAIN:

				g_GUI_Sel = GUIDEMO_SetupSubItemsUI(sel1, sel2);
				switch(g_GUI_Sel)
				{
					case 0: g_GUI=GUI_SUB_MAIN;break;
					case 1: g_GUI=GUI_SUB_MAIN;sel1=1 ;sel2=1;break;
					case 2: g_GUI=GUI_RESET;break;
					case 3: g_GUI=GUI_SUB_MAIN;sel1=3 ;sel2=3;break;
					case 4: g_GUI=GUI_RUN;g_GUI_Sel=0;break;
					case 5: g_GUI=(g_current_mode==RUN)? GUI_RUN:GUI_MAIN;g_GUI_Sel=3;break;
					case GUI_ERROR_OCCUR	:g_GUI = GUI_ERROR_OCCUR;break;
					case DIRECT_SHUT_DOWN	:g_GUI = GUI_SHUTWOWN_TIMER;break;
					case DIRECT_STARTUP  	:g_GUI = GUI_START_UP;g_GUI_Sel=0;break;
					case DIRECT_FEED		:g_GUI = GUI_FEED_TIMER;break;
					case DIRECT_SHUT_COMPELED:g_GUI = GUI_SHUTDOWN_COMPELE;break;
				}
				break;
			case GUI_RESET:
				g_GUI_Sel = GUIDEMO_ResetInitialUI();
				switch(g_GUI_Sel)
				{
					case 0: g_GUI=GUI_SUB_MAIN;sel1=2 ;sel2=0;break;
					case 1: g_GUI=GUI_RUN;break;
					case 2: g_GUI=GUI_SUB_MAIN;sel1=2 ;sel2=0;break;
					case GUI_ERROR_OCCUR	:g_GUI = GUI_ERROR_OCCUR;break;
					case DIRECT_SHUT_DOWN	:g_GUI = GUI_SHUTWOWN_TIMER;break;
					case DIRECT_STARTUP  	:g_GUI = GUI_START_UP;g_GUI_Sel=0;break;
					case DIRECT_FEED		:g_GUI = GUI_FEED_TIMER;break;
					case DIRECT_SHUT_COMPELED:g_GUI = GUI_SHUTDOWN_COMPELE;break;
				}
				break;
				
			case GUI_ERROR_OCCUR:
				g_GUI_Sel=GUIDEMO_error(g_fatal_error);
				switch(g_GUI_Sel)
				{
					case 0: 
						if(3!=g_fatal_error)
						{
							if(g_current_mode!=SHUT)
							{
								g_shutdown_mode_counter=1200;
								g_current_mode=SHUT;
							}	
							
							g_GUI=GUI_SHUTWOWN;
							g_GUI_Sel=0;
						}else{
							g_GUI=GUI_MAIN;
							g_GUI_Sel=0;							
						}
						break;
					case GUI_ERROR_OCCUR	:g_GUI = GUI_ERROR_OCCUR;break;
					case DIRECT_SHUT_DOWN	:g_GUI = GUI_SHUTWOWN_TIMER;break;
					case DIRECT_STARTUP  	:g_GUI = GUI_START_UP;g_GUI_Sel=0;break;
					case DIRECT_FEED		:g_GUI = GUI_FEED_TIMER;break;
					case DIRECT_SHUT_COMPELED:g_GUI = GUI_SHUTDOWN_COMPELE;break;
				}
				
				break;
			default:
				break;
		}
	}
}

// ÿ100ms��ɨ��һ��EC11������״̬
void touch_task(void *p_arg)
{
	OS_ERR err;
	
	while(1)
	{
		EC11_BUT_Scan();
		OSTimeDlyHMSM(0,0,0,100,OS_OPT_TIME_PERIODIC,&err);
	}
}

extern u16 USART_RX_STA;
void wifi_task(void *p_arg)
{
	OS_ERR err;

	u8 ack_buf[128] = {0};
	u8 ack_len = 0;
	u8 ack_buf_len = 128;
	
	while(1) {
		if (USART_RX_STA&0X8000) {
			ack_len = run_cmd_from_usart(&U1_R_BUF[0], USART_RX_STA&0X7FFF, ack_buf, ack_buf_len);
            Recv_Data_Handle(Buf,ack_len);   // Just Send ACK

			USART_RX_STA=0;
			DMA_SetCurrDataCounter(DMA1_Channel5, U1_DMA_R_LEN);//DMAͨ����DMA����Ĵ�С
			DMA_Cmd(DMA1_Channel5, ENABLE);  //ʹ��USART1 RX DMA1 ��ָʾ��ͨ�� 

		}

		OSTimeDlyHMSM(0,0,0,10,OS_OPT_TIME_PERIODIC,&err);
	}
}

// ÿ��2�����һ���¶���Ϣ
// �ú����е��¶���ϢӦȫ����Ϊ���¶Ȱ���ȡ
void temp_watch_task(void *p_arg)
{
	int debug_temp=1;
	OS_ERR err;
//	u8 pid_counter = 0;
	
#if FOR_DEBUG_USE
	int tmpval  = 100;
	int testcnt = 0;
#endif
		
	while(1)
	{
		LED0 = !LED0;
		
		OSTimeDlyHMSM(0,0,0,500,OS_OPT_TIME_PERIODIC,&err);
		OSTimeDlyHMSM(0,0,0,500,OS_OPT_TIME_PERIODIC,&err);
		OSTimeDlyHMSM(0,0,0,500,OS_OPT_TIME_PERIODIC,&err);
		OSTimeDlyHMSM(0,0,0,500,OS_OPT_TIME_PERIODIC,&err);
		
		//printf("recv_len = %d, recv_times = %d\n", g_ota_one_pg_recv_len, g_ota_pg_numid);
		
		// ������ȫ�ֱ���
#if 0
		g_temp_val_new.temp1 = debug_temp % 501;
		g_temp_val_new.temp2 = debug_temp % 501;
		g_temp_val_new.temp3 = debug_temp % 501;
		g_temp_val_new.temp4 = debug_temp % 501;
		g_temp_val_new.temp5 = debug_temp % 501;
		debug_temp++;
		OSTimeDlyHMSM(0,0,0,500,OS_OPT_TIME_PERIODIC,&err);
	
#else
/*		g_temp_val_new.temp1 = (int)Get_Temperature(ADC_Channel_6);
		g_temp_val_new.temp2 = (int)Get_Temperature(ADC_Channel_5);
		g_temp_val_new.temp3 = (int)Get_Temperature(ADC_Channel_4);
		g_temp_val_new.temp4 = (int)Get_Temperature(ADC_Channel_3);
*/
		g_temp_val_new.temp5 = (int)Get_Temperature(ADC_Channel_2);

		
		g_temp_val_new.temp1 = Smoke_Mult*10; //P_out;  //  *******************************************  PID display  *******************
		g_temp_val_new.temp2 = g_time_remain; //I_out; //g_target_temp_val; //I_out;
		g_temp_val_new.temp3 = Auger_On/10;  //PID_dt;
		g_temp_val_new.temp4 = I_out; //Auger_Off/10;	
		
#endif
/*
		if (0 == pid_counter) {
			PID_dt++;
			S1_Counter_TempAvg++;
		}
		
#if	FOR_PID_USE
		PID_Ctr(g_temp_val_new.temp5, g_set_temp);
		// �ú�����õ���2��ֵ:Auger_On, Auger_Off,�ֱ��ʾ����ȿ�Auger_On�룬�ٹر�Auger_Off��
#endif		
		OSTimeDlyHMSM(0,0,0,100,OS_OPT_TIME_PERIODIC,&err);//��ʱ500ms
		
		pid_counter++;
		if (10 == pid_counter) {
			pid_counter = 0;
		} */
	}
}
		int g_sensor_error_continue=0;
		int g_sensor_error_mode_counter=0;
		int g_flame_error_continue=0;
		int g_flame_error_mode_counter=0;
		int shut_down_continue=0;
#if 0// for debug
int cnt_error = 0;
#endif
void counter_process()
{
#if 0// for debug
	if (cnt_error != 20) {
		cnt_error++;
	}
	
	if (cnt_error == 19) {
		g_fatal_error =1;//flame err ����
		g_flame_error_mode_counter=1200;
		g_current_mode = FLAME_ERROR;
	}
#endif
		
	switch(g_current_mode)
	{
		case STARTUP:
			if (g_startup_mode_counter > 0) {
				g_startup_mode_counter--;
			}
			break;
		case RUN:
			if (59 == g_run_mode_counter_sec) {// running timing
				g_run_mode_counter_sec = 0;
				if (59 == g_run_mode_counter_mins) {
					g_run_mode_counter_mins = 0;
					g_run_mode_counter_hour++;
				} else {
					g_run_mode_counter_mins++;
				}
			} else {
				g_run_mode_counter_sec++;
			}
			break;
		case FEED://feed
			if (g_feed_mode_counter > 0) {
				g_feed_mode_counter--;
			}
			break;
		case SHUT://shutdown
			if (g_shutdown_mode_counter > 0) {
				g_shutdown_mode_counter--;
			}
			break;
		case SENSOR_ERROR://shutdown
			if (g_sensor_error_mode_counter > 0) {
				g_sensor_error_mode_counter--;
			}
			break;
		case FLAME_ERROR://shutdown
			if (g_flame_error_mode_counter > 0) {
				g_flame_error_mode_counter--;
			}
			break;
	}
}
	
// ����ģʽ
void run_task(void *p_arg)
{
	OS_ERR err;
	int cur_temp=0;
//	int last_temp=0;
	int total_sec=0;
//	int sec_T1=0;
//	int sec_T2=0;
	int sec_ON=0;
	int sec_OFF=0;
	int sec_ON_left=0;
	int sec_OFF_left=0;
	int temp=0;
	
	int flame_run_first_in=1;
	int flame_run_sec=0;
	int flame_run_hot_status=0;
	int cnt_1s=0;
	
	u8 pid_counter = 0;
	
	g_startup_mode_last = 0;
	g_run_mode_last = 0;
	g_feed_mode_last = 0;
	g_shutdown_mode_last = 0;
	

	MOT = Control_OFF;		
	HOT = Control_OFF;
	FAN = Control_OFF;
	while(1)
	{
		
		if(RUN != g_current_mode) g_flame_update_status = 1;//	 Set temperature changes, update flame
		
		cnt_1s++;
		if(cnt_1s>=10)//1s timer
		{
			cnt_1s=0;
			switch(g_current_mode)
			{
				case IDLE://idle
					IDLE_Mode();
					break;
				case STARTUP://startupģʽ
					if((g_temp_val_new.temp5 > 600))
					{
						g_fatal_error = 3;//over temp ���±���
						g_current_mode = VOER_ERROR;
					}	
					Startup_Mode(360 - g_startup_mode_counter);
					/*if (g_startup_mode_counter > 0) 
					{
						g_startup_mode_counter--;
					}*/
					break;
				case RUN://runģʽ
					if((g_temp_val_new.temp5 > 600))
					{
						g_fatal_error =3;//over temp ���±���
						g_current_mode = VOER_ERROR;
					}
					if((g_temp5_error == 1))
					{
						g_fatal_error = 2;//sensor error RTD ����
						g_current_mode = SENSOR_ERROR;
					}
					/*if (59 == g_run_mode_counter_sec) 
					{// running timing
							g_run_mode_counter_sec = 0;
					if (59 == g_run_mode_counter_mins) 
					{
							g_run_mode_counter_mins = 0;
							g_run_mode_counter_hour++;
					} else {
							g_run_mode_counter_mins++;
					}
					} else {
						g_run_mode_counter_sec++;
					}*/
					
					total_sec=(g_run_mode_counter_hour*3600 + g_run_mode_counter_mins* 60 + g_run_mode_counter_sec);
				
					if((g_flame_update_status == 1)||(RUN != g_current_mode)) //	 Set temperature changes, update flame
					{
						g_flame_update_status=0;
						flame_run_sec=0;
						flame_run_first_in=1;
					
						Run_Mode_HOT_ON(0);//Turn off the hot
						flame_run_hot_status=0;
			
					}
					if(((g_set_temp<190)||(g_set_temp>250))&&(g_temp_val_new.temp5 < 135))//	Check for Flame-Out condition.
					{
						flame_run_sec++;
						if((flame_run_sec > 15*60) && (flame_run_first_in!=0))//Record temp at 15 min
						{
							cur_temp  = g_temp_val_new.temp5;
							flame_run_sec=0;
							flame_run_first_in=0;
						}else if((flame_run_sec> 3*60) && (!flame_run_first_in) )//Compare the temperature every 3 minutes
						{
							if(g_temp_val_new.temp5>cur_temp) 
							{
									cur_temp  = g_temp_val_new.temp5;
									flame_run_sec=0;
							}else {
								g_fatal_error =1;//flame err ����
								g_flame_error_mode_counter=1200;
								g_current_mode = FLAME_ERROR;
							}
						}
					}else if((g_set_temp>=190)&&(g_set_temp<=200)&&(g_set_temp-g_temp_val_new.temp5>=10))//�²��10��
					{
						flame_run_sec++;
						if(flame_run_first_in == 1)
						{
							cur_temp  = g_temp_val_new.temp5;
							flame_run_first_in=0;
						}
					
						if(flame_run_sec>18*60)// 18*60Flame-out error and shutdown after 18 min
						{	
								g_fatal_error =1;//flame err ����
								g_flame_error_mode_counter=1200;
								g_current_mode = FLAME_ERROR;
						}else if(flame_run_sec>12*60)  // 12*60Start sampling for increase in temp again, 12 min
						{
								if(g_temp_val_new.temp5>cur_temp) 
								{
									flame_run_sec=0;
									flame_run_first_in = 1;
								}
						}else if(flame_run_sec>9*60) // 9*60 turn off hotrod after 6 min
						{
							Run_Mode_HOT_ON(0);//Turn off the hot
							flame_run_hot_status=0;
						}else if((flame_run_sec>3*60)&&(flame_run_hot_status == 0))//3*60
						{
							if(g_temp_val_new.temp5>cur_temp) 
								flame_run_sec=0;
							else
							{
									Run_Mode_HOT_ON(1);//Turn on the hot
									flame_run_hot_status=1;
							}
							flame_run_first_in = 1;
						}
					}else if((g_set_temp>200)&&(g_set_temp<=250)&&(g_set_temp-g_temp_val_new.temp5>=20))//�²��20��
					{
						flame_run_sec++;
						if(flame_run_first_in == 1)
						{
							cur_temp  = g_temp_val_new.temp5;
							flame_run_first_in=0;
						}
						if(flame_run_sec> 18*60)// Flame-out error and shutdown after 18 min
						{	
							g_fatal_error =1;//flame err ����
							g_flame_error_mode_counter=1200;
							g_current_mode = FLAME_ERROR;
						}else if(flame_run_sec> 12*60)  // Start sampling for increase in temp again, 12 min
						{
							if(g_temp_val_new.temp5>cur_temp) 
							{
									flame_run_sec=0;
									flame_run_first_in = 1;
							}
						}else if(flame_run_sec> 9*60) // turn off hotrod after 6 min
						{
							Run_Mode_HOT_ON(0);//Turn off the hot
							flame_run_hot_status=0;
						}else if((flame_run_sec> 3*60)&&(flame_run_hot_status == 0))
						{
							if(g_temp_val_new.temp5>cur_temp) 
									flame_run_sec=0;
							else
							{
									Run_Mode_HOT_ON(1);//Turn on the hot
									flame_run_hot_status=1;
							}
							flame_run_first_in = 1;
						}
					}else
					{
						flame_run_sec=0;
						flame_run_first_in=1;
					
						Run_Mode_HOT_ON(0);//Turn off the hot
						flame_run_hot_status=0;
					}
			
					break;
				case FEED://feed
					Feed_Mode(420 - g_feed_mode_counter);// 7 minutes
					/*if (g_feed_mode_counter > 0) 
					{
						g_feed_mode_counter--;
					}*/
					break;
				case SHUT://shutdown
					Shutdown_Mode(1200 - g_shutdown_mode_counter);// 20 minutes1200
					/*if (g_shutdown_mode_counter > 0) {
						g_shutdown_mode_counter--;
					}*/
					break;
				case SENSOR_ERROR://shutdown
					Sensor_error_Mode(1200 - g_sensor_error_mode_counter);// 20 minutes1200
					/*if (g_sensor_error_mode_counter > 0) {
						g_sensor_error_mode_counter--;
					}*/
					break;
				case VOER_ERROR://shutdown
					Run_Mode_FLAME_OVER();
					break;
				case FLAME_ERROR://shutdown
					Flame_error_Mode(1200 - g_flame_error_mode_counter);// 20 minutes1200
					if (g_flame_error_mode_counter > 0) {
						g_flame_error_mode_counter--;
					}
					break;
			}
		}
	
	if (RUN == g_current_mode)
	{
			if(sec_ON_left <= Auger_On)
				Run_Mode_MOT_ON(1);
			else
				Run_Mode_MOT_ON(0);
			
			sec_ON_left++;
			
			if(sec_ON_left >= (Auger_On + Auger_Off))
				sec_ON_left = 0;
			
			g_time_remain = sec_ON_left/10.0;       // 											************  TESTING ONLY  *******************
						
	/*			
			if((sec_ON != Auger_On) || (sec_OFF != Auger_Off))
			{//�����ر�ʱ��仯
				sec_ON_left = Auger_On;
				sec_OFF_left = Auger_Off;
				
				sec_ON = Auger_On;
				sec_OFF= Auger_Off;
			}
			
			if(Auger_On!=0)
			{
				if(sec_ON_left>0)
				{
					Run_Mode_MOT_ON(1);
					sec_ON_left--;
				}else if(sec_OFF_left>0)
				{
					Run_Mode_MOT_ON(0);
					sec_OFF_left--;
				}
				
				if((sec_ON_left==0) && (sec_OFF_left==0))
				{
					sec_ON_left=sec_ON;
					sec_OFF_left=sec_OFF;
				}
				 g_time_remain = (sec_ON_left + sec_OFF_left)/10;
			}
			else Run_Mode_MOT_ON(1);//The motor continues to keep going
		}
	*/	
/*
				if(sec_ON_left < Auger_On)  //  ---------------------------TESTING   MIN FEED 6-46  RATE
					Run_Mode_MOT_ON(1);
				else
				{
					if(sec_OFF_left >= 460)						// Turn off Auger after additional 6 seconds OFF time  ...change to use Auger_On_Min???
					{
						sec_OFF_left = 0;
						Run_Mode_MOT_ON(0);
					}
				else if(sec_OFF_left >=400)				//	Turn on Auger after 40 seconds OFF time (min feed rate)
					Run_Mode_MOT_ON(1);
					else
						Run_Mode_MOT_ON(0);
					
						sec_OFF_left++;
				}
					
				sec_ON_left++;
				
				if(sec_ON_left >= (Auger_On + Auger_Off))
				{
					sec_ON_left = 0;
					sec_OFF_left = 0;
				}
*/
			

			if (0 == pid_counter) 
			{
				PID_dt++;
				S1_Counter_TempAvg++;
			}
		
			pid_counter++;
				
			if (10 == pid_counter) 
				pid_counter = 0;
		
		
			PID_Ctr(g_temp_val_new.temp5, g_set_temp);		
		
/*				
				if(sec_ON_left < Auger_On)
					{
						Run_Mode_MOT_ON(1);
						sec_ON_left++;
					}
				else if(sec_OFF_left < Auger_Off)
					{
						Run_Mode_MOT_ON(0);
						sec_OFF_left++;
					}
				
				if((sec_ON_left + sec_OFF_left) >= (Auger_On + Auger_Off))
				{
					sec_ON_left = 0;
					sec_OFF_left = 0;
				}
				 g_time_remain = (sec_ON_left + sec_OFF_left)/10;
*/
		}	
		
		OSTimeDlyHMSM(0,0,0,50,OS_OPT_TIME_PERIODIC,&err);
		OSTimeDlyHMSM(0,0,0,50,OS_OPT_TIME_PERIODIC,&err);
	}
}

