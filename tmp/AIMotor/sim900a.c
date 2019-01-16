#include "sim900a.h" 
#include "delay.h"	
#include "led.h"     
#include "w25qxx.h"  
#include "malloc.h"
#include "string.h"
#include "usart3.h" 
#include "ff.h" 
#include "ucos_ii.h" 
#include "can1.h"
#include "rfid.h"
#include "MP3PLAY.H"
#include "rtc.h"

// Door LOCK Sta:
// BIT7: 0-idle, 1-changed
// BIT0: 0-Close, 1-Open
u8 g_drlock_sta_chged = 0;

// DOOR Sta:
u8 g_door_state = 0;

// ECAR ON12V:
u8 g_power_state = 0;

u8 g_iap_update = 0;
u8 g_iap_update_name[128] = "";

u8 g_mp3_update = 0;
u8 g_mp3_update_name[128] = "0:/test.wav";

u32 g_dw_recv_sum = 0;
u32 g_dw_size_total = 180044;

// BIT7: 0-idle, 1-changed
// BIT0: 0-Start, 1-Stop
u8 g_bms_charge_sta_chged = 0;

u8 g_invaid_move = 0;

u8 g_calypso_active = 0;

// BT(if no BT, use SIM)
u8 g_mac_addr[32] = "";
u8 g_iccid_sim[32] = "";

u8 g_bms_temp_max = 30;// 30
u8 g_bms_temp_min = 30;// 30
u8 g_bms_battery_vol = 80;// 80%
u16 g_bms_charged_times = 0;// Save into ExFlash

u8 gps_temp_dat1[32] = "";
u8 gps_temp_dat2[32] = "";
u8 gps_temp_dat3[32] = "";
u8 gps_temp_dat4[32] = "";
u8 gps_temp_dat5[32] = "";

u8 USART3_RX_BUF_BAK[U3_RECV_LEN_ONE];

void SoftReset(void);
void write_logs(char *module, char *log, u16 size, u8 mode);

extern u8 g_mp3_play;
extern u8 g_mp3_play_name[32];
extern u8 g_calypso_card_id[CARD_ID_SIZE+1];
extern u8 g_calypso_serial_num[SERIAL_NUM_SIZE+1];

__sim7500dev sim7500dev;

const char* cmd_list[] = {
	CMD_DEV_REGISTER,
	CMD_HEART_BEAT,
	CMD_QUERY_PARAMS,
	CMD_RING_ALARM,
	CMD_UNLOCK_DOOR,
	CMD_DOOR_LOCKED,
	CMD_DOOR_UNLOCKED,
	CMD_JUMP_LAMP,
	CMD_CALYPSO_UPLOAD,
	CMD_ENGINE_START,
	CMD_INVALID_MOVE,
	CMD_REPORT_GPS,
	CMD_IAP_SUCCESS,
	CMD_CHARGE_STARTED,
	CMD_CHARGE_STOPED,
	CMD_DEV_SHUTDOWN,
	CMD_QUERY_GPS,
	CMD_IAP_UPGRADE,
	CMD_MP3_UPDATE,	
	CMD_MP3_PLAY,
	CMD_START_TRACE,
	CMD_STOP_TRACE,
	CMD_QUERY_BMS,
	CMD_QUERY_MP3,
	NULL
};

char send_buf[LEN_MAX_SEND] = "";

u8 g_server_time[LEN_SYS_TIME+1] = "";

static u8 g_ring_times = 0;
static u8 g_lamp_times = 0;

u8 g_hbeat_gap = 6;// default 6s
u8 g_gps_trace_gap = 20;// default 10s

u8 g_longitude[32] = "";
u8 g_latitude[32] = "";
u8 g_imei_str[32] = "";

u8 g_dw_write_enable = 0;
vu16 g_data_pos = 0;
vu16 g_data_size = 0;
void sim7500e_mobit_process(u8* hbeaterrcnt, u8 index);

u8 sim7500e_get_cmd_count()
{
	u8 cnt = 0;
	while(1) {
		if (NULL == cmd_list[cnt]) {
			break;
		}
		cnt++;
	}

	return cnt;
}

u8 sim7500e_is_supported_cmd(u8 cnt, char* str)
{
	u8 i = 0;

	for (i=0; i<cnt; i++) {
		if (0 == strncmp(str, cmd_list[i], strlen(cmd_list[i]))) {
			break;
		}
	}

	if (i != UNKNOWN_CMD) {
		printf("Recved CMD/ACK %s\n", str);
	}

	return i;
}

char* sim7500e_connect_check(u8 index)
{
	char *strx=0;
	strx=strstr((const char*)(USART3_RX_BUF+U3_RECV_LEN_ONE*index),(const char*)"CONNECT OK");
	if (NULL == strx) {
		strx=strstr((const char*)(USART3_RX_BUF+U3_RECV_LEN_ONE*index),(const char*)"ALREADY CONNECT");
		if (NULL == strx) {
			strx=strstr((const char*)(USART3_RX_BUF+U3_RECV_LEN_ONE*index),(const char*)"CONNECT FAIL");
			if (NULL == strx) {
				strx=strstr((const char*)(USART3_RX_BUF+U3_RECV_LEN_ONE*index),(const char*)"ERROR");
				if (strx != NULL) {
					sim7500dev.tcp_status = 2;// Connect Failed/Error
				} else {
					// Unknown Recved MSG
				}
			} else {
				sim7500dev.tcp_status = 2;// Connect Failed/Error
			}
		} else {
			sim7500dev.tcp_status = 1;// Connect OK
		}
	} else {
		sim7500dev.tcp_status = 1;// Connect Failed/Error
	}
	
	return strx;
}

