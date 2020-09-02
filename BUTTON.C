#include	"extern.h"

//#define USE_5K 1
#define USE_10K 1

void pwmgx_enable_2K(void);
void pwmg2_enable_2K_50(void);
void pwmg2_disable(void);
void tm2_enable_50K();

BIT		p_InA_OD	:	PA.4;
BIT		p_InA_VJ	:	PA.5;
BIT		p_InA_V	    :	PA.7;
BIT		p_InB_H	    :	PB.1;
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
	.ADJUST_IC	SYSCLK=IHRC/2		//	SYSCLK=IHRC/4
    
    $ EOSCR		DIS_LVD_BANDGAP;

	$	p_OutA_2K		    Out, Low;
	$	p_OutB_V1		    Out, Low;
	$	p_OutB_V2		    Out, Low;
    $	p_OutA_V3		    Out, Low;
	$	p_OutB_H1		    Out, Low;
    $	p_OutB_LED		    Out, Low;// off

	$	p_InA_OD		    In;
	$	p_InA_VJ		    In;
	$	p_InA_V		        In;
	$	p_InB_H		        In;
    $	p_InA_QV2		    In;
    $	p_InB_QV3		    In;

	// IN Pull-UP
    PAPH		=		_FIELD(p_InA_OD, p_InA_V);
    PBPH		=		_FIELD(p_InB_H);
    
	$ T16M		IHRC, /4, BIT8;
//	$ T16M		IHRC, /1, BIT8;				// 32us
//	$ T16M		IHRC, /1, BIT11;			// 256us
//	$ T16M		IHRC, /1, BIT10;			// 128us
	$ TM2C		IHRC, Disable, Period, Inverse;

	BYTE	Key_Flag;
	Key_Flag			=	_FIELD(p_InA_OD, p_InA_VJ, p_InA_V, p_InB_H, p_InA_QV2, p_InB_QV3);

	BYTE	Sys_Flag	=	0;
	BIT		f_mode2				:	Sys_Flag.0;
	BIT		f_od_switch_on		:	Sys_Flag.1;// OD switch status
	BIT		f_Trig_lpress_OD	:	Sys_Flag.2;// OD long press
	BIT		f_Trig_spress_OD	:	Sys_Flag.3;// OD short press
	
	BYTE	Sys_FlagA	=	0;
	BIT		t16_10ms	:	Sys_FlagA.1;
	BIT		f_Key_Trig3	:	Sys_FlagA.3;
	BIT		f_Key_Trig4	:	Sys_FlagA.4;
    BIT		f_IN_QV2	:	Sys_FlagA.5;
    BIT		f_IN_QV3	:	Sys_FlagA.6;
    BIT		t_128us  	:	Sys_FlagA.7;    	

	BYTE	Sys_FlagB	=	0;
	BIT		f_2k_on			:	Sys_FlagB.1;
	BIT		f_rst_all_flag	:	Sys_FlagB.2;
	BIT		f_led_state		:	Sys_FlagB.3;
	BIT		f_vj_low2high	:	Sys_FlagB.5;
	
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
	
//	pmode	Program_Mode;
//	fppen	=	0xFF;

	BYTE	count_128us = 1;
	BYTE	count_1ms = 1;
	BYTE	count_10ms = 1;
	BYTE	count_l = 0;
//	BYTE    last_vj_state = 8;
	BYTE    last_vj_state = 1;

	// for long press
	BYTE	debounce_time_lpress_OD		=	130;				// Key debounce time = 250 ms
	BYTE	debounce_time_lpress_V		=	250;				// Key debounce time = 250 mS
	BYTE	debounce_time_lpress_H		=	250;				// Key debounce time = 250 mS

	BYTE	cnt_3s_time_4 	= 0;// CN1/H
	BYTE	cnt_3s_time_2k 	= 0;// 2KHz
	BYTE	cnt_3s_time_led = 19;// 1Hz

	BYTE	cnt_3s_time_startup 	= 0;
    BYTE	stepx = 0;
    BYTE	start = 0;
    BYTE	mid_val = 6;
    BYTE	end_val = 10;
    BYTE	end_val_cmp = 12;
	
	// TM16 Period = 256us -> produce 50us Interrupt
	// 65536 - 52736 =  12800
	// 12800 / 65536 = 25/128
	// 50us / 256us = 25/128
	//WORD	count	=	52735;
	
	WORD	count	=	56;
	stt16	count;
	
	f_mode2 = 0;
	f_2k_on = 0;

#ifdef USE_10K 
	mid_val = 6;
	end_val = 10;
