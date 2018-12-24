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

//START任务
//设置任务的优先级
#define START_TASK_PRIO				3
//任务堆栈大小 
#define START_STK_SIZE			  256
//任务控制块
OS_TCB StartTaskTCB;
//任务堆栈	
CPU_STK START_TASK_STK[START_STK_SIZE];
//任务函数
void start_task(void *p_arg);

//TOUCH任务
//设置任务优先级
#define TOUCH_TASK_PRIO				4
//任务堆栈大小
#define TOUCH_STK_SIZE				128
//任务控制块
OS_TCB TouchTaskTCB;
//任务堆栈
CPU_STK TOUCH_TASK_STK[TOUCH_STK_SIZE];
//touch任务
void touch_task(void *p_arg);

//TEMP任务
//设置任务优先级
#define TEMP_TASK_PRIO 				5
//任务堆栈大小
#define TEMP_STK_SIZE				64
//任务控制块
OS_TCB TempTaskTCB;
//任务堆栈
CPU_STK TEMP_TASK_STK[TEMP_STK_SIZE];
//led0任务
void temp_watch_task(void *p_arg);

//EMWINDEMO任务
//设置任务优先级
#define EMWINDEMO_TASK_PRIO			6
//任务堆栈大小
#define EMWINDEMO_STK_SIZE			2048
//任务控制块
OS_TCB EmwindemoTaskTCB;
//任务堆栈
CPU_STK EMWINDEMO_TASK_STK[EMWINDEMO_STK_SIZE];
//emwindemo_task任务
void emwindemo_task(void *p_arg);

//Run任务
//设置任务优先级
#define RUN_TASK_PRIO 				7
//任务堆栈大小
#define RUN_STK_SIZE				64
//任务控制块
OS_TCB RUNTaskTCB;
//任务堆栈
CPU_STK RUN_TASK_STK[RUN_STK_SIZE];
//led0任务
void run_task(void *p_arg);


////////////////////////////////////////////////////////
OS_TMR 	tmr1;		//定时器1
OS_TMR	tmr2;		//定时器2
void tmr1_callback(void *p_tmr, void *p_arg); 	//定时器1回调函数
void tmr2_callback(void *p_tmr, void *p_arg);	//定时器2回调函数

u32 Check_Sys=0;

// Target温度值
extern const short baseTempMax[];
extern EVENT_VAL g_event_val_new;// 当前状态
extern EVENT_VAL g_event_val_last;// 上一次状态
extern TEMP_VAL g_temp_val_new;// 当前温度
extern TEMP_VAL g_temp_val_last;// 上一次温度

//
// User Interface
//

// 是否发生严重错误
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

// 几种模式的开始和结束标志
// 0-end 1-start
extern int g_startup_mode;
extern int g_run_mode;
extern int g_feed_mode;
extern int g_shutdown_mode;

int g_startup_mode_last;
int g_run_mode_last;
int g_feed_mode_last;
int g_shutdown_mode_last;

// 几种模式的计时器
extern u16 g_startup_mode_counter;
extern u16 g_run_mode_counter_mins;// minutes
extern u16 g_run_mode_counter_sec;// seconds
extern u16 g_run_mode_counter_hour;// hours
extern u16 g_feed_mode_counter;
extern u16 g_shutdown_mode_counter;


// RUN模式倒计时，时间设置以及使能
// 单位seconds(默认480s)
extern int g_run_timer_setting;

// 0-华氏度(默认) 1摄氏度
// g_temp_val_new.temp_unit

// 右侧温度区(对应温度棒1234)
// g_temp_val_new.temp1
// g_temp_val_new.temp2
// g_temp_val_new.temp3
// g_temp_val_new.temp4

// RUN界面最大字体的温度
// g_temp_val_new.temp5

// 温度棒是否读异常
// 0-OK(default) 1-温度棒异常
extern int g_temp1_error;
extern int g_temp2_error;
extern int g_temp3_error;
extern int g_temp4_error;
extern int g_temp5_error;

// 表示是否初始化
// 0-未初始化(default), 1-已初始化
extern int g_factory_reseted;

// 进入startup或者run标志
// 0-run, 1-startup mode(default)
extern int g_startup_enable;

// smoke进度条百分比，默认值为50(%)
extern int g_smoke_val_percent;
extern int g_target_temp_val;
extern int g_set_temp;

extern int g_temp_center;

// EMWIN界面入口
extern void GUIDEMO_AfterLogo(void);
extern void GUIDEMO_UpdateTemp(int *temp_val);

extern int g_time_remain;

u8 g_ota_runing = 0;
u16 g_ota_recv_sum = 0;
u16 g_ota_one_pg_recv_len = 0;
u16 g_ota_pg_numid = 0;

#if 1
// LOGO图片转大数组
extern const unsigned char gImage_camp[1308];

// 获取每个像素点的2字节颜色信息
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

