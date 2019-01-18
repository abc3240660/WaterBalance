#include "sys.h"
#include "usart.h"	 
#include "dma.h"
#include "common.h"
#include "control.h"
#include "timer.h"
#include "malloc.h"
#if SYSTEM_SUPPORT_OS
#include "includes.h"
#endif
#if 1
#pragma import(__use_no_semihosting)             
struct __FILE 
{ 
	int handle; 

}; 

FILE __stdout;       
_sys_exit(int x) 
{ 
	x = x; 
} 
int fputc(int ch, FILE *f)
{      
	while((USART1->SR&0X40)==0);
    USART1->DR = (u8) ch;      
	return ch;
}
#endif 

u8* USART_RX_BUF = NULL;
u8* USART_RX_BUF_BAK = NULL;
u32 USART_RX_STA = 0;

extern u8 g_ota_sta;
extern u32 g_ota_recv_sum;
extern u16 g_ota_pg_numid;

extern u16 g_ota_pg_nums;
extern u32 g_ota_bin_size;

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
u8 snd_buf[128] = {0};

u8 snd_buf[128] = {0};

u8 g_ota_bin_md5[SSL_MAX_LEN] = {0};
u8 g_ota_package_md5[SSL_MAX_LEN] = {0};

void UART1_SendData(u8 *data, u16 num);

void uart_init(u32 bound){
    GPIO_InitTypeDef GPIO_InitStructure;
	USART_InitTypeDef USART_InitStructure;
	NVIC_InitTypeDef NVIC_InitStructure;
	 
	RCC_APB2PeriphClockCmd(RCC_APB2Periph_USART1|RCC_APB2Periph_GPIOA, ENABLE);
 	USART_DeInit(USART1);
	//USART1_TX   PA.9
    GPIO_InitStructure.GPIO_Pin = GPIO_Pin_9; //PA.9
    GPIO_InitStructure.GPIO_Speed = GPIO_Speed_50MHz;
    GPIO_InitStructure.GPIO_Mode = GPIO_Mode_AF_PP;
    GPIO_Init(GPIOA, &GPIO_InitStructure);
   
    //USART1_RX	  PA.10
    GPIO_InitStructure.GPIO_Pin = GPIO_Pin_10;
    GPIO_InitStructure.GPIO_Mode = GPIO_Mode_IN_FLOATING;
    GPIO_Init(GPIOA, &GPIO_InitStructure);

    NVIC_InitStructure.NVIC_IRQChannel = USART1_IRQn;
	NVIC_InitStructure.NVIC_IRQChannelPreemptionPriority=3 ;
	NVIC_InitStructure.NVIC_IRQChannelSubPriority = 3;
	NVIC_InitStructure.NVIC_IRQChannelCmd = ENABLE;
	NVIC_Init(&NVIC_InitStructure);
  
	USART_InitStructure.USART_BaudRate = bound;
	USART_InitStructure.USART_WordLength = USART_WordLength_8b;
	USART_InitStructure.USART_StopBits = USART_StopBits_1;
	USART_InitStructure.USART_Parity = USART_Parity_No;
	USART_InitStructure.USART_HardwareFlowControl = USART_HardwareFlowControl_None;
	USART_InitStructure.USART_Mode = USART_Mode_Rx | USART_Mode_Tx;

    USART_Init(USART1, &USART_InitStructure);
    //USART_ITConfig(USART1, USART_IT_RXNE, ENABLE);
		USART_ITConfig(USART1, USART_IT_IDLE, ENABLE);
    USART_Cmd(USART1, ENABLE);
		
		while(USART_GetFlagStatus(USART1,USART_FLAG_TC) == RESET);
		
		// 100ms
		TIM6_Int_Init(9999, 7199);
		
		TIM_Cmd(TIM6, DISABLE);
		
		USART_RX_STA=0;
		
		USART_RX_BUF     = (u8*)mymalloc(SRAMEX, USART_MAX_RECV_LEN);
		USART_RX_BUF_BAK = (u8*)mymalloc(SRAMEX, USART_MAX_RECV_LEN);
}