u8 sim7500e_ipsta_check(u8 *sta)
{
	if (strstr((const char*)USART3_RX_BUF_BAK, "CLOSED")) {
		*sta = 2;
	}

	if (strstr((const char*)USART3_RX_BUF_BAK, "CONNECT OK")) {
		*sta = 1;
	}
}

u8 sim7500e_mp3_dw_check(void)
{
	u16 i = 0;
	u32 br = 0;
	u8 res = 0;
	FIL f_txt;
	u16 size = 0;
	u16 size_pos = 0;
	u16 data_pos = 0;
	u8 size_str[8] = "";

	for (i=0; i<U3_RECV_LEN_ONE; i++) {
		if ((':' == USART3_RX_BUF_BAK[i]) && (' ' == USART3_RX_BUF_BAK[i+1])) {
			size_pos = i + 2;
		}

		if ((i >= size_pos) && (size_pos != 0)) {
			if ((0x0D == USART3_RX_BUF_BAK[i]) && (0x0A == USART3_RX_BUF_BAK[i+1])) {
				data_pos = i + 2;
				break;
			}

			if ((i-size_pos) < 8) {
				size_str[i-size_pos] = USART3_RX_BUF_BAK[i];
			} else {
				printf("DW size is too large = %s!\n", size_str);
			}
		}
	}

	size = atoi(size_str);

	g_data_pos = data_pos;
	g_data_size = size;
	g_dw_write_enable = 1;
	
	while(1) {
		if (0 == g_dw_write_enable) {
			break;
		}
		delay_ms(10);
	}
	
	g_dw_recv_sum += size;
	
	return 0;
}

u8 sim7500e_imei_check(void)
{
	u8 i = 0;
	
	memset(g_imei_str, 0, 16);
	while(1) {
		if ((USART3_RX_BUF_BAK[2+i]>='0') && (USART3_RX_BUF_BAK[2+i]<='9')) {// 2:\r\n
			g_imei_str[i] = USART3_RX_BUF_BAK[2+i];
		} else {
			if ((USART3_RX_BUF_BAK[2+i]!='\r') && (USART3_RX_BUF_BAK[2+i]!='\n')) {
				break;
			}
		}
		i++;
	}
	
	printf("IMEI = %s\n", g_imei_str);
	
	if (strlen((const char*)g_imei_str) >= 10) {
		return 1;// OK
	} else {
		return 0;// NG
	}
}

u8 sim7500e_gps_check(void)
{
	memset(gps_temp_dat4, 0, 32);
	memset(gps_temp_dat5, 0, 32);
	sscanf((const char*)USART3_RX_BUF_BAK, "%[^,],%[^,],%[^,],%[^,],%[^,]", gps_temp_dat1, gps_temp_dat2, gps_temp_dat3, gps_temp_dat4, gps_temp_dat5);
	
	// 如果中途SIM7000E断电重启，那么是有回显的
	// 判断GPS PWR是否因为异常断电导致被关闭了
	// Exception Process
	
	memset(g_latitude, 0, 32);
	memset(g_longitude, 0, 32);
	if (strlen((const char*)gps_temp_dat4) > 5) {
		strcpy((char*)g_latitude, (const char*)gps_temp_dat4);
		strcpy((char*)g_longitude, (const char*)gps_temp_dat5);
		printf("GPS Latitude(%s), Longitude(%s)\n", gps_temp_dat4, gps_temp_dat5);
	} else {
		printf("Get GPS Nothing...\n");
	}
	
	
	return 1;// OK
}

u8* sim7500e_check_cmd(u8 *str, u8 index)
{
	char *strx = 0;

	printf("SIM7000E-01 Recv Data %s\n", USART3_RX_BUF+U3_RECV_LEN_ONE*index);
	// write_logs("SIM7000E", (char*)(USART3_RX_BUF+U3_RECV_LEN_ONE*index), USART3_RX_STA[index]&0X7FFF, 0);

	// CONNECT's actual result ACK maybe recved 150sec later after "OK"
	// very very long time wait, so must wait till recved CONNECT or ERROR
	
	if (0 == strcmp((const char*)str, "CONNECT")) {
		strx = sim7500e_connect_check(index);
	} else if (0 == strcmp((const char*)str, PROTOCOL_HEAD)) {// Unexcept Recved, But very emergency, Just Process it now
		u8 temp = 0;
		sim7500e_mobit_process(&temp, index);
		// Unexcept Recved must return NULL
		// strx = NULL
	} else {
		strx = strstr((const char*)(USART3_RX_BUF+U3_RECV_LEN_ONE*index), (const char*)str);
	}
	
	// Already get "OK", next to get extra info(IMEI / GPS)
	// Go on receiving other packages ASAP
	memcpy(USART3_RX_BUF_BAK, USART3_RX_BUF+U3_RECV_LEN_ONE*index, U3_RECV_LEN_ONE);
	USART3_RX_STA[index] = 0;
	
	return (u8*)strx;
}

