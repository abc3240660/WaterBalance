#include	"extern.h"

//#define USE_20K 1
#define USE_10K 1

BIT		p_InB_OD	:	PB.1;
BIT		p_InA_VJ	:	PA.5;
BIT		p_InA_V	    :	PA.7;
BIT		p_InA_H	    :	PA.4;
BIT		p_InA_QV2	:	PA.0;
BIT		p_InB_QV3	:	PB.0;

BIT		p_OutA_2K	:	PA.3;
BIT		p_OutB_V1   :	PB.2;
BIT		p_OutB_V2   :	PB.5;
BIT		p_OutA_V3   :	PA.6;
BIT		p_OutB_H1   :	PB.6;

// High is active
BIT		p_OutB_LED	:	PB.7;// LED

void	FPPA0 (void)
{
	.ADJUST_IC	SYSCLK=IHRC/4		//	SYSCLK=IHRC/4
    
    $ EOSCR		DIS_LVD_BANDGAP;

	$	p_OutA_2K		    Out, Low;
	$	p_OutB_V1		    Out, Low;
	$	p_OutB_V2		    Out, Low;
    $	p_OutA_V3		    Out, Low;
	$	p_OutB_H1		    Out, Low;
    $	p_OutB_LED		    Out, Low;// off

	$	p_InB_OD		    In;
//	$	p_InB_OD		    Out, Low;// off
	$	p_InA_VJ		    In;
//	$	p_InA_V		        In;
	$	p_InA_V		        Out, Low;// off
//	$	p_InA_H		        In;
	$	p_InA_H		        Out, Low;// off
    $	p_InA_QV2		    In;
    $	p_InB_QV3		    In;

	// IN Pull-UP
    PBPH		=		_FIELD(p_InB_OD);
//	PAPH		=		_FIELD(p_InA_V, p_InA_H);
//	PAPH		=		_FIELD(p_InA_H);

#ifdef USE_10K
	$ T16M		IHRC, /4, BIT9;				// 256us
#endif

#ifdef USE_20K
	$ T16M		IHRC, /4, BIT8;				// 128us
#endif

    // 1/16M * 2^(9+1) = 64us
    // 1/16M * 2^(10+1) = 128
    // $ T16M      IHRC, /1, BIT10;// 16MHz/1 = 16MHz:the time base of T16.
	$ TM2C		IHRC, Disable, Period, Inverse;

	BYTE	Key_Flag;
	Key_Flag			=	_FIELD(p_InB_OD, p_InA_VJ, p_InA_V, p_InA_H, p_InA_QV2, p_InB_QV3);

	BYTE	Sys_Flag	=	0;
	BIT		f_Key_Trig1	:	Sys_Flag.0;
	BIT		t16_10ms	:	Sys_Flag.1;
	BIT		f_Key_Trig2	:	Sys_Flag.2;
	BIT		f_Key_Trig3	:	Sys_Flag.3;
	BIT		f_Key_Trig4	:	Sys_Flag.4;
    BIT		f_IN_QV2	:	Sys_Flag.5;
    BIT		f_IN_QV3	:	Sys_Flag.6;

	BYTE	Sys_FlagB	=	0;
	BIT		f_mode2	:	Sys_FlagB.0;
	BIT		f_2k_on	:	Sys_FlagB.1;
	BIT		f_led_flash	:	Sys_FlagB.2;
	BIT		f_led_state	:	Sys_FlagB.3;
	BIT		f_vj_on	:	Sys_FlagB.5;
	BIT		f_pwm_mode	:	Sys_FlagB.6;
	
	BYTE	Sys_FlagC	=	0;
	BIT		f_V1_on	:	Sys_FlagC.0;
	BIT		f_V2_on	:	Sys_FlagC.1;
	BIT		f_H1_on	:	Sys_FlagC.2;
    BIT		f_V3_on	:	Sys_FlagC.3;

	BYTE	Sys_FlagD	=	0;
	BIT		f_last_V1_on	:	Sys_FlagD.0;
	BIT		f_last_V2_on	:	Sys_FlagD.1;
	BIT		f_last_H1_on	:	Sys_FlagD.2;
    BIT		f_last_V3_on	:	Sys_FlagD.3;
	BIT		f_last_level	:	Sys_FlagD.4;
	BIT		f_sync_ok		:	Sys_FlagD.5;
	BIT		f_ev1527_ok		:	Sys_FlagD.6;
	BIT		f_last_xlevel	:	Sys_FlagD.7;
	
//	pmode	Program_Mode;
//	fppen	=	0xFF;

	BYTE	t16_flag;
	BYTE	count1 = 1;
	BYTE 	count2 = 0;
	BYTE	count3 = 0;
	BYTE	count4 = 0;
	BYTE	count_l = 0;
	BYTE	count_h = 0;
	BYTE	count_x = 0;
	BYTE    last_vj_state = 8;

	BYTE	cnt_Key_10ms_1	=	250;			//	Key debounce time = 40 mS
	BYTE	cnt_Key_10ms_2	=	4;				//	Key debounce time = 40 mS
	BYTE	cnt_Key_10ms_3	=	4;				//	Key debounce time = 40 mS
	BYTE	cnt_Key_10ms_4	=	4;				//	Key debounce time = 40 mS
	
	BYTE	cnt_Key_10ms_3_mode2	=	250;				//	Key debounce time = 40 mS
	BYTE	cnt_Key_10ms_4_mode2	=	250;				//	Key debounce time = 40 mS

	BYTE	cnt_3s_time_1 	= 0;// 
	BYTE	cnt_3s_time_3 	= 0;// CN1/V
	BYTE	cnt_3s_time_4 	= 0;// CN1/H
	BYTE	cnt_3s_time_2k 	= 0;// 2KHz
	BYTE	cnt_3s_time_2k_switch 	= 0;// 2KHz
	BYTE	cnt_3s_time_led = 0;// 1Hz
	BYTE	cnt_3s_time_3_mode2 	= 0;// CN1/V
	BYTE	cnt_3s_time_4_mode2 	= 0;// CN1/H	
	BYTE	cnt_3s_time_34_after_ms 	= 0;// time elipse after mode switch

	BYTE	cnt_3s_time_startup 	= 0;// 
    BYTE	stepx = 0;
    BYTE	start = 0;
	
	BYTE    always_low_cnt = 0;
	BYTE    always_high_cnt = 0;
	BYTE    dat_bit_cnt = 0;

	BYTE    tmp_byte1 = 0;
	BYTE    tmp_byte2 = 0;
	BYTE    tmp_byte3 = 0;
	BYTE    tmp_byte4 = 0;
	BYTE   	ev1527_byte1 = 0;
	BYTE    ev1527_byte2 = 0;
	BYTE    ev1527_byte3 = 0;
	BYTE    ev1527_byte4 = 0;

#ifdef USE_10K
	WORD	count	=	112;
#endif
#ifdef USE_20K
	WORD	count	=	64;
#endif

	stt16	count;

	f_mode2 = 1;
	f_2k_on = 1;
	
	p_InA_V = 0;

	while (1)
	{
		if  (INTRQ.T16)// = 10KHz=100us
		{
			INTRQ.T16		=	0;
			stt16	count;
#if 0
			if (0 == count_x) {
				count_x = 1;
				p_OutB_V2 = 0;
			} else {
				count_x = 0;
				p_OutB_V2 = 1;
			}
#endif
#if 0
//			f_2k_on = 1;

			if (!p_InB_OD) {// LOW
				p_OutB_V2 = 0;
				p_InA_V = 0;
			} else {
				p_OutB_V2 = 1;
				p_InA_V = 1;
//				f_2k_on = 1;
			}
#if 0
			if (!p_InA_V) {// LOW
				p_OutB_V2 = 0;
			} else {
				p_OutB_V2 = 1;
//				f_2k_on = 1;
			}
			if (!p_InA_H) {// LOW
				p_OutB_V2 = 0;
//				f_2k_on = 1;
			} else {
				p_OutB_V2 = 1;
			}
			if (!p_InA_VJ) {// LOW
				p_OutB_V2 = 0;
			} else {
				p_OutB_V2 = 1;
//				f_2k_on = 1;
			}
#endif
#endif
			if (!f_ev1527_ok) {
				if (!p_InB_OD) {// LOW
//					p_InA_V = 0;
					always_low_cnt++;

					if (always_low_cnt >= 141) {
						always_low_cnt=0;
						always_high_cnt=0;
						dat_bit_cnt=0; 
						f_sync_ok=0; 
						tmp_byte1=0; 
						tmp_byte2=0; 
						tmp_byte3=0; 
						tmp_byte4=0;
					}

					f_last_level=0;
				} else {
//					p_InA_V = 1;

					if (!f_last_level) {
#if 0
						if (0 == count_x) {
							count_x = 1;
							p_OutB_V2 = 0;
						} else {
							count_x = 0;
							p_OutB_V2 = 1;
						}
#endif
						// always_high_cnt=1->2->3, 3-1=2: [200us,300us)
						// always_high_cnt=1->2->3->4->5, 5-1=4: [400us,500us)
						// always_high_cnt=[3,5] = [200us,500us)
						if (((always_high_cnt>=2)&&(always_high_cnt<=5))&&((always_low_cnt>=100)&&(always_low_cnt<=131))) {
							dat_bit_cnt=0; f_sync_ok=1; tmp_byte1=0; tmp_byte2=0; tmp_byte3=0; tmp_byte4=0;
//							p_InA_V = 1;
						} else if ((f_sync_ok)&&((always_low_cnt>=8)&&(always_low_cnt<=15))) {
							if ((always_high_cnt<2) || (always_high_cnt>5)) {
								p_InA_H = 1;
								always_low_cnt=0;
								always_high_cnt=0;
								dat_bit_cnt=0; f_sync_ok=0; tmp_byte1=0; tmp_byte2=0; tmp_byte3=0; tmp_byte4=0;
							} else {
#if 1
								if (0 == dat_bit_cnt) {
									p_InA_V = 1;
								}

								if (10 == dat_bit_cnt) {
									p_InA_V = 0;
								}

								if (15 == dat_bit_cnt) {
									p_InA_V = 1;
								}
								
								if (dat_bit_cnt >= 20) {
									p_InA_V = 0;
								}
#endif
								if (23 == dat_bit_cnt) {
									ev1527_byte1=tmp_byte1;ev1527_byte2=tmp_byte2;
									ev1527_byte3=tmp_byte3;ev1527_byte4=tmp_byte4;
									f_ev1527_ok=1;
								}

								dat_bit_cnt++;
								if (dat_bit_cnt > 24) {
									always_low_cnt=0;
									always_high_cnt=0;
									dat_bit_cnt=0; f_sync_ok=0; tmp_byte1=0; tmp_byte2=0; tmp_byte3=0; tmp_byte4=0;
								}
							}
						} else if ((f_sync_ok)&&((always_low_cnt>=2)&&(always_low_cnt<=5))) {
							if ((always_high_cnt<8) || (always_high_cnt>15)) {
								p_InA_H = 1;
								always_low_cnt=0;
								always_high_cnt=0;
								dat_bit_cnt=0; f_sync_ok=0; tmp_byte1=0; tmp_byte2=0; tmp_byte3=0; tmp_byte4=0;
							} else {
#if 1
								if (0 == dat_bit_cnt) {
									p_InA_V = 1;
								}

								if (10 == dat_bit_cnt) {
									p_InA_V = 0;
								}

								if (15 == dat_bit_cnt) {
									p_InA_V = 1;
								}
								
								if (dat_bit_cnt >= 20) {
									p_InA_V = 1;
								}
#endif
								switch (dat_bit_cnt) {
									case 0 : { tmp_byte1=tmp_byte1 | 0B10000000; break; }
									case 1 : { tmp_byte1=tmp_byte1 | 0B01000000; break; }
									case 2 : { tmp_byte1=tmp_byte1 | 0B00100000; break; }
									case 3 : { tmp_byte1=tmp_byte1 | 0B00010000; break; }
									case 4 : { tmp_byte1=tmp_byte1 | 0B00001000; break; }
									case 5 : { tmp_byte1=tmp_byte1 | 0B00000100; break; }
									case 6 : { tmp_byte1=tmp_byte1 | 0B00000010; break; }
									case 7 : { tmp_byte1=tmp_byte1 | 0B00000001; break; }
									case 8 : { tmp_byte2=tmp_byte2 | 0B10000000; break; }
									case 9 : { tmp_byte2=tmp_byte2 | 0B01000000; break; }
									case 10: { tmp_byte2=tmp_byte2 | 0B00100000; break; }
									case 11: { tmp_byte2=tmp_byte2 | 0B00010000; break; }
									case 12: { tmp_byte2=tmp_byte2 | 0B00001000; break; }
									case 13: { tmp_byte2=tmp_byte2 | 0B00000100; break; }
									case 14: { tmp_byte2=tmp_byte2 | 0B00000010; break; }
									case 15: { tmp_byte2=tmp_byte2 | 0B00000001; break; }
									case 16: { tmp_byte3=tmp_byte3 | 0B10000000; break; }
									case 17: { tmp_byte3=tmp_byte3 | 0B01000000; break; }
									case 18: { tmp_byte3=tmp_byte3 | 0B00100000; break; }
									case 19: { tmp_byte3=tmp_byte3 | 0B00010000; break; }
									case 20: { tmp_byte4=tmp_byte4 | 0B10000000; break; }
									case 21: { tmp_byte4=tmp_byte4 | 0B01000000; break; }
									case 22: { tmp_byte4=tmp_byte4 | 0B00100000; break; }
									case 23: {
											   tmp_byte4=tmp_byte4 | 0B00010000; 
											   ev1527_byte1=tmp_byte1;ev1527_byte2=tmp_byte2;
											   ev1527_byte3=tmp_byte3;ev1527_byte4=tmp_byte4;
											   f_ev1527_ok=1;
											   f_2k_on = 1;
											   break; 
									}
									default: { break; }
								} 

								dat_bit_cnt++;
								if (dat_bit_cnt > 24) {
									dat_bit_cnt=0; f_sync_ok=0; tmp_byte1=0; tmp_byte2=0; tmp_byte3=0; tmp_byte4=0;
								}
							}
						} else {
							p_OutB_LED = 1;
							dat_bit_cnt=0; f_sync_ok=0; tmp_byte1=0; tmp_byte2=0; tmp_byte3=0; tmp_byte4=0;
						}

						always_low_cnt=0;
						always_high_cnt=0;
					} else {
						// do nothing
					}

					if (always_high_cnt >= 16) {
						always_low_cnt=0;
						always_high_cnt=0;
						dat_bit_cnt=0; f_sync_ok=0; tmp_byte1=0; tmp_byte2=0; tmp_byte3=0; tmp_byte4=0;
					}

					always_high_cnt++;

					f_last_level=1;
				}
			} else {
				p_InA_V = 0;
			}
			
			if (f_ev1527_ok) {
				f_2k_on = 1;
				f_ev1527_ok = 0;
			}

			if (--count1 == 0)
			{
				count1		=	100;				//	100us * 100 = 10 ms 
				t16_10ms	=	1;
			}
/*			
			if (0 == count_l) {
				count_l = 1;
				p_OutB_V2 = 0;
			} else {
				count_l = 0;
				p_OutB_V2 = 1;
			}

			if (50 == count_l) {
				p_OutB_V2 = 0;
			}
*/
            count_l++;

            if (100 == count_l) {
                count_l = 0;
                count_h++;
                if (100 == count_h)
                    count_h = 0;
            }
        }

		while (t16_10ms)
		{
			t16_10ms	=	0;

			if (f_2k_on) {
				if (0 == cnt_3s_time_2k) {
					// Enable PWMG0C to 2KHz
                    pwmgcubl = 0b0000_0000;
					pwmgcubh = 0b0001_1111;
					
					pwmg2dtl = 0b1000_0000;
					pwmg2dth = 0b0000_1111;

					pwmg2c = 0b0000_0110;// PA3 PWM
                    pwmgclk = 0b1100_0000;// enable PWMG CLK(=SYSCLK/16)
				}

				cnt_3s_time_2k++;
				// ring 120ms
				if (10 == cnt_3s_time_2k) {
					f_2k_on = 0;
					cnt_3s_time_2k = 0;
                    
					// Disable 2KHz
                    pwmg2c = 0b0000_0000;// do not output PWM
	                p_OutA_2K	=	0;
				}
			}

			break;
		}
	}
}
