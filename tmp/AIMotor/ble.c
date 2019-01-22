// Mobit Protocol:
//           STX + LEN(1B) + RAND(1B) + KEY(1B) + CMD(1B) + DATA(LEN B) + 						CRC(1B)

// Encode
// CRC8 for STX / LEN / RAND / KEY / CMD / DATA
// ^ for KET / CMD / DATA with (RAND=RAND+0x32)

// Decode
// CRC8 for STX / LEN / RAND / KEY / CMD / DATA
// ^ for KET / CMD / DATA with (RAND=RAND-0x32)

// TBD: if disconnet, reset the value to 0
u8 g_key_once = 0;
const u8 password_correct[32] = "MOBITCAR";

u8 g_srv_ip[32]   = "";
u8 g_srv_apn[32]  = "";
u8 g_srv_port[32] = "";

u8 snd_buf[128] = {0};

u8 crc8_calc(u8 *data, u8 len)
{
	u8 crc = 0;

	return crc;
}

void get_srv_info(u8 *data, u8 len)
{
	u8 i = 0;
	u8 j = 0;
	u8 split_cnt = 0;

	memset(g_srv_ip, 0, 32);
	memset(g_srv_port, 0, 32);
	memset(g_srv_apn, 0, 32);

	for (i=0; i<len; i++) {
		if (',' == data[i]) {
			j = 0;
			split_cnt++;
			continue;
		}

		if (0 == split_cnt) {
			g_srv_ip[j++] = data[i];
		} else if (1 == split_cnt) {
			g_srv_port[j++] = data[i];
		} else if (2 == split_cnt) {
			g_srv_apn[j++] = data[i];
		}
	}
}

void rand_encode(u8 *data, u8 rand, u8 len)
{
	u8 i = 0;

	for (i=0; i<len; i++) {
		data[i] ^= (rand + 0x32);
	}
}

// 1-CRC NG
// 2-Not yet Get KEY
// 3-KEY NG
// 4-Head NG
// 5-Size NG
// 6-Unknown CMD
// ERR Report
// DEV->APP: A3A4 +   01 +     RAND +      KEY +    10 +     01 + 								CRC (CRC NG)
// DEV->APP: A3A4 +   01 +     RAND +      KEY +    10 +     02 + 								CRC (Please Get KEY first)
// DEV->APP: A3A4 +   01 +     RAND +      KEY +    10 +     03 +	 							CRC (KEY NG)
void auto_err_report(u8 err_mode)
{
	u8 rand = random();

	snd_buf[0] = 0xA3;
	snd_buf[1] = 0xA4;
	snd_buf[2] = 0x01;
	snd_buf[3] = rand;
	snd_buf[4] = 0x00;
	snd_buf[5] = 0x10;
	snd_buf[6] = err_mode;
	snd_buf[7] = crc8_calc(snd_buf, 7);

	rand_encode(data+4, (rand+0x32), snd_buf[2]+2);

	UART1_SendData(snd_buf, 8);
}

void ack_other_actions(u8 cmd, u8 is_action_ok)
{
	u8 rand = random();

	snd_buf[0] = 0xA3;
	snd_buf[1] = 0xA4;
	snd_buf[2] = 0x01;
	snd_buf[3] = rand;
	snd_buf[4] = g_key_once;
	snd_buf[5] = cmd;

	if (is_action_ok) {
		snd_buf[6] = 0x01;
	} else {
		snd_buf[6] = 0x02;
	}

	snd_buf[7] = crc8_calc(snd_buf, 7);

	rand_encode(data+4, (rand+0x32), snd_buf[2]+2);

	UART1_SendData(snd_buf, 8);
}