// 写入一次起点终点坐标，然后写入所有数据(颜色信息)
void image_show(u16 xsta,u16 ysta,u16 xend,u16 yend,u8 scan,u8 *p)
{  
	u32 i;
	u32 len=0;
	
	BlockWrite(xsta,xend,ysta,yend);
	
	len=(xend-xsta+1)*(yend-ysta+1);	//写入的数据长度
	for(i=0;i<len;i++)
	{
		*(__IO u16 *) (Bank1_LCD_D) = image_getcolor(scan&(1<<4),p);
		p+=2;
	}	    					  	    
} 

void image_display(u16 x,u16 y,u8 * imgx)
{
	HEADCOLOR *imginfo;
 	u8 ifosize=sizeof(HEADCOLOR);//得到HEADCOLOR结构体的大小
	imginfo=(HEADCOLOR*)imgx;
 	image_show(x,y,x+imginfo->w-1,y+imginfo->h-1,imginfo->scan,imgx+ifosize);
}

// 在指定位置开始显示一张图片
void disp_img(u16 x, u16 y)
{
	image_display(x,y,(u8*)gImage_camp);//在指定地址显示图片
}
#endif

int main(void)
{
	OS_ERR err;
	CPU_SR_ALLOC();	
	
	delay_init();	    	//延时函数初始化	  
	NVIC_PriorityGroupConfig(NVIC_PriorityGroup_2); 	//设置NVIC中断分组2:2位抢占优先级，2位响应优先级
#ifdef SRAM_MEMDEV
	FSMC_SRAM_Init();		//初始化SRAM
#endif
 	LED_Init();			    //LED端口初始化
	Lcd_Initialize();
	
	// 横屏和扫描方式设置
	LCD_Display_Dir(HORIZON_DISPLAY);

	EC11_EXTI_Init();//EC11编码器初始化
	my_mem_init(SRAMIN); 	//初始化内部内存池
	
	Adc_Init();
	Control_Init();

	//PID_Init();
	
	TIM3_Int_Init(9999, 7199);
	uart_init(115200);	 	//串口初始化为115200
	Usart_DMA_Init();
	
	OSInit(&err);		//初始化UCOSIII
	OS_CRITICAL_ENTER();//进入临界区
	//创建开始任务
	OSTaskCreate((OS_TCB 	* )&StartTaskTCB,		//任务控制块
				 (CPU_CHAR	* )"start task", 		//任务名字
                 (OS_TASK_PTR )start_task, 			//任务函数
                 (void		* )0,					//传递给任务函数的参数
                 (OS_PRIO	  )START_TASK_PRIO,     //任务优先级
                 (CPU_STK   * )&START_TASK_STK[0],	//任务堆栈基地址
                 (CPU_STK_SIZE)START_STK_SIZE/10,	//任务堆栈深度限位
                 (CPU_STK_SIZE)START_STK_SIZE,		//任务堆栈大小
                 (OS_MSG_QTY  )0,					//任务内部消息队列能够接收的最大消息数目,为0时禁止接收消息
                 (OS_TICK	  )0,					//当使能时间片轮转时的时间片长度，为0时为默认长度，
                 (void   	* )0,					//用户补充的存储区
                 (OS_OPT      )OS_OPT_TASK_STK_CHK|OS_OPT_TASK_STK_CLR, //任务选项
                 (OS_ERR 	* )&err);				//存放该函数错误时的返回值
	OS_CRITICAL_EXIT();	//退出临界区	 
	OSStart(&err);  //开启UCOSIII
							 
}

//开始任务函数
void start_task(void *p_arg)
{
	OS_ERR err;
	CPU_SR_ALLOC();
	p_arg = p_arg;

	CPU_Init();
#if OS_CFG_STAT_TASK_EN > 0u
   OSStatTaskCPUUsageInit(&err);  	//统计任务                
#endif
	
#ifdef CPU_CFG_INT_DIS_MEAS_EN		//如果使能了测量中断关闭时间
    CPU_IntDisMeasMaxCurReset();	
#endif

#if	OS_CFG_SCHED_ROUND_ROBIN_EN  //当使用时间片轮转的时候
	 //使能时间片轮转调度功能,时间片长度为1个系统时钟节拍，既1*5=5ms
	OSSchedRoundRobinCfg(DEF_ENABLED,1,&err);  
#endif		
	RCC_AHBPeriphClockCmd(RCC_AHBPeriph_CRC,ENABLE);//开启CRC时钟
#if 1
	WM_SetCreateFlags(WM_CF_MEMDEV);
	GUI_Init();  			//STemWin初始化
#if 0
	//创建定时器1
	OSTmrCreate((OS_TMR		*)&tmr1,		//定时器1
                (CPU_CHAR	*)"tmr1",		//定时器名字
                (OS_TICK	 )0,			//0
                (OS_TICK	 )10,          //100*10=1000ms
                (OS_OPT		 )OS_OPT_TMR_PERIODIC, //周期模式
                (OS_TMR_CALLBACK_PTR)tmr1_callback,//定时器1回调函数
                (void	    *)0,			//参数为0
                (OS_ERR	    *)&err);		//返回的错误码
				
				
	//创建定时器2
	OSTmrCreate((OS_TMR		*)&tmr2,		
                (CPU_CHAR	*)"tmr2",		
                (OS_TICK	 )200,			//200*10=2000ms	
                (OS_TICK	 )0,   					
                (OS_OPT		 )OS_OPT_TMR_ONE_SHOT, 	//单次定时器
                (OS_TMR_CALLBACK_PTR)tmr2_callback,	//定时器2回调函数
                (void	    *)0,			
                (OS_ERR	    *)&err);
	OSTmrStart(&tmr1,&err);	//开启定时器1
	OSTmrStart(&tmr2,&err);	//开启定时器2
#endif
	
	OS_CRITICAL_ENTER();	//进入临界区
	// STemWin UI任务	
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
	// EC11按键任务
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
	// 温度读取任务
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
	// RUN任务
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
								 
	OS_TaskSuspend((OS_TCB*)&StartTaskTCB,&err);		//挂起开始任务			 
	OS_CRITICAL_EXIT();	//退出临界区
}