u8 sim7500e_send_cmd(u8 *cmd, u8 *ack, u16 waittime)
{
	u8 ret = 0;
	u8 res = 0;

	if (0 == strcmp((const char*)cmd, "AT+CIPSTART")) {
		sim7500dev.tcp_status=0;// IDLE
	}
	
	printf("SIM7000E Send Data %s\n", cmd);
	// write_logs("SIM7000E", (char*)cmd, strlen((char*)cmd), 1);
	
	sim7500dev.cmdon = 1;
	if ((u32)cmd <= 0XFF) {
		while ((USART3->SR&0X40) == 0);
		USART3->DR = (u32)cmd;
	} else {
		u3_printf("%s\r\n", cmd);
	}
	if (ack&&waittime) {
		u8 i = 0;
		while (--waittime) {
			delay_ms(10);
			// printf("test100\n");
			for (i=0; i<U3_RECV_BUF_CNT; i++) {
				if (USART3_RX_STA[i]&0X8000) {
					if (sim7500e_check_cmd(ack, i)) {
						res = 0;
						ret = 1;
						break;
					} else {
						res = 1;
					}
				}
			}

			if (1 == ret) {
				break;
			}

			delay_ms(40);
		}
		if (waittime == 0) {
			res = 2;
		}
	}

	return res;
}

void sim7500e_tcp_send(char* send)
{
	if(sim7500e_send_cmd("AT+CIPSEND",">",40)==0)//发送数据
	{
		sim7500e_send_cmd((u8*)send,0,500);	//发送数据:0X00  
		delay_ms(20);						//必须加延时
		sim7500e_send_cmd((u8*)0X1A,0,0);	//CTRL+Z,结束数据发送,启动一次传输	
	}else sim7500e_send_cmd((u8*)0X1B,0,0);	//ESC,取消发送 	
}

// DEV ACK
void sim7500e_do_engine_start_ack(char* send)
{
	CAN1_StartEngine();

	memset(send, 0, LEN_MAX_SEND);
	sprintf(send, "%s,%s,%s,%s,%s,%d$", PROTOCOL_HEAD, DEV_TAG, g_imei_str, CMD_DEV_ACK, CMD_ENGINE_START, g_power_state);

	printf("SEND:%s\n", send);
	
	sim7500e_tcp_send(send);
}

// DEV ACK
void sim7500e_do_open_door_ack(char* send)
{
	CAN1_OpenDoor();

	memset(send, 0, LEN_MAX_SEND);
	sprintf(send, "%s,%s,%s,%s,%s,%d$", PROTOCOL_HEAD, DEV_TAG, g_imei_str, CMD_DEV_ACK, CMD_UNLOCK_DOOR, g_door_state);

	printf("SEND:%s\n", send);
	
	sim7500e_tcp_send(send);
}

// DEV ACK
void sim7500e_do_jump_lamp_ack(char* send)
{
	CAN1_JumpLamp(g_lamp_times);

	memset(send, 0, LEN_MAX_SEND);
	sprintf(send, "%s,%s,%s,%s,%s$", PROTOCOL_HEAD, DEV_TAG, g_imei_str, CMD_DEV_ACK, CMD_JUMP_LAMP);

	printf("SEND:%s\n", send);
	
	sim7500e_tcp_send(send);
}

// DEV ACK
void sim7500e_do_ring_alarm_ack(char* send)
{
	CAN1_RingAlarm(g_ring_times);

	memset(send, 0, LEN_MAX_SEND);
	sprintf(send, "%s,%s,%s,%s,%s$", PROTOCOL_HEAD, DEV_TAG, g_imei_str, CMD_DEV_ACK, CMD_RING_ALARM);

	printf("SEND:%s\n", send);
	
	sim7500e_tcp_send(send);
}

// DEV ACK
void sim7500e_do_query_params_ack(char* send)
{
	memset(send, 0, LEN_MAX_SEND);
	sprintf(send, "%s,%s,%s,%s,%s,%s,%s$", PROTOCOL_HEAD, DEV_TAG, g_imei_str, CMD_DEV_ACK, CMD_QUERY_PARAMS, g_mac_addr, g_iccid_sim);

	printf("SEND:%s\n", send);
	
	sim7500e_tcp_send(send);
}

// DEV ACK
void sim7500e_do_start_trace_ack(char* send)
{
	memset(send, 0, LEN_MAX_SEND);
	sprintf(send, "%s,%s,%s,%s,%s$", PROTOCOL_HEAD, DEV_TAG, g_imei_str, CMD_DEV_ACK, CMD_START_TRACE);

	printf("SEND:%s\n", send);
	
	sim7500e_tcp_send(send);
}

// DEV ACK
void sim7500e_do_stop_trace_ack(char* send)
{
	memset(send, 0, LEN_MAX_SEND);
	sprintf(send, "%s,%s,%s,%s,%s$", PROTOCOL_HEAD, DEV_TAG, g_imei_str, CMD_DEV_ACK, CMD_STOP_TRACE);

	printf("SEND:%s\n", send);
	
	sim7500e_tcp_send(send);
}