void run_cmd_from_usart(u8 *data, u16 num)
{
	u8 snd_len = 0;
	
	if (data[0] != 0xfe) {
		return;
	}

	snd_len = num;
	memset(snd_buf, 0, 128);

	switch (data[1])
	{
		// FE 01 01 03 05 00 FF -> set TEMP to 350F
		// FE 01 03 01 06 00 FF -> set TEMP to LOW SMOKE(160F)
		// FE 01 04 01 06 00 FF -> set TEMP to HIGH SMOKE(220F)
		case 0x01:
			g_temp_val_new.target_val = data[3]*100 + data[4]*10 + data[5];
			if (g_temp_val_new.target_val < EXT_MAX_SS_MIN) {
				g_temp_val_new.target_val = EXT_MAX_SS_MIN;
			}
			if (g_temp_val_new.target_val > EXT_MAX_SS_MAX) {
				g_temp_val_new.target_val = EXT_MAX_SS_MAX;
			}
			// TBD: if during the UI with temp, please change the display value 
			g_temp_val_new.target_update = 1;
			memcpy(snd_buf, data, num);
			break;
		// FE 02 VAL(1B) FF -> set Smoke
		// VAL: 1~10
		case 0x02:
			g_temp_val_new.target_smoke = data[2];
			g_temp_val_new.target_update = 1;
			memcpy(snd_buf, data, num);
			break;
		// FE 03 VAL(1B) FF -> set Smoke
		// VAL: 0-SHUT, 1-Startup, 2-Feed
		case 0x03:
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
			memcpy(snd_buf, data, num);
			break;
		// 3B Temp + 3B runtime + 1B SmokeLevel for step1
		// 3B Temp + 3B runtime + 1B SmokeLevel for step2
		// 3B Temp + 3B runtime + 1B SmokeLevel for step3
		case 0x04:
			snd_buf[0] = 0xFE;
			snd_buf[1] = 0x04;
			snd_buf[2] = 0x01;
			snd_buf[3] = 0xFF;
			snd_len = 4;
			break;
		// Need ACK:Send Current Wifi Icon Level to APP
		case 0x05:
			snd_buf[0] = 0xFE;
			snd_buf[1] = 0x05;

			// target temp
			// 001-LOW Smoke, 002-HIGH Smoke
			snd_buf[2] = 0x05;
			snd_buf[3] = 0x05;
			snd_buf[4] = 0x05;

			// running time(max 999 minutes)
			// 001-LOW Smoke, 002-HIGH Smoke
			snd_buf[5] = 0x05;
			snd_buf[6] = 0x05;
			snd_buf[7] = 0x05;

			snd_buf[8] = 0xFF;
			snd_len = 9;
			break;
		case 0x06:
			g_feed_mode = 0;
			g_shutdown_mode = 0;
			g_startup_mode = 0;
			g_run_mode = 0;
			
			g_temp_val_new.temp_unit = 0;
			memcpy(snd_buf, data, num);
			break;
		case 0x0b:
			snd_buf[0] = 0xFE;
			snd_buf[1] = 0x0B;
		
			// Grill ACT(TEMP)
			snd_buf[1+1] = g_temp_val_new.temp5/100;
			snd_buf[1+2] = (g_temp_val_new.temp5%100)/10;
			snd_buf[1+3] = g_temp_val_new.temp5%10;
		
			// Grill SET(TEMP)
			snd_buf[1+4] = g_temp_val_new.target_val/100;
			snd_buf[1+5] = (g_temp_val_new.target_val%100)/10;
			snd_buf[1+6] = g_temp_val_new.target_val%10;
		
			// Reserved
			snd_buf[1+7] = 0;
			snd_buf[1+8] = 0;
			snd_buf[1+9] = 0;

			// Auger On time(min)
			snd_buf[1+10] = 0;
			snd_buf[1+11] = 0;
			snd_buf[1+12] = 0;
		
			// Probe1 temp
			snd_buf[1+13] = g_temp_val_new.temp1/100;
			snd_buf[1+14] = (g_temp_val_new.temp1%100)/10;
			snd_buf[1+15] = g_temp_val_new.temp1%10;

			// Probe2 temp
			snd_buf[1+16] = g_temp_val_new.temp2/100;
			snd_buf[1+17] = (g_temp_val_new.temp2%100)/10;
			snd_buf[1+18] = g_temp_val_new.temp2%10;
			
			// Probe3 temp
			snd_buf[1+19] = g_temp_val_new.temp3/100;
			snd_buf[1+20] = (g_temp_val_new.temp3%100)/10;
			snd_buf[1+21] = g_temp_val_new.temp3%10;
			
			// Probe4 temp
			snd_buf[1+22] = g_temp_val_new.temp4/100;
			snd_buf[1+23] = (g_temp_val_new.temp4%100)/10;
			snd_buf[1+24] = g_temp_val_new.temp4%10;
			
			// Reserved
			snd_buf[1+25] = 0;
			
			// Somke Level
			snd_buf[1+26] = g_temp_val_new.target_smoke;
			
			// Mode: 0-Startup, 1-IDLE, 2-RUN, 3-FEED, 4-SHUT
			// TBD
			// snd_buf[1+27] = g_temp_val_new.target_smoke;
			
			// F/C
			snd_buf[1+28] = (g_temp_val_new.temp_unit == 0)?1:2;//1 F 2 C
			
			// RTD Fault: 0-OK, 1-ERROR
			// TBD
			// snd_buf[1+29] = (g_temp1_error == 0)?1:2;
			
			// Over Temp: 0-OK, 1-ERROR
			// TBD
			// snd_buf[1+30] = (g_fatal_error == 1)?1:2;//over temp
			
			// Flame Err: 0-OK, 1-ERROR
			// TBD
			// snd_buf[1+31] = 0;

			// Err4 Reserved: 0-OK, 1-ERROR
			snd_buf[1+32] = 0;
			// Err5 Reserved: 0-OK, 1-ERROR
			snd_buf[1+33] = 0;
			// Err6 Reserved: 0-OK, 1-ERROR
			snd_buf[1+34] = 0;
			
			// Probe1 Sta: 0-No Probe, 1-Plug in
			snd_buf[1+35] = (g_temp1_error == 0)?1:0;
			// Probe2 Sta: 0-No Probe, 1-Plug in
			snd_buf[1+36] = (g_temp2_error == 0)?1:0;
			// Probe3 Sta: 0-No Probe, 1-Plug in
			snd_buf[1+37] = (g_temp3_error == 0)?1:0;
			// Probe4 Sta: 0-No Probe, 1-Plug in
			snd_buf[1+38] = (g_temp4_error == 0)?1:0;

			// Ctr_Mot: 0-Off, 1-Auger On
			snd_buf[1+39] = (MOT_I == Control_ON)?1:0;//mod
			// Ctr_Hot: 0-Off, 1-Hot Rod On
			snd_buf[1+40] = (HOT_I == Control_ON)?1:0;//hot
			// Ctr_Fan: 0-Off, 1-Fan On
			snd_buf[1+41] = (FAN_I == Control_ON)?1:0;//fun

			// Recipe Step: 0-No Recipe, 1~n-Step 1~N
			snd_buf[1+42] = 0;

			// The running time of this step(unit: minutes)
			snd_buf[1+43] = 0;
			snd_buf[1+44] = 0;
			snd_buf[1+45] = 0;

			// 28B User ID Reserved
			// snd_buf[1+46] = 0;
			// snd_buf[1+73] = 0;

			snd_buf[75] = 0xFF;
			
			snd_len = 76;
			break;
		// Night mode
		// VAL1: 0-Day, 1-Night
		case 0x11:
			break;
		// F/C
		// VAL1: 0-F, 1-C
		case 0x12:
			break;
		// WIFI Status
		// FE 23 VAL1(1B) VAL2(1B) FF
		// VAL1: Wifi Icon Always On or Flash
		//       0-WIFI Flash(Not Internet Connected), 1-WIFI Alayws On(Connected) 3-WIFI-Flash(Not Server Connected)
		// VAL2: Wifi Icon Level
		//       0-Hide
		//       1~4-Level 1~4
		// No Need to ACK
		case 0x23:
			break;
		// BLE Status
		// FE 24 VAL1(1B) FF
		// VAL1: 0-No Connectiton, 1-Connected
		// No Need to ACK
		case 0x24:
			break;

		// Grill PIN
		// VAL1: 6B ASCII
		case 0x30:
			break;
		// NETWORK AP
		// VAL1: 1~13B ASCII
		case 0x31:
			break;
		// BLE AP
		// VAL1: 1~13B ASCII
		case 0x32:
			break;
		// FIRMWARE
		// FE 34 00 FF -> set Smoke
		case 0x34:
			// FE 34 1~13B ASCII FF
			break;

		case 0x40:// OTA
			switch(data[2])
			{
				// FE 40 01 01 FF
				case 0x01:// OTA Start
					// TBD: Clost FAN / MOT / HOT
					g_ota_sta = 3;
					g_ota_recv_sum = 0;
					g_ota_pg_numid = 0;
					g_ota_pg_nums = 0;
					g_ota_bin_size = 0;
					memcpy(snd_buf, data, num);
					break;
				// FE 40 02 + 3B-Size + 1B-TotalPackagesNum + MD5 + FF
				case 0x02:// Total Packages & MD5
					g_ota_pg_nums = data[6];
					g_ota_bin_size = (data[3]<<16) + (data[4]<<8) + data[5];
					memcpy(g_ota_bin_md5, data+7, SSL_MAX_LEN);
					memcpy(snd_buf, data, num);
					break;
				// FE 40 03 + 1B-PackageNum + MD5 + FF
				case 0x03:// Divided Package & MD5
					g_ota_pg_numid = data[3];
					memcpy(g_ota_package_md5, data+4, SSL_MAX_LEN);
					memcpy(snd_buf, data, num);
					break;
				// FE 40 04 + 1B-RESULT + 1B-PackageNum + FF
				// RESULT: 1-pass, 2-fail
				case 0x04:// MCU Received Divided FW INFO
					snd_buf[3] = 1;// pass
					snd_len = 0;
					break;
				// FE 40 06 01 FF
				case 0x06:// Query OTA Sta
					snd_buf[3] = g_ota_sta;// 0-idle, 1-pass, 2-fail, 3-in progress
					break;
				// FE 40 07 01 FF
				case 0x07:// Force to Stop OTA
					g_ota_sta = 0;
					g_ota_recv_sum = 0;
					g_ota_pg_numid = 0;
					g_ota_pg_nums = 0;
					g_ota_bin_size = 0;
					memcpy(snd_buf, data, num);
					break;
				default:
					snd_len = 0;
					break;
			}
			break;
		default:
			snd_len = 0;
			break;

	}
	
	if (snd_len != 0) {
		UART1_SendData(snd_buf, snd_len);
	}
}