//定时器1的回调函数 100ms
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
//	LCD_ShowxNum(62,111,tmr1_num,3,16,0x80); //显示定时器1的执行次数
//	LCD_Fill(6,131,114,313,lcd_discolor[tmr1_num%14]);//填充区域
	tmr1_num++;		//定时器1执行次数加1*/
//	Check_Sys++;
}

//定时器2的回调函数
void tmr2_callback(void *p_tmr,void *p_arg)
{
/*	static u8 tmr2_num = 0;
	tmr2_num++;		//定时器2执行次数加1
	LCD_ShowxNum(182,111,tmr2_num,3,16,0x80);  //显示定时器1执行次数
	LCD_Fill(126,131,233,313,lcd_discolor[tmr2_num%14]); //填充区域
	LED1 = ~LED1;
	printf("定时器2运行结束\r\n");*/
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

// EMWIN界面入口
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
	// 该处温度应改为由温度棒读出
	// 如果温度棒没接或者读不出，请设置为0，温度会显示为"-"
	// 该处为第一次显示，后续在temp_watch_task中更新温度信息即可
	temp_val.temp1 = 0;
	temp_val.temp2 = 0;
	temp_val.temp3 = 0;
	temp_val.temp4 = 0;
	temp_val.temp5 = 0;
	
	// 默认值，可自行设置

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

// 每100ms，扫描一次EC11编码器状态
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
			DMA_SetCurrDataCounter(DMA1_Channel5, U1_DMA_R_LEN);//DMA通道的DMA缓存的大小
			DMA_Cmd(DMA1_Channel5, ENABLE);  //使能USART1 RX DMA1 所指示的通道 

		}

		OSTimeDlyHMSM(0,0,0,10,OS_OPT_TIME_PERIODIC,&err);
	}
}

// 每隔2秒更新一次温度信息
// 该函数中的温度信息应全部改为从温度棒读取
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
		
		// 保存至全局变量
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
		// 该函数会得到的2个值:Auger_On, Auger_Off,分别表示马达先开Auger_On秒，再关闭Auger_Off秒
#endif		
		OSTimeDlyHMSM(0,0,0,100,OS_OPT_TIME_PERIODIC,&err);//延时500ms
		
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
		g_fatal_error =1;//flame err 报警
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
	
// 运行模式
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
				case STARTUP://startup模式
					if((g_temp_val_new.temp5 > 600))
					{
						g_fatal_error = 3;//over temp 高温报警
						g_current_mode = VOER_ERROR;
					}	
					Startup_Mode(360 - g_startup_mode_counter);
					/*if (g_startup_mode_counter > 0) 
					{
						g_startup_mode_counter--;
					}*/
					break;
				case RUN://run模式
					if((g_temp_val_new.temp5 > 600))
					{
						g_fatal_error =3;//over temp 高温报警
						g_current_mode = VOER_ERROR;
					}
					if((g_temp5_error == 1))
					{
						g_fatal_error = 2;//sensor error RTD 报警
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
								g_fatal_error =1;//flame err 报警
								g_flame_error_mode_counter=1200;
								g_current_mode = FLAME_ERROR;
							}
						}
					}else if((g_set_temp>=190)&&(g_set_temp<=200)&&(g_set_temp-g_temp_val_new.temp5>=10))//温差超过10度
					{
						flame_run_sec++;
						if(flame_run_first_in == 1)
						{
							cur_temp  = g_temp_val_new.temp5;
							flame_run_first_in=0;
						}
					
						if(flame_run_sec>18*60)// 18*60Flame-out error and shutdown after 18 min
						{	
								g_fatal_error =1;//flame err 报警
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
					}else if((g_set_temp>200)&&(g_set_temp<=250)&&(g_set_temp-g_temp_val_new.temp5>=20))//温差超过20度
					{
						flame_run_sec++;
						if(flame_run_first_in == 1)
						{
							cur_temp  = g_temp_val_new.temp5;
							flame_run_first_in=0;
						}
						if(flame_run_sec> 18*60)// Flame-out error and shutdown after 18 min
						{	
							g_fatal_error =1;//flame err 报警
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
			{//开启关闭时间变化
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