// DEV ACK
void sim7500e_do_dev_shutdown_ack(char* send)
{
	// do something power saving
	// let sim7000e goto sleep

#if 0
	memset(send, 0, LEN_MAX_SEND);
	sprintf(send, "%s,%s,%s,%s,%s$", PROTOCOL_HEAD, DEV_TAG, g_imei_str, CMD_DEV_ACK, CMD_DEV_SHUTDOWN);

	printf("SEND:%s\n", send);
	
	sim7500e_tcp_send(send);
#endif
}

// DEV ACK
void sim7500e_do_query_gps_ack(char* send)
{
	memset(send, 0, LEN_MAX_SEND);

	if (strlen((const char*)g_longitude) > 5) {
		sprintf(send, "%s,%s,%s,%s,%s,%s,%s,0$", PROTOCOL_HEAD, DEV_TAG, g_imei_str, CMD_DEV_ACK, CMD_QUERY_GPS, g_longitude, g_latitude);
	} else {
		sprintf(send, "%s,%s,%s,%s,%s,F,F,0$", PROTOCOL_HEAD, DEV_TAG, g_imei_str, CMD_DEV_ACK, CMD_QUERY_GPS);
	}

	printf("SEND:%s\n", send);
	
	sim7500e_tcp_send(send);
}

// DEV ACK
void sim7500e_do_iap_upgrade_ack(char* send)
{
	memset(send, 0, LEN_MAX_SEND);
	sprintf(send, "%s,%s,%s,%s,%s$", PROTOCOL_HEAD, DEV_TAG, g_imei_str, CMD_DEV_ACK, CMD_IAP_UPGRADE);

	printf("SEND:%s\n", send);
	
	sim7500e_tcp_send(send);

	// Must stop other send/recv first
	// Do http get and Flash
	// Do SoftReset
}

// DEV ACK
void sim7500e_do_mp3_play_ack(char* send)
{
	FIL f_txt;
	char filename[64] = "";

	memset(send, 0, LEN_MAX_SEND);
	sprintf(filename, "0:/MUSIC/%s", g_mp3_play_name);

	if (0 == f_open(&f_txt,(const TCHAR*)filename, FA_READ)) {// existing
		g_mp3_play = 1;
		sprintf(send, "%s,%s,%s,%s,%s,%s,1$", PROTOCOL_HEAD, DEV_TAG, g_imei_str, CMD_DEV_ACK, CMD_MP3_PLAY, g_mp3_play_name);
		// Play mp3 in other task
	} else {// file non-existing
		g_mp3_play = 0;
		sprintf(send, "%s,%s,%s,%s,%s,%s,0$", PROTOCOL_HEAD, DEV_TAG, g_imei_str, CMD_DEV_ACK, CMD_MP3_PLAY, g_mp3_play_name);
	}

	printf("SEND:%s\n", send);
	
	sim7500e_tcp_send(send);
}

// DEV ACK
void sim7500e_do_query_bms_ack(char* send)
{
	memset(send, 0, LEN_MAX_SEND);
	sprintf(send, "%s,%s,%s,%s,%s,%d,%d,%d$", PROTOCOL_HEAD, DEV_TAG, g_imei_str, CMD_DEV_ACK, CMD_QUERY_BMS, g_bms_battery_vol, g_bms_charged_times, g_bms_temp_max);

	printf("SEND:%s\n", send);
	
	sim7500e_tcp_send(send);
}

// DEV ACK
void sim7500e_do_query_mp3_ack(char* send)
{
	// TBD: send buf size maybe not enough
	memset(send, 0, LEN_MAX_SEND);
	// sprintf(send, "%s,%s,%s,%s,%s,%d,%d,%d$", PROTOCOL_HEAD, DEV_TAG, g_imei_str, CMD_DEV_ACK, CMD_QUERY_BMS, g_bms_battery_vol, g_bms_charged_times, g_bms_temp_max);

	printf("SEND:%s\n", send);
	
	sim7500e_tcp_send(send);
}

// DEV ACK
void sim7500e_do_mp3_dw_success_ack(char* send)
{
	memset(send, 0, LEN_MAX_SEND);
	sprintf(send, "%s,%s,%s,%s,%s,%s$", PROTOCOL_HEAD, DEV_TAG, g_imei_str, CMD_DEV_ACK, CMD_MP3_UPDATE, g_mp3_update_name);

	printf("SEND:%s\n", send);
	
	sim7500e_tcp_send(send);
}

// DEV Auto Send
u8 sim7500e_do_dev_register_auto(char* send)
{
	memset(send, 0, LEN_MAX_SEND);
	sprintf(send, "%s,%s,%s,%s,%s,%s,%d$", PROTOCOL_HEAD, DEV_TAG, g_imei_str, CMD_DEV_REGISTER, HW_VERSION, SW_VERSION, g_bms_battery_vol);

	printf("SEND:%s\n", send);
	
	sim7500e_tcp_send(send);
	
	return 0;
}