void ack_get_key(u8 is_pw_ok)
{
	u8 rand = random();
	u8 g_key_once  = random();

	snd_buf[0] = 0xA3;
	snd_buf[1] = 0xA4;
	snd_buf[2] = 0x02;
	snd_buf[3] = rand;

	if (is_pw_ok) {
		snd_buf[4] = g_key_once;
	} else {
		snd_buf[4] = 0x00;
	}

	snd_buf[5] = 0x01;

	if (is_pw_ok) {
		snd_buf[6] = 0x01;
		snd_buf[7] = g_key_once;
	} else {
		snd_buf[6] = 0x00;
		snd_buf[7] = 0x00;
	}

	snd_buf[8] = crc8_calc(snd_buf, 8);

	rand_encode(data+4, (rand+0x32), snd_buf[2]+2);

	UART1_SendData(snd_buf, 9);
}

void run_bt_cmd(u8 *data, u16 num)
{
	u8 key = 0;
	u8 rand = 0;
	
	// Check Header
	if ((data[0] != 0xA3) || (data[1] != 0xA4)) {
		auto_err_report(4);
		return;
	}

	// Check Size
	if ((data[2]+7) != num) {
		auto_err_report(5);
		return;
	}

	// Check Crc
	if (data[num-1] != crc8_calc(data, num-1)) {
		auto_err_report(1);
		return;
	}

	// Check KEY
	if ((data[5] != 0x01) && (data[4] != g_key_once))
		auto_err_report(3);
		return;
	}

	memset(snd_buf, 0, 128);

	rand = data[3];

	rand_encode(data+4, (rand-0x32), data[2]+2);

	key  = data[4];

	switch (data[5])
	{
	// Get Key
	// APP->DEV: A3A4 +   08 +     RAND +      00  +    01 +     PW(MobitCar) + 					CRC
	// DEV->APP: A3A4 +   02 +     RAND +      KEY +    01 +     01 + KEY + 						CRC (PW is OK)
	// DEV->APP: A3A4 +   02 +     RAND +      KEY +    01 +     00 + 00 + 							CRC (PW is NG)
	case 0x01:
		if (memcmp(data+6, password_correct, 8)) {// ERR
			ack_get_key(0);
		} else {// OK
			ack_get_key(1);
		}
		snd_len = 9;
		break;
	case 0x20:
		// Set IP/APN
		// APP->DEV: A3A4 +   NN +     RAND +      KEY +    20 +     192.168.1.1,88,mobit.apn + 		CRC
		// DEV->APP: A3A4 +   01 +     RAND +      KEY +    20 +     01 + 								CRC (SET OK)
		// DEV->APP: A3A4 +   01 +     RAND +      KEY +    20 +     02 + 								CRC (SET NG)
		get_srv_info(data+6, data[2]);
		ack_config_srv(1);
		break;
	case 0x21:
		// Unlock
		// APP->DEV: A3A4 +   00 +     RAND +      KEY +    21 +     NULL + 							CRC
		// DEV->APP: A3A4 +   01 +     RAND +      KEY +    21 +     01/02 + 							CRC (OK/NG)
		ack_other_actions(data[5], 1);
		break;
	case 0x22:
		// Lock
		// APP->DEV: A3A4 +   00 +     RAND +      KEY +    22 +     NULL + 							CRC
		// DEV->APP: A3A4 +   01 +     RAND +      KEY +    22 +     01/02 + 							CRC (OK/NG)
		ack_other_actions(data[5], 1);
		break;
	case 0x23:
		// Lamp Jump
		// APP->DEV: A3A4 +   00 +     RAND +      KEY +    23 +     NULL + 							CRC
		// DEV->APP: A3A4 +   01 +     RAND +      KEY +    23 +     01/02 + 							CRC (OK/NG)
		ack_other_actions(data[5], 1);
		break;
		// Reboot
		// APP->DEV: A3A4 +   00 +     RAND +      KEY +    24 +     NULL + 							CRC
		// DEV->APP: A3A4 +   01 +     RAND +      KEY +    24 +     01/02 + 							CRC (OK/NG)
	case 0x24:
		ack_other_actions(data[5], 1);
		break;
	default:
		auto_err_report(6);
		break;
	}
}
