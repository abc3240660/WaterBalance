#include	"extern.h"

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
	.ADJUST_IC	SYSCLK=IHRC/4		//	SYSCLK=IHRC/4
    
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
    
	$ T16M		IHRC, /64, BIT11;			// 2us
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
	BIT		f_Key_Trig2	:	Sys_FlagA.2;
	BIT		f_Key_Trig3	:	Sys_FlagA.3;
	BIT		f_Key_Trig4	:	Sys_FlagA.4;
    BIT		f_IN_QV2	:	Sys_FlagA.5;
    BIT		f_IN_QV3	:	Sys_FlagA.6;
    BIT		t_128us  	:	Sys_FlagA.7;    	

	BYTE	Sys_FlagB	=	0;
	BIT		f_2k_on			:	Sys_FlagB.1;
	BIT		f_led_flash		:	Sys_FlagB.2;
	BIT		f_led_state		:	Sys_FlagB.3;
	BIT		f_vj_on			:	Sys_FlagB.4;
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

	BYTE	count1 = 1;
	BYTE	count_l = 0;
	BYTE	count_h = 0;
//	BYTE    last_vj_state = 8;
	BYTE    last_vj_state = 1;

	// for long press
	BYTE	debounce_time_lpress_OD		=	250;				// Key debounce time = 250 ms
	BYTE	debounce_time_lpress_V		=	250;				// Key debounce time = 250 mS
	BYTE	debounce_time_lpress_H		=	250;				// Key debounce time = 250 mS

	BYTE	cnt_3s_time_4 	= 0;// CN1/H
	BYTE	cnt_3s_time_2k 	= 0;// 2KHz
	BYTE	cnt_3s_time_led = 19;// 1Hz

	BYTE	cnt_3s_time_startup 	= 0;// 
    BYTE	stepx = 0;
    BYTE	start = 0;
    
	f_mode2 = 0;
	f_2k_on = 0;

	while (1)
	{
		if (INTRQ.T16)
		{
			INTRQ.T16		=	0;
			t_128us         =   1;
			If (--count1 == 0)					//	DZSN  count
			{
//				count1		=	78;				//	128uS * 78 = 10 mS 
				count1		=	39;				//	128uS * 78 = 10 mS 				
				t16_10ms	=	1;
			}

            // 1->78
            count_l++;
            
            if (100 == count_l) {
                count_l = 0;
                count_h++;
                if (100 == count_h)
                    count_h = 0;
			}
		}

		if (f_od_switch_on) {
			if (f_mode2) {
				if ((1 == count_l)&&(0 == count_h)) {
					if (f_V1_on) {
						p_OutB_V1 = 1;
					}
					if (f_V3_on) {
						p_OutA_V3 = 1;
					}
	                p_OutB_H1 = 0;
	                p_OutB_V2 = 0;
	            } else if ((40 == count_l)&&(0 == count_h)) {
					p_OutB_V1 = 0;
					p_OutA_V3 = 0;
					if (f_V2_on) {
	                    p_OutB_V2 = 1;
					}
					if (f_H1_on) {
						p_OutB_H1 = 1;
					}
				} else if ((78 == count_l)&&(0 == count_h)) {
					count_l = 0;
					count_h = 0;
				}
			}
		}

//		while (t16_10ms)
        while (t_128us)
		{
			t_128us  = 0;
//			t16_10ms	=	0;

            if (t16_10ms) {
				if (f_od_switch_on) {
					if (cnt_3s_time_startup < 250) {
						cnt_3s_time_startup++;
					}
    	
		//			if (50 == cnt_3s_time_startup) {
		//				f_2k_on = 1;
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
			}	

			if (1) {
				if (t16_10ms) {
					A =	(PA ^ Key_Flag) & _FIELD(p_InA_OD);
					if (! ZF) {
						// Active: H->L
						if (!p_InA_OD) {
							if (--debounce_time_lpress_OD == 0) {
								Key_flag ^=	_FIELD(p_InA_OD);
								if (f_od_switch_on) {
									f_Trig_lpress_OD = 1;
								}
								debounce_time_lpress_OD = 250;
							}
						} else {// Up: H->L
							Key_flag ^=	_FIELD(p_InA_OD);
						}
					} else {
						if (debounce_time_lpress_OD < 245) {
							Key_flag ^= FIELD(p_InA_OD);
							f_Trig_spress_OD = 1;// short push
						}
						debounce_time_lpress_OD = 250;
					}
				}

				// short press -> enable / disable other buttons
				if (f_Trig_spress_OD) {
					f_Trig_spress_OD = 0;

					if (f_od_switch_on) {
						f_od_switch_on = 0;
									
						// TBD: disable PWM

						p_OutB_V1 = 0;
						p_OutB_V2 = 0;
						p_OutA_V3 = 0;
						p_OutB_H1 = 0;
									
						count1 = 1;
						count_l = 0;
						count_h = 0;
									
						Sys_FlagA = 0;
						Sys_FlagB = 0;
						Sys_FlagC = 0;
						Sys_FlagD = 0;
									
//						last_vj_state = 8;
						last_vj_state = 1;								
									
						debounce_time_lpress_OD	=	250;	
						debounce_time_lpress_V	=	250;	
						debounce_time_lpress_H	=	250;	
									
						cnt_3s_time_4 	= 0;// CN1/H
						cnt_3s_time_2k 	= 0;// 2KHz
						cnt_3s_time_led = 19;// 1Hz
									
						stepx = 0;
						start = 0;
									
						cnt_3s_time_startup = 0;
									
						p_OutB_LED = 0;
					} else {
						f_od_switch_on = 1;
					}
								
					f_2k_on = 1;
				}

				// long press -> switch laser mode
				if (f_Trig_lpress_OD) {
					f_Trig_lpress_OD = 0;

					if (f_mode2) {
						f_mode2 = 0;
					} else {
						f_mode2 = 1;
					}
				}
			
				if ((f_od_switch_on) && (1 == start)) {
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

					if (p_InA_VJ) {// H is special mode
						if ((0 == last_vj_state) && (cnt_3s_time_led == 19)) {
							f_vj_on = 1;
							f_led_flash = 1;
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
					} else {
						if ((last_vj_state != 0) && (cnt_3s_time_led == 19)) {
							last_vj_state = 0;
							
							f_vj_on = 0;
							f_led_flash = 0;
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
					
					if (f_led_flash) {
						if (19 == cnt_3s_time_led) {
							if (f_led_state) {
								p_OutB_LED = 0;
								f_led_state = 0;
								
								if (f_vj_on) {
									// 2kHz
									f_2k_on = 1;
								}
							} else {
								p_OutB_LED = 1;
								f_led_state = 1;
								
								if (f_vj_on) {
									// 2kHz
									f_2k_on = 1;
								}
							}
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
			}

			if (f_2k_on) {
				if (0 == cnt_3s_time_2k) {
					// Enable Timer2 PWM to 2KHz
					tm2ct = 0x0;
					tm2b = 0b1011_0110;// 124
					//tm2s = 0b000_01111;// 15
                    tm2s = 0b000_00011;// 7
					tm2c = 0b0001_1000;// CLK(=IHRC/2) | PA3 | Period | Disable Inverse
				}

				if (t16_10ms) {
				    cnt_3s_time_2k++;
				}
				// ring 120ms
				if (10 == cnt_3s_time_2k) {
					f_2k_on = 0;
					cnt_3s_time_2k = 0;
					// Disable Timer2 PWM
					tm2c = 0b0000_0000;// IHRC | PA3 | Period | Disable Inverse
		            p_OutA_2K	=	0;
				}
			}

            t16_10ms = 0; 
			break;
		}
	}
}