// DEV Auto Send
void sim7500e_do_heart_beat_auto(char* send)
{
	u8 rssi_str[LEN_RSSI_VAL] = "";
	char dev_time[LEN_SYS_TIME] = "";

	RTC_TimeTypeDef RTC_TimeStruct;
	RTC_DateTypeDef RTC_DateStruct;
		
	RTC_GetTime(RTC_Format_BIN,&RTC_TimeStruct);
	RTC_GetDate(RTC_Format_BIN, &RTC_DateStruct);

	sprintf((char*)dev_time,"20%02d%02d%02d%02d%02d%02d",RTC_DateStruct.RTC_Year,RTC_DateStruct.RTC_Month,RTC_DateStruct.RTC_Date,RTC_TimeStruct.RTC_Hours,RTC_TimeStruct.RTC_Minutes,RTC_TimeStruct.RTC_Seconds);
		
	// TBD: Get rssi

	memset(send, 0, LEN_MAX_SEND);
	sprintf(send, "%s,%s,%s,%s,%s,%d,%s,%d$", PROTOCOL_HEAD, DEV_TAG, g_imei_str, CMD_HEART_BEAT, dev_time, (g_drlock_sta_chged&0x7F), rssi_str, g_bms_battery_vol);
	
	printf("SEND:%s\n", send);
	
	sim7500e_tcp_send(send);
}

// DEV Auto SEND
void sim7500e_do_door_closed_report(char* send)
{
	memset(send, 0, LEN_MAX_SEND);
	sprintf(send, "%s,%s,%s,%s$", PROTOCOL_HEAD, DEV_TAG, g_imei_str, CMD_DOOR_LOCKED);

	printf("SEND:%s\n", send);
	
	sim7500e_tcp_send(send);
}

// DEV Auto SEND
void sim7500e_do_door_opened_report(char* send)
{
	memset(send, 0, LEN_MAX_SEND);
	sprintf(send, "%s,%s,%s,%s$", PROTOCOL_HEAD, DEV_TAG, g_imei_str, CMD_DOOR_UNLOCKED);

	printf("SEND:%s\n", send);
	
	sim7500e_tcp_send(send);
}

// DEV Auto SEND
void sim7500e_do_invalid_moving_report(char* send)
{
	memset(send, 0, LEN_MAX_SEND);
	sprintf(send, "%s,%s,%s,%s$", PROTOCOL_HEAD, DEV_TAG, g_imei_str, CMD_INVALID_MOVE);

	printf("SEND:%s\n", send);
	
	sim7500e_tcp_send(send);
}

// DEV Auto SEND
void sim7500e_do_gps_location_report(char* send)
{
	memset(send, 0, LEN_MAX_SEND);

	sim7500e_send_cmd("AT+CGNSINF","OK",40);
	sim7500e_gps_check();
	
	if (strlen((const char*)g_longitude) > 5) {
		sprintf(send, "%s,%s,%s,%s,%s,%s,0$", PROTOCOL_HEAD, DEV_TAG, g_imei_str, CMD_REPORT_GPS, g_longitude, g_latitude);
	} else {
		sprintf(send, "%s,%s,%s,%s,F,F,0$", PROTOCOL_HEAD, DEV_TAG, g_imei_str, CMD_REPORT_GPS);
	}

	printf("SEND:%s\n", send);
	
	sim7500e_tcp_send(send);
}

// DEV Auto SEND
void sim7500e_do_iap_success_report(char* send)
{
	memset(send, 0, LEN_MAX_SEND);
	sprintf(send, "%s,%s,%s,%s,%s$", PROTOCOL_HEAD, DEV_TAG, g_imei_str, CMD_IAP_SUCCESS, SW_VERSION);

	printf("SEND:%s\n", send);
	
	sim7500e_tcp_send(send);
}

// DEV Auto SEND
void sim7500e_do_charge_start_report(char* send)
{
	memset(send, 0, LEN_MAX_SEND);
	sprintf(send, "%s,%s,%s,%s,%d,%d$", PROTOCOL_HEAD, DEV_TAG, g_imei_str, CMD_CHARGE_STARTED, g_bms_battery_vol, g_bms_charged_times);

	printf("SEND:%s\n", send);
	
	sim7500e_tcp_send(send);
}

// DEV Auto SEND
void sim7500e_do_charge_stop_report(char* send)
{
	memset(send, 0, LEN_MAX_SEND);
	sprintf(send, "%s,%s,%s,%s,%d,%d$", PROTOCOL_HEAD, DEV_TAG, g_imei_str, CMD_CHARGE_STOPED, g_bms_battery_vol, g_bms_charged_times);

	printf("SEND:%s\n", send);
	
	sim7500e_tcp_send(send);
}

// DEV Auto SEND
void sim7500e_do_calypso_report(char* send)
{
	memset(send, 0, LEN_MAX_SEND);
	sprintf(send, "%s,%s,%s,%s,%s$", PROTOCOL_HEAD, DEV_TAG, g_imei_str, CMD_CALYPSO_UPLOAD, g_calypso_card_id);

	printf("SEND:%s\n", send);
	
	sim7500e_tcp_send(send);
}