void UART1_SendData(u8 *data, u16 num)
{
	u16 t = 0;
	u16 len = num;
	
	for(t=0;t<len;t++)
	{
		USART_SendData(USART1, data[t]);
		while(USART_GetFlagStatus(USART1,USART_FLAG_TC)!=SET);
	}
}

void UART1_ReportMcuNeedReset(void)
{
	u8 buf[8] = {0xFF, 0x05, 0x01, 0xFF};

	UART1_SendData(buf, 4);
}

// FE 40 04 + 1B-RESULT + 1B-PackageNum + FF
// RESULT: 1-pass, 2-fail
void UART1_ReportOtaPackageSta(u8 md5_res)
{
	u8 buf[8] = {0xFF, 0x40, 0x04, 0x01, 0x01, 0xFF};// Pass

	if (md5_res != 1) {
		buf[3] = 2;// Fail
	}

	buf[4] = g_ota_pg_numid;

	UART1_SendData(buf, 6);
}

// MCU request to stop ota
void UART1_RequestStopOta(void)
{
	u8 buf[5] = {0xFF, 0x40, 0x08, 0x01, 0xFF};

	UART1_SendData(buf, 5);
}

void UART1_ReportOtaBinSta(u8 md5_res)
{
	u8 buf[5] = {0xFF, 0x40, 0x05, 0x01, 0xFF};// Pass

	if (md5_res != 1) {
		buf[3] = 2;// Fail
	}

	UART1_SendData(buf, 5);
}

void USART1_IRQHandler(void)
{
#ifdef SYSTEM_SUPPORT_OS	 	
	OSIntEnter();    
#endif

		 if (USART_GetITStatus(USART1, USART_IT_IDLE) != RESET)  
	   {
				TIM_SetCounter(TIM6,0);
			  TIM_Cmd(TIM6, ENABLE);

	      USART_ClearITPendingBit(USART1, USART_IT_IDLE);
	      USART_ReceiveData(USART1);
	   }

#ifdef SYSTEM_SUPPORT_OS	 
	OSIntExit();  											 
#endif
} 
#endif	