#endif

#ifdef USE_5K 
	mid_val = 11;
	end_val = 20;
#endif

	end_val_cmp = end_val + 2;

	// Enable 50KHz to be the basement to timing control 100KHz
	// tm2_enable_50K();

	while (1)
	{
		if (INTRQ.T16) {// TM16 Interrupt 20KHz (=50us)
			INTRQ.T16		=	0;
			stt16	count;
			
			if (--count_1ms == 0) {
				count_1ms		=	20;				// 50us * 20 = 1 ms

				if (--count_10ms == 0) {
					count_10ms		=	10;				// 1ms * 10 = 10 ms
					t16_10ms	=	1;
				}
			}

			if (--count_128us == 0) {
				count_128us		=	3;				// 50us * 3 = 150 us
				t_128us         =   1;
			}

            // 1->end_val
            //count_l++;

            //if (end_val_cmp == count_l) {
            //    count_l = 0;
			//}
			
			if (0 == count_l) {
				count_l = 1;
			} else {
				count_l = 0;
			}
		}

		// time period 20KHz(=50us)
		if (f_od_switch_on) {
			if (f_mode2) {
#if 1
				if (1 == count_l) {
					if (f_V1_on) {
						p_OutB_V1 = 1;
					}
					if (f_V3_on) {
						p_OutA_V3 = 1;
					}
					p_OutB_H1 = 0;
					p_OutB_V2 = 0;
				} else {
					p_OutB_V1 = 0;
					p_OutA_V3 = 0;
					if (f_V2_on) {
						p_OutB_V2 = 1;
					}
					if (f_H1_on) {
						p_OutB_H1 = 1;
					}
				}
#else
				if (1 == count_l) {
					if (f_V1_on) {
						p_OutB_V1 = 1;
					}
					if (f_V3_on) {
						p_OutA_V3 = 1;
					}
	                p_OutB_H1 = 0;
	                p_OutB_V2 = 0;
	            } else if (mid_val == count_l) {
					p_OutB_V1 = 0;
					p_OutA_V3 = 0;
					if (f_V2_on) {
	                    p_OutB_V2 = 1;
					}
					if (f_H1_on) {
						p_OutB_H1 = 1;
					}
				} else if (end_val == count_l) {
					count_l = 0;
				}
#endif
			}
		}

		// time period 130us
        if (t_128us) {
			t_128us  = 0;

            if (t16_10ms) {
				if (f_od_switch_on) {
					if (cnt_3s_time_startup < 250) {
						cnt_3s_time_startup++;
					}
    	
					if (65 == cnt_3s_time_startup) {
						if (!f_mode2) {
							p_OutB_H1	=	1;
						}
						
						f_V1_on = 0;
						f_V2_on = 0;
						f_H1_on = 1;
						f_V3_on = 0;
						stepx = 1;
						f_2k_on = 1;
					} else if (110 == cnt_3s_time_startup) {
						if (1 == stepx) {
							if (!f_mode2) {
								p_OutB_V1 = 1;
							}
							
							f_V1_on = 1;
							
							stepx = 2;
							f_2k_on = 1;
						}
					} else if (155 == cnt_3s_time_startup) {
						if (2 == stepx) {
							// Not a Key -> no need to do debounce
							if (p_InA_QV2) {// HIGH
								f_IN_QV2 = 1;
							} else {
								f_IN_QV2 = 0;
							}
							
							if (f_IN_QV2) {
								if (!f_mode2) {
		                            p_OutB_V2 = 1;
								}
								
								f_V2_on = 1;
								stepx = 3;
								f_2k_on = 1;
							} else {
								f_V1_on = 0;
								p_OutB_V1 = 0;
								start = 1;
								stepx = 1;
							}
						 }
					} else if (200 == cnt_3s_time_startup) {
						if ((3 == stepx) && (0 == start)) {
							// Not a Key -> no need to do debounce
		                    if (p_InB_QV3) {// HIGH
								f_IN_QV3 = 1;
							} else {
								f_IN_QV3 = 0;
							}
							
							if (f_IN_QV3) {
								if (!f_mode2) {
									p_OutA_V3 = 1;
								}
								
								f_V3_on = 1;
								
								stepx = 4;
								f_2k_on = 1;
							} else {
								f_V1_on = 0;
								f_V2_on = 0;
								
								p_OutB_V1 = 0;
		                        p_OutB_V2 = 0;
								start = 1;
								stepx = 1;
							}
						}
					} else if (249 == cnt_3s_time_startup) {
						if ((4 == stepx) && (0 == start)) {
							f_V1_on = 0;
							f_V2_on = 0;
							f_V3_on = 0;
							
							p_OutB_V1 = 0;
		                    p_OutB_V2 = 0;
							p_OutA_V3 = 0;
							
							stepx = 1;
							start = 1;
						}
					}
				}

				A =	(PA ^ Key_Flag) & _FIELD(p_InA_OD);
				if (! ZF) {
					// Active: H->L
					if (!p_InA_OD) {
						if (--debounce_time_lpress_OD == 0) {
							Key_flag ^=	_FIELD(p_InA_OD);
							if (f_od_switch_on) {
								f_Trig_lpress_OD = 1;
							}
							debounce_time_lpress_OD = 130;
						}
					} else {// Up: H->L
						Key_flag ^=	_FIELD(p_InA_OD);
					}
				} else {
					if (debounce_time_lpress_OD < 125) {
						Key_flag ^=	_FIELD(p_InA_OD);
						f_Trig_spress_OD = 1;// short push
					}
					debounce_time_lpress_OD = 130;
				}

				if ((f_Trig_spress_OD) || (f_Trig_lpress_OD)) {
					// short press -> enable / disable other buttons
					if (f_Trig_spress_OD) {
						f_Trig_spress_OD = 0;

						if (f_od_switch_on) {
							f_od_switch_on = 0;
							f_rst_all_flag = 1;
						} else {
							f_mode2 = 0;
							f_od_switch_on = 1;
						}
					}

					// long press -> switch laser mode
					if (f_Trig_lpress_OD) {
						f_Trig_lpress_OD = 0;
						//f_rst_all_flag = 1;

						if (f_mode2) {
							f_mode2 = 0;
							if (f_V1_on) {
								p_OutB_V1 = 1;
							} else {
								p_OutB_V1 = 0;
							}
							if (f_V2_on) {
								p_OutB_V2 = 1;
							} else {
								p_OutB_V2 = 0;
							}
							if (f_V3_on) {
								p_OutA_V3 = 1;
							} else {
								p_OutA_V3 = 0;
							}
							if (f_H1_on) {
								p_OutB_H1 = 1;
							} else {
								p_OutB_H1 = 0;
							}
						} else {
							f_mode2 = 1;
						}
					}

					if (f_rst_all_flag) {
						f_rst_all_flag = 0;
									
						p_OutB_V1 = 0;
						p_OutB_V2 = 0;
						p_OutA_V3 = 0;
						p_OutB_H1 = 0;
									
						count_l = 0;
									
						Sys_FlagA = 0;
						Sys_FlagB = 0;
						Sys_FlagC = 0;
						Sys_FlagD = 0;
									
//						last_vj_state = 8;
						last_vj_state = 1;								
									
						debounce_time_lpress_OD	=	130;	
						debounce_time_lpress_V	=	250;	
						debounce_time_lpress_H	=	250;	
									
						cnt_3s_time_4 	= 0;// CN1/H
						cnt_3s_time_2k 	= 0;// 2KHz
						cnt_3s_time_led = 19;// 1Hz
									
						stepx = 0;
						start = 0;
					
						cnt_3s_time_startup = 0;
									
						p_OutB_LED = 0;
					}

					f_2k_on = 1;
				}
			}
			
			if ((f_od_switch_on) && (1 == start)) {
				// time period 10ms
				if (t16_10ms) {
					A = (PA ^ Key_flag) & _FIELD(p_InA_V);
					if (! ZF) {
						// Active: H->L
						if (!p_InA_V) {
							if (--debounce_time_lpress_V == 0) {
								Key_flag ^=	_FIELD(p_InA_V);
								debounce_time_lpress_V = 250;
							}
						} else {// Up: H->L
							Key_flag ^=	_FIELD(p_InA_V);
						}
					} else {
						if (debounce_time_lpress_V < 245) {
							Key_flag ^=	_FIELD(p_InA_V);
							f_Key_Trig3 = 1;// short push
						}
						debounce_time_lpress_V = 250;
					}
                    	
					A = (PB ^ Key_flag) & _FIELD(p_InB_H);
					if (! ZF) {
						// Active: H->L
						if (!p_InB_H) {
							if (--debounce_time_lpress_H == 0) {
								Key_flag ^=	_FIELD(p_InB_H);
								debounce_time_lpress_H = 250;
							}
						} else {// Up: H->L
							Key_flag ^=	_FIELD(p_InB_H);
						}
					} else {
						if (debounce_time_lpress_H < 245) {
							Key_flag ^=	_FIELD(p_InB_H);
							f_Key_Trig4 = 1;// short push
						}
						debounce_time_lpress_H = 250;
					}	
   				}

				// time period 100KHz(=10us)
				// VJ Active -> LED Always Twinkle(190ms High + 190ms Low)
				// VJ Deactive -> change "LED Twinkle" to "LED Always ON"
				if (p_InA_VJ) {// H is special mode
					// if cnt_3s_time_led != 19 -> VJ(/LED Twinkle) is during activing, no need to restart
					if ((0 == last_vj_state) && (cnt_3s_time_led == 19)) {
						f_vj_low2high = 1;
					}

					if ((last_vj_state != 1) && (cnt_3s_time_led == 19)) {
						last_vj_state = 1;
									
						// Disable all 100HZ PWM
						p_OutB_V1	=	0;
						p_OutB_V2	=	0;
						p_OutA_V3   =   0;
						p_OutB_H1	=	0;
							
						if (f_V1_on) {
							f_last_V1_on = 1;
						} else {
							f_last_V1_on = 0;
						}
								
						if (f_V2_on) {
							f_last_V2_on = 1;
						} else {
							f_last_V2_on = 0;
						}
								
						if (f_H1_on) {
							f_last_H1_on = 1;
						} else {
							f_last_H1_on = 0;
						}
								
						if (f_V3_on) {
							f_last_V3_on = 1;
						} else {
							f_last_V3_on = 0;
						}
								
						f_V1_on = 0;
						f_V2_on = 0;
						f_H1_on = 0;
						f_V3_on = 0;
					}
				} else {// L is normal mode
					// the 1st place to update last_vj_state's actual value
					if ((last_vj_state != 0) && (cnt_3s_time_led == 19)) {
						last_vj_state = 0;
							
						f_vj_low2high = 0;
							
						if (f_last_V1_on) {
							if (!f_mode2) {
								p_OutB_V1	=	1;
							}
									
							f_V1_on = 1;
						}
						if (f_last_V2_on) {
							if (!f_mode2) {
								p_OutB_V2	=	1;
							}
								
							f_V2_on = 1;
						}
						if (f_last_H1_on) {
							if (!f_mode2) {
								p_OutB_H1	=	1;
							}
									
							f_H1_on = 1;
						}
						if (f_last_V3_on) {
							if (!f_mode2) {
								p_OutA_V3	=	1;
							}
										
							f_V3_on = 1;
						}
					}
				}

				// time period 10ms
				if (t16_10ms) {					
					if (f_Key_Trig3)// CN1/V
					{
						p_OutB_LED = 1;
						f_Key_Trig3 = 0;
						f_2k_on = 1;
                    	
						// disable V when VJ is L -> H
						if (!f_vj_low2high) {
							if (1 == stepx) {
								if (!f_mode2) {
									p_OutB_V1 = 1;
								}
									
								f_V1_on = 1;
									
								stepx = 2;
							} else if (2 == stepx) {
								// Not a Key -> no need to do debounce
								if (p_InA_QV2) {// HIGH
									f_IN_QV2 = 1;
								} else {
									f_IN_QV2 = 0;
								}
									
								if (f_IN_QV2) {
									if (!f_mode2) {
                	                    p_OutB_V2 = 1;
									}
										
									f_V2_on = 1;
									stepx = 3;
								} else {
									f_V1_on = 0;
									p_OutB_V1 = 0;
										
									stepx = 1;
								}
							} else if (3 == stepx) {
								// Not a Key -> no need to do debounce
                	            if (p_InB_QV3) {// HIGH
									f_IN_QV3 = 1;
								} else {
									f_IN_QV3 = 0;
								}
									
								if (f_IN_QV3) {
									if (!f_mode2) {
										p_OutA_V3 = 1;
									}
										
									f_V3_on = 1;
										
									stepx = 4;
								} else {
									f_V1_on = 0;
									f_V2_on = 0;
										
									p_OutB_V1 = 0;
                	                p_OutB_V2 = 0;
									
									stepx = 1;
								}
							} else if (4 == stepx) {
								f_V1_on = 0;
								f_V2_on = 0;
								f_V3_on = 0;
									
								p_OutB_V1 = 0;
                	            p_OutB_V2 = 0;
								p_OutA_V3 = 0;
									
								stepx = 1;
							}
						}
					}
                    	
					if (f_Key_Trig4)// CN1/H
					{
						p_OutB_LED = 1;
						f_Key_Trig4 = 0;
						f_2k_on = 1;
                    	
						// disable V when VJ is L -> H
						if (!f_vj_low2high) {
							cnt_3s_time_4++;
			        	
							if (1 == cnt_3s_time_4) {
								if (f_H1_on) {// Current is ON
									f_H1_on = 0;
									p_OutB_H1 = 0;
								} else {// Current is OFF
									f_H1_on = 1;
										
									if (!f_mode2) {
										p_OutB_H1 = 1;
									}
								}
							} else if (2 == cnt_3s_time_4) {
								cnt_3s_time_4 = 0;
									
								if (f_H1_on) {// Current is ON
									f_H1_on = 0;
									p_OutB_H1 = 0;
								} else {// Current is OFF
									f_H1_on = 1;
										
									if (!f_mode2) {
										p_OutB_H1 = 1;
									}
								}
							}
						}
					}
				}	
					
				// time period 100KHz(=10us)
				// VJ Active -> LED Always Twinkle(190ms High + 190ms Low)
				// VJ Deactive -> change "LED Twinkle" to "LED Always ON"
				if (f_vj_low2high) {
					// 19->0, 0->19
					if (19 == cnt_3s_time_led) {
						if (f_led_state) {
							p_OutB_LED = 0;
							f_led_state = 0;
						} else {
							p_OutB_LED = 1;
							f_led_state = 1;
						}
								
						f_2k_on = 1;
						cnt_3s_time_led = 0;
					}

					if (t16_10ms) {
				        cnt_3s_time_led++;
					}
				} else {
					p_OutB_LED = 1;
					cnt_3s_time_led = 19;
				}
			}

			// The only place close 2KHz is after 100ms's ring, it close automatically
			// if 2K is during activing, new request will ignore
			if (f_2k_on) {
				if (0 == cnt_3s_time_2k) {
					// Enable 2KHz PWM
					pwmgx_enable_2K();
					pwmg2_enable_2K_50();
				}

				if (t16_10ms) {
				    cnt_3s_time_2k++;
				}

				// ring 100ms
				if (10 == cnt_3s_time_2k) {
					// The only place close 2KHz
					f_2k_on = 0;
					cnt_3s_time_2k = 0;
					// Disable 2KHz PWM
					pwmg2_disable();
		            p_OutA_2K	=	0;
				}
			}

            t16_10ms = 0; 
		}
	}
}