void sim7500e_parse_msg(char* msg, char* send)
{
	int index = 0;
	int data_pos = 0;
	char delims[] = ",";
	char* split_str = NULL;

	enum CMD_TYPE cmd_type = UNKNOWN_CMD;

	int cmd_count = sim7500e_get_cmd_count();

#ifdef DEBUG_USE
	//printf("Support %d CMDs\n", cmd_count);
#endif

	split_str = strtok(msg, delims);
	while(split_str != NULL) {
#ifdef DEBUG_USE
		//printf("split_str = %s\n", split_str);
#endif
		// index = 3: SVR CMD
		// index = 4: SVR ACK
		if ((3 == index) || (4 == index)) {
			if (UNKNOWN_CMD == cmd_type) {
				cmd_type = (enum CMD_TYPE)sim7500e_is_supported_cmd(cmd_count, split_str);

				if (cmd_type != UNKNOWN_CMD) {
					if (0 == data_pos) {
						data_pos = index;
						// printf("data_pos = %d, cmd_type = %d\n", data_pos, cmd_type);
					}
					
					// No need Parse extra params
					if (UNLOCK_DOOR == cmd_type) {
						sim7500e_do_open_door_ack(send);
					} else if (ENGINE_START == cmd_type) {
						sim7500e_do_engine_start_ack(send);
					} else if (QUERY_PARAMS == cmd_type) {
						sim7500e_do_query_params_ack(send);
					} else if (DEV_SHUTDOWN == cmd_type) {
						sim7500e_do_dev_shutdown_ack(send);
					} else if (QUERY_GPS == cmd_type) {
						sim7500e_do_query_gps_ack(send);
					} else if (QUERY_BMS == cmd_type) {
						sim7500e_do_query_bms_ack(send);
					} else if (STOP_TRACE == cmd_type) {
						g_gps_trace_gap = 0;
						printf("g_gps_trace_gap = %d\n", g_gps_trace_gap);
						sim7500e_do_stop_trace_ack(send);
					}
				} else {
					// TBD: Exception Process
					break;
				}
			}
		}

		// Parse CMD or ACK
		// Need to Parse extra params
		if (index > data_pos) {
			if (DEV_REGISTER == cmd_type) {
				if (5 == index) {
					strncpy((char*)g_server_time, split_str, LEN_SYS_TIME);
					g_server_time[LEN_SYS_TIME] = '\0';
					printf("g_server_time = %s\n", g_server_time);
					// TBD: Need To Check
					RTC_Sync_time(g_server_time);
				} else if (6 == index) {
					g_hbeat_gap = atoi(split_str);
					printf("g_hbeat_gap = %d\n", g_hbeat_gap);
				}
			} else if (HEART_BEAT == cmd_type) {
				// Do nothing
			} else if (IAP_UPGRADE == cmd_type) {
				g_iap_update = 1;
				strcpy((char*)g_iap_update_name, split_str);
				sim7500e_do_iap_upgrade_ack(send);
			} else if (MP3_PLAY == cmd_type) {
				strcpy((char*)g_mp3_play_name, split_str);
				sim7500e_do_mp3_play_ack(send);
			} else if (MP3_UPDATE == cmd_type) {
				g_mp3_update = 1;
				strcpy((char*)g_mp3_update_name, split_str);
			} else if (START_TRACE == cmd_type) {
				g_gps_trace_gap = atoi(split_str);
				printf("g_gps_trace_gap = %d\n", g_gps_trace_gap);
				sim7500e_do_start_trace_ack(send);
			} else if (RING_ALARM == cmd_type) {
				g_ring_times = atoi(split_str);
				printf("g_ring_times = %d\n", g_ring_times);
				sim7500e_do_ring_alarm_ack(send);
			} else if (JUMP_LAMP == cmd_type) {
				g_lamp_times = atoi(split_str);
				printf("g_lamp_times = %d\n", g_lamp_times);
				sim7500e_do_jump_lamp_ack(send);
			}
		}
		split_str = strtok(NULL, delims);
		index++;
	}
}

u8 sim7500e_setup_connect(void)
{
	u8 i = 0;

	sim7500dev.tcp_status=0;// IDLE
	
	printf("Start CIPSTART...\n");

	i = 0;
	while (1) {
		// sim7500e_send_cmd("AT+CIPSHUT","SHUT OK",200);
		if (sim7500e_send_cmd("AT+CIPSTART=\"TCP\",\"47.105.112.41\",88", "CONNECT",600)) {// Max 600*50ms = 30s		
			if(sim7500e_send_cmd("AT+CIPSHUT","SHUT OK",200)) {
				delay_ms(1000);
				sim7500e_send_cmd("AT+CIPSHUT","SHUT OK",200);
			}
		}
		
		if (1 == sim7500dev.tcp_status) {// Connected OK
			break;
		} else {
			// write_logs("SIM7000E", (char*)"Cannot Setup TCP Connect, just soft restart...\n", strlen((char*)"Cannot Setup TCP Connect, just soft restart...\n"), 3);
			// printf("Cannot Setup TCP Connect, just soft restart...\n");
			// Re-Close->Open Try
			delay_ms(100);
		}
		
		if (5 == i++) {
			break;
		}
	}

	if (1 == sim7500dev.tcp_status) {// Connected OK
		delay_ms(10);
		if(sim7500e_send_cmd("AT+CIPSEND=5",">",40)) return 1;
		delay_ms(10);
	
		if(sim7500e_send_cmd("Hello","SEND OK",200)) return 1;
		delay_ms(10);

		if(sim7500e_do_dev_register_auto(send_buf)) return 1;
		delay_ms(10);

		return 0;
	} else {
		return 1;
	}
}

u8 sim7500e_setup_initial(void)
{
	u8 i = 0;
	for (i=0; i<5; i++) {
		if (0 == sim7500e_send_cmd("AT","OK",20))break;
		if (4 == i) return 1;
		delay_ms(50);
	}
	
	// Close Echo Display
	if(sim7500e_send_cmd("ATE0","OK",40)) {
		if(sim7500e_send_cmd("ATE0","OK",40)) return 1;
	}

	// Get IMEI
	if(sim7500e_send_cmd("AT+GSN","OK",40)) {
		if(sim7500e_send_cmd("AT+GSN","OK",40)) return 1;
	}
	sim7500e_imei_check();

	// TBD: Get ICCID
	// TBD: Get MAC of SIM7000E or BT
	
	// Open GPS
	if(sim7500e_send_cmd("AT+CGNSPWR=1","OK",40)) {
		if(sim7500e_send_cmd("AT+CGNSPWR=1","OK",40)) return 1;
	}
	
	if(sim7500e_send_cmd("AT+CGATT?","+CGATT: 1",40)) {
		if(sim7500e_send_cmd("AT+CGATT?","+CGATT: 1",40)) return 1;
	}

	if(sim7500e_send_cmd("AT+CIPSHUT","SHUT OK",200)) {
		if(sim7500e_send_cmd("AT+CIPSHUT","SHUT OK",200)) return 1;
	}
	
	if(sim7500e_send_cmd("AT+CSTT=\"CMNET\"","OK",40)) {
		if(sim7500e_send_cmd("AT+CSTT=\"CMNET\"","OK",40)) return 1;
	}
	
	if(sim7500e_send_cmd("AT+CIICR","OK",200)) {
		if(sim7500e_send_cmd("AT+CIICR","OK",200)) return 1;
	}
	
	sim7500e_send_cmd("AT+CIFSR",0,40);
	delay_ms(200);
	
	// Temp Test
	// TBD: Need to be delete
	CAN1_JumpLamp(g_lamp_times);

	return sim7500e_setup_connect();
}

u8 sim7500e_setup_http(void)
{
	sim7500e_send_cmd("AT+HTTPTERM","XXX",100);
	delay_ms(1000);
	sim7500e_send_cmd("AT+SAPBR=0,1","XXX",100);
	delay_ms(1000);
	
	if (sim7500e_send_cmd("AT+SAPBR=3,1,\"APN\",\"CMNET\"","OK",500)) {
		if (sim7500e_send_cmd("AT+SAPBR=3,1,\"APN\",\"CMNET\"","OK",500)) return 1;
	}
	
	if (sim7500e_send_cmd("AT+SAPBR=1,1","OK",500)) {
		if (sim7500e_send_cmd("AT+SAPBR=1,1","OK",500)) return 1;
	}
	
	if (sim7500e_send_cmd("AT+SAPBR=2,1","OK",500)) {
		if (sim7500e_send_cmd("AT+SAPBR=2,1","OK",500)) return 1;
	}
	
	if (sim7500e_send_cmd("AT+HTTPINIT","OK",500)) {
		if (sim7500e_send_cmd("AT+HTTPINIT","OK",500)) return 1;
	}
	
	if (sim7500e_send_cmd("AT+HTTPPARA=\"CID\",1","OK",500)) {
		if (sim7500e_send_cmd("AT+HTTPPARA=\"CID\",1","OK",500)) return 1;
	}
	
	if (sim7500e_send_cmd("AT+HTTPPARA=\"URL\",\"http://gdlt.sc.chinaz.com/Files/DownLoad/sound1/201701/8224.wav\"","OK",500)) {
		if (sim7500e_send_cmd("AT+HTTPPARA=\"URL\",\"http://gdlt.sc.chinaz.com/Files/DownLoad/sound1/201701/8224.wav\"","OK",500)) return 1;
	}
	
	if (sim7500e_send_cmd("AT+HTTPACTION=0","+HTTPACTION: 0,200",500)) {
		if (sim7500e_send_cmd("AT+HTTPACTION=0","+HTTPACTION: 0,200",500)) return 1;
	}
	g_dw_size_total = atoi(USART3_RX_BUF_BAK+21);
	
	return 0;
}

void sim7500e_idle_actions(u16 gps_cnt)
{
#if 0
	if (g_calypso_active) {
		sim7500e_do_calypso_report(send_buf);
		g_calypso_active = 0;
	}
	if (g_drlock_sta_chged&0x80) {// Door Changed
		if (g_drlock_sta_chged&0x01) {// OPEN
			sim7500e_do_door_opened_report(send_buf);
		} else {// CLOSE
			sim7500e_do_door_closed_report(send_buf);
		}
		g_drlock_sta_chged &= 0x7F;
	}
	if (g_bms_charge_sta_chged&0x80) {// Charge Changed
		if (g_bms_charge_sta_chged&0x01) {// Start
			sim7500e_do_charge_start_report(send_buf);
		} else {// Stop
			sim7500e_do_charge_stop_report(send_buf);
		}
		g_bms_charge_sta_chged &= 0x7F;
	}
	if (g_invaid_move) {
		sim7500e_do_invalid_moving_report(send_buf);
	}
	if (g_gps_trace_gap) {
		// every loop delay 10ms
		if (0 == gps_cnt) {
			sim7500e_do_gps_location_report(send_buf);
		}
	}
#endif
	if (1 == g_mp3_update) {
		u8 res = 0;
		FIL f_txt;
		u16 split_size = 0;

		if (sim7500e_setup_http()) {
			return;
		}
#if 1		
		res = f_open(&f_txt,(const TCHAR*)g_mp3_update_name,FA_READ|FA_WRITE|FA_CREATE_ALWAYS);
		if (0 == res) {
			f_close(&f_txt);
		}
#endif

		// do http get
		while (1) {
			if ((split_size+g_dw_recv_sum) > g_dw_size_total) {
				split_size = g_dw_size_total - g_dw_recv_sum;
			} else {
				split_size =  512;
			}

			printf("g_dw_recv_sum = %d\n", g_dw_recv_sum);
			sprintf(send_buf, "AT+HTTPREAD=%d,%d", g_dw_recv_sum, split_size);
			if (0 == sim7500e_send_cmd(send_buf,"+HTTPREAD",500)) {
				if (sim7500e_mp3_dw_check()) {
					break;
				}
			}

			if (g_dw_recv_sum == g_dw_size_total) {
				g_mp3_update = 3;
				break;
			}
		}
	}
	if (g_iap_update) {
		// do http get
	}
}