// PWMG0/1/2 Share the same Freq but different duty ratio
// Setting PWM's Freq to 2KHz
// Fpwm_freq = Fpwm_clk / (CB + 1) = 4M/8/250 = 2KHz
// Fpwm_freq = Fpwm_clk / (CB + 1) = 8M/16/250 = 2KHz
// [6:4]: 000-1,  001-2,  010-4,  011-8,  
//        100-16, 101-32, 110-64, 111-128
void pwmgx_enable_2K(void)
{
	pwmgcubl = 0b0100_0000;
	pwmgcubh = 0b0011_1110;// CB = {pwmgcubh[7:0], pwmgcubl[7:6]} = 249
	
	//pwmgclk = 0b1011_0000;// Fpwm_clk = = SYSCLK / 8 = 4M/8
	pwmgclk = 0b1100_0000;// Fpwm_clk = = SYSCLK / 16 = 8M/16
}

// Enable PWMG2 Output with 50% duty ratio at 2kHz
void pwmg2_enable_2K_50(void)
{
	pwmg2dtl = 0b0010_0000;// DB0 = pwmg0dtl[5] = 1
	pwmg2dth = 0b0001_1111;// DB10_1 = {pwmg0dth[7:0], pwmg0dtl[7:6]}

	// Fpwm_duty = [DB10_1 + DB0*0.5 + 0.5] / (CB + 1) = (DB10_1 + 1) / 250
	pwmg2c = 0b0000_0110;// PA3 PWM
}

// Disable PWMG2
void pwmg2_disable(void)
{
	pwmg2c = 0b0000_0000;// disable PWMG2
	pwmgclk = 0b0011_0000;// disable PWMGX
}

// Enable Timer2 PWM to 50KHz -> Interrupt = 100KHz(one period have 2 interrupt)
// freq = Fsrc / 2 / (K+1) / S1 / (S2+1) = 4M / 2 / 1 / 4 / 10 = 50KHz
void tm2_enable_50K()
{
	tm2ct = 0x0;
	tm2b = 0b0000_0000;// K
    tm2s = 0b0_01_01001;// S1=^-^[6:5]=(01->4), S2=[4:0]=9
	tm2c = 0b0001_0000;// CLK(=IHRC/4) | Disable Output | Period | Disable Inverse
}