void sim7500e_mobit_process(u8* hbeaterrcnt, u8 index)
{
	u8 *pTemp = NULL;
	u8 data_lenth = 0;

	printf("SIM7000E-03 Recv Data %s\n", USART3_RX_BUF+U3_RECV_LEN_ONE*index);
	// write_logs("SIM7000E", (char*)(USART3_RX_BUF+U3_RECV_LEN_ONE*index), USART3_RX_STA[index]&0X7FFF, 0);
			
	if (*hbeaterrcnt) {
		if (strstr((const char*)(USART3_RX_BUF+U3_RECV_LEN_ONE*index), "SEND OK")) {
			*hbeaterrcnt = 0;
		}
	}
			
	// Received User Data
	pTemp = (u8*)strstr((const char*)(USART3_RX_BUF+U3_RECV_LEN_ONE*index), PROTOCOL_HEAD);
	if (pTemp) {
		data_lenth = strlen((const char*)pTemp);
				
		memset(USART3_RX_BUF_BAK, 0, U3_RECV_LEN_ONE);
		// TBD: maybe copy some next package's data
		memcpy(USART3_RX_BUF_BAK, pTemp, U3_RECV_LEN_ONE);
				
		if (data_lenth < LEN_MAX_RECV) {
			USART3_RX_BUF_BAK[data_lenth] = '\0';// $ -> 0
		}
				
		USART3_RX_STA[index] = 0;// Go on receiving other packages ASAP
				
		printf("RECVED MSG(%dB): %s\n", data_lenth, USART3_RX_BUF_BAK);
				
		sim7500e_parse_msg(USART3_RX_BUF_BAK, send_buf);
	} else {
		USART3_RX_STA[index] = 0;
	}
}

// TBD Add Watch dog in sim7000e loop
/////////////////////////////////////////////////////////////////////////////////////////////////////////// 
void sim7500e_communication_loop(u8 mode,u8* ipaddr,u8* port)
{
	u8 i = 0;
	u8 oldsta = 0XFF;
	u8 connectsta = 0;			//0,正在连接;1,连接成功;2,连接关闭; 
	u8 hbeaterrcnt = 0;			//心跳错误计数器,连续5次心跳信号无应答,则重新连接
	u8 iap_success = 0;

	u16 timex = 0;
	u16 gps_cnt = 0;
	
	if (sim7500e_setup_initial()) {
		return;
	}

	if (iap_success) {
		sim7500e_do_iap_success_report(send_buf);
	}

	// Connected OK
	while(1) {
		if ((timex%20) == 0) {
			LED0 = !LED0;

			// TCP connect failed, just re-connect
			if (connectsta==2 || hbeaterrcnt>8) {
				//if (sim7500e_setup_connect()) {
				//	break;
				//}

				connectsta = 0;
 				hbeaterrcnt = 0;
			}
		}

		// will not be run, because, if connectsta=0, will do re-connect
		if ((connectsta==0) && (timex%200)==0) {
			//sim7500e_send_cmd("AT+CIPSTATUS","OK",500);
			//sim7500e_ipsta_check(&connectsta);
		}

		// If TCP package arrive here, then it will be 
		// parsed in sim7500e_send_cmd -> sim7500e_check_cmd
		// At most, lost one receive for AT-ACK
		// But TCP package will not be skiped
		if (1) {//(connectsta == 1) {
			if (g_gps_trace_gap) {
				// every loop delay 10ms
				if(0 == gps_cnt%(g_gps_trace_gap*100)) {
					gps_cnt = 0;
				}
				
				if (1000 == gps_cnt) {
					if (0 == g_mp3_update) {
						g_mp3_update = 1;
					}
				}
			} else {
				gps_cnt = 0;
			}
			
			sim7500e_idle_actions(gps_cnt);

			// every loop delay 10ms
			if (timex >= (g_hbeat_gap*100)) {
				timex = 0;
			
				//sim7500e_do_heart_beat_auto(send_buf);
				
				//hbeaterrcnt++;
				printf("hbeaterrcnt = %d\r\n",hbeaterrcnt);
			}
			
			gps_cnt++;
		}

		delay_ms(10);

//		for (i=0; i<U3_RECV_BUF_CNT; i++) {
//			if (USART3_RX_STA[i]&0X8000) {
//				//sim7500e_mobit_process(&hbeaterrcnt, i);
//			}
//		}

		if (oldsta != connectsta) {
			oldsta = connectsta;
		}

		timex++;
	}
}
