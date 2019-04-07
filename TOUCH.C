#include	"extern.h"

// H-PB2;V1-PB1;V2-PB0;OD-PA4;DTA-PB7；VJ-PA5;
BIT		p_InA_OD	:	PA.4;
BIT		p_InA_VJ	:	PA.5;
BIT		p_InB_H	    :	PB.2;
BIT		p_InB_V1	:	PB.1;
BIT		p_InB_DTA	:	PB.7;
BIT		p_InB_V2	:	PB.0;

// 蜂鸣器-PA3；H'-PB6;V1'-PA6;V2'-PB5
BIT		p_OutA_2K	:	PA.3;
BIT		p_OutB_V2   :	PB.5;
BIT		p_OutA_V1   :	PA.6;
BIT		p_OutB_H1   :	PB.6;

// High is active
BIT		p_OutB_LED	:	PB.7;// LED

void	FPPA0 (void)
{
	.ADJUST_IC	SYSCLK=IHRC/4		//	SYSCLK=IHRC/4
    
    $ EOSCR		DIS_LVD_BANDGAP;

	$	p_OutA_2K		    Out, Low;
	$	p_OutA_V1		    Out, Low;
	$	p_OutB_V2		    Out, Low;
	$	p_OutB_H1		    Out, Low;
    $	p_OutB_LED		    Out, High;// off

	$	p_InA_OD		    In;
	$	p_InA_VJ		    In;
	$	p_InB_H		        In;
	$	p_InB_V1		    In;
    $	p_InB_DTA		    In;
    $	p_InB_V2		    In;

	// IN Pull-UP
    PAPH		=		_FIELD(p_InA_OD, p_InB_H);
    PBPH		=		_FIELD(p_InB_V1);

//	$ T16M		IHRC, /1, BIT11;			//	16MHz / 1 = 16MHz : the time base of T16.
	$ T16M		IHRC, /1, BIT10;			//	16MHz / 1 = 16MHz : the time base of T16.	
	$ TM2C		IHRC, Disable, Period, Inverse;

	BYTE	Key_Flag;
	Key_Flag			=	_FIELD(p_InA_OD, p_InA_VJ, p_InB_H, p_InB_V1, p_InB_DTA, p_InB_V2);

	BYTE	Sys_Flag	=	0;
	BIT		f_OD_LP_Trig	:	Sys_Flag.0;
	BIT		t16_10ms	:	Sys_Flag.1;
	BIT		f_Key_TrigV1	:	Sys_Flag.2;
	BIT		f_Key_TrigH	:	Sys_Flag.3;
	BIT		f_Key_TrigV2	:	Sys_Flag.4;
    BIT		t_128us  	:	Sys_Flag.7;    	

	BYTE	Sys_FlagB	=	0;
	BIT		f_mode2	:	Sys_FlagB.0;
	BIT		f_2k_on	:	Sys_FlagB.1;
	BIT		f_led_flash	:	Sys_FlagB.2;
	BIT		f_vj_on	:	Sys_FlagB.5;
//	BIT		f_2k_flag	:	Sys_FlagB.6;	
	
	BYTE	Sys_FlagC	=	0;
	BIT		f_V1_on	:	Sys_FlagC.0;
	BIT		f_V2_on	:	Sys_FlagC.1;
	BIT		f_H1_on	:	Sys_FlagC.2;
	BIT		f_OD_disable	:	Sys_FlagC.3;
	BIT		f_H_disable		:	Sys_FlagC.4;
	BIT		f_V1_disable	:	Sys_FlagC.5;
	BIT		f_V2_disable	:	Sys_FlagC.6;

	BYTE	Sys_FlagD	=	0;
	BIT		f_last_V1_on	:	Sys_FlagD.0;
	BIT		f_last_V2_on	:	Sys_FlagD.1;
	BIT		f_last_H1_on	:	Sys_FlagD.2;
	BIT     f_OD_SP_sta    :   Sys_FlagD.4;
    BIT     f_sync_trig     :   Sys_FlagD.7;

    BYTE    sync_sta = 0;
    BYTE    data_sta = 0;
    BYTE    l_cnt_sync = 0;
    BYTE    h_cnt_sync = 0;
//    BYTE    l_cnt_dat = 0;
    BYTE    h_cnt_dat = 0;
	
	BYTE    dat_bit_cnt = 0;
    BYTE    l_low_cnt = 0;

//	pmode	Program_Mode;
//	fppen	=	0xFF;

	BYTE	t16_flag;
	BYTE	count1 = 1;
	BYTE	count_l = 0;
	BYTE	count_h = 0;
	BYTE    last_vj_state = 8;
	BYTE    f_2k_flag = 0;

	BYTE    od_disable_cnt = 0;
	BYTE    h_disable_cnt = 0;
	BYTE    v1_disable_cnt = 0;
	BYTE    v2_disable_cnt = 0;

	BYTE	cnt_Key_10ms_1	=	150;			//	Key debounce time = 40 mS
	BYTE	cnt_Key_10ms_2	=	4;				//	Key debounce time = 40 mS
	BYTE	cnt_Key_10ms_3	=	4;				//	Key debounce time = 40 mS
	BYTE	cnt_Key_10ms_4	=	4;				//	Key debounce time = 40 mS
	
	BYTE	cnt_Key_10ms_3_mode2	=	250;				//	Key debounce time = 40 mS
	BYTE	cnt_Key_10ms_4_mode2	=	250;				//	Key debounce time = 40 mS
	BYTE	cnt_Key_10ms_5_mode2	=	250;				//	Key debounce time = 40 mS
	
	BYTE	cnt_3s_time_1 	= 0;
	BYTE	cnt_3s_time_v1 	= 0;// V1
	BYTE	cnt_3s_time_v2 	= 0;// V2
	BYTE	cnt_3s_time_h 	= 0;// H
	BYTE	cnt_3s_time_2k 	= 0;// 2KHz
	BYTE	cnt_3s_time_2k_switch 	= 0;// 2KHz
	BYTE	cnt_3s_time_led = 19;// 1Hz

	BYTE	cnt_3s_time_startup 	= 0;// 
    BYTE	stepx = 0;
	BYTE	stepxx = 1;
    BYTE	start = 0;
    BYTE	pwd_cnt = 0;
	
	f_mode2 = 0;
	f_2k_on = 0;

	while (1)
	{
		if  (INTRQ.T16)
		{
			INTRQ.T16		=	0;
			
            if (!p_InB_DTA) {// LOW
			    if (f_sync_trig) {
                    l_cnt_sync = 0;
                    h_cnt_sync = 0;
					l_low_cnt++;
				
                    if (1 == l_low_cnt) {
						if ((h_cnt_dat>1) && (h_cnt_dat<4)) {// 0.256ms ~ 0.512ms H
							data_sta = 0;
							dat_bit_cnt++;
						} else if ((h_cnt_dat>6) && (h_cnt_dat<9)) {// 0.896ms ~ 1.024ms H
							data_sta = 1;
							dat_bit_cnt++;
						} else {
							dat_bit_cnt = 0;
							f_sync_trig = 0;// need re-sync
						}
					}
//                    h_cnt_dat = 0;

					if (21 == dat_bit_cnt) {
						if (data_sta) {
							if (!f_OD_disable) {
								if (f_OD_SP_sta) {
									f_OD_SP_sta = 0;// short push
								} else {
									f_OD_SP_sta = 1;// short push
								}
							}
						} else {
							//p_Out2_H = 0;
						}
					} else if (22 == dat_bit_cnt) {
						if (data_sta) {
							if (!f_V2_disable) {
								f_Key_TrigV2 = 1;
							}
						} else {
							//p_Out6_V1 = 0;
						}
					} else if (23 == dat_bit_cnt) {
						if (data_sta) {
							if (!f_V1_disable) {
								f_Key_TrigV1 = 1;
							}
						} else {
							//p_Out1_V2 = 0;
						}
					} else if (24 == dat_bit_cnt) {
						if (data_sta) {
							if (!f_H_disable) {
								f_Key_TrigH = 1;
							}
						} else {
							//p_Out6_V1 = 0;
						}
						
						data_sta = 0;
						sync_sta = 0;
							
						dat_bit_cnt = 0;
							
						h_cnt_dat = 0;
							
						l_cnt_sync = 0;
						h_cnt_sync = 0;
					
                        f_sync_trig = 0;// wait next re-sync
					}

                    h_cnt_dat = 0;                
			    } else {
                    h_cnt_dat = 0;
					l_low_cnt = 0;
                    if ((h_cnt_sync>1) && (h_cnt_sync<5)) {// 0.256ms ~ 0.512ms H
                        sync_sta = 1;// sync stage1 ok
                    } else {
                        sync_sta = 0;
						h_cnt_sync = 0;
                        l_cnt_sync = 0;
                    }

                    if (1 == sync_sta) {
                        l_cnt_sync++;
					} else {
                        sync_sta = 0;
                        l_cnt_sync = 0;
					}	
                }
		    } else {// HIGH
                if (f_sync_trig) {
                    l_cnt_sync = 0;
                    h_cnt_sync = 0;
					h_cnt_dat++;
					l_low_cnt = 0;
				} else {
                    h_cnt_dat = 0;
					l_low_cnt = 0;
                    h_cnt_sync++;					
                    if (1 == sync_sta) {
                        if ((l_cnt_sync>75) && (l_cnt_sync<80)) {// 9.856ms ~ 10.112ms L
                            sync_sta = 2;// sync stage2 ok
                        } else {
                            sync_sta = 0;
							h_cnt_sync = 0;
							l_cnt_sync = 0;
                        }                    
                    } else if (2 == sync_sta) {
                        sync_sta = 3;// synced
                        f_sync_trig = 1;
						h_cnt_dat++;
                    } else {
                        sync_sta = 0;
					}	
                }
            }

			t_128us         =   1;
			If (--count1 == 0)					//	DZSN  count
			{
				count1		=	78;				//	128uS * 78 = 10 mS 
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
			
			if (f_H1_on||f_V1_on||f_V2_on) {
				if (f_OD_SP_sta) {// 16.4ms, 50% PWM
					if (0 == pwd_cnt) {
						if (f_H1_on) {
							p_OutB_H1 = 1;
						}
						if (f_V1_on) {
							p_OutA_V1 = 1;
						}
						if (f_V2_on) {
							p_OutB_V2 = 1;
						}
					} else if (64 == pwd_cnt) {
						if (f_H1_on) {
							p_OutB_H1 = 0;
						}
						if (f_V1_on) {
							p_OutA_V1 = 0;
						}
						if (f_V2_on) {
							p_OutB_V2 = 0;
						}
					} else if (128 == pwd_cnt) {
						pwd_cnt = 0;
					}

					pwd_cnt++;
				} else {
					if (f_H1_on) {
						p_OutB_H1 = 1;
					}
					if (f_V1_on) {
						p_OutA_V1 = 1;
					}
					if (f_V2_on) {
						p_OutB_V2 = 1;
					}
				}
			}
		}

//		while (t16_10ms)
        while (t_128us)
		{
			t_128us  = 0;
//			t16_10ms	=	0;

            if (t16_10ms) {
			    if (cnt_3s_time_startup < 250) {
				    cnt_3s_time_startup++;
				}    
			

//				if (50 == cnt_3s_time_startup) {
//					f_2k_on = 1;
				if (65 == cnt_3s_time_startup) {
					if (!f_mode2) {
						p_OutB_H1	=	1;
					}
					
					f_V1_on = 0;
					f_V2_on = 0;
					f_H1_on = 1;
            	    stepx = 1;
					f_2k_on = 1;
				} else if (110 == cnt_3s_time_startup) {
					if (1 == stepx) {
            	        if (!f_mode2) {
            	            p_OutA_V1 = 1;
            	        }
            	        
            	        f_V1_on = 1;
            	        
            	        stepx = 2;
						f_2k_on = 1;
            	    }
				} else if (155 == cnt_3s_time_startup) {
					if (2 == stepx) {
            	        if (!f_mode2) {
            	            p_OutB_V2 = 1;
            	        }
            	        
            	        f_V2_on = 1;
            	        stepx = 3;
						f_2k_on = 1;
            	    }
				} else if (200 == cnt_3s_time_startup) {
					if ((3 == stepx) && (0 == start)) {
            	        f_V1_on = 0;
            	        f_V2_on = 0;
            	        
            	        p_OutA_V1 = 0;
            	        p_OutB_V2 = 0;
            	        
            	        stepx = 1;
            	        start = 1;
            	    }
				}
            }
            
//			else if (250 == cnt_3s_time_startup) {
			if (1 == start) {
				if (t16_10ms) {
					// port change detect(both H->L and L->H)
					A	=	(PA ^ Key_Flag) & _FIELD(p_InA_OD);	//	only check the bit of p_Key_In.
					if (! ZF)
					{										//	if is not same,
						// Active: H->L
						if (!p_InA_OD) {
							if (--cnt_Key_10ms_1 == 0)
							{									//	and over debounce time.
								Key_flag	^=	_FIELD(p_InA_OD);
								f_OD_LP_Trig	=	1;				//	so Trigger, when stable at 3000 mS.
								cnt_Key_10ms_1	=	150;
							}
                	
							if (cnt_Key_10ms_1 == 145) {
								f_2k_on = 1;
							}
						} else {// Up: H->L
							Key_flag	^=	_FIELD(p_InA_OD);
						}
					} else {
						if (cnt_Key_10ms_1 < 145) {
							if (!f_OD_disable) {
								if (f_OD_SP_sta) {
									f_OD_SP_sta = 0;// short push
								} else {
									f_OD_SP_sta = 1;// short push
								}
							}
						}

						cnt_Key_10ms_1	=	150;
					}
                	
					A	=	(PA ^ Key_Flag) & _FIELD(p_InA_VJ);	//	only check the bit of p_Key_In.
					if (! ZF)
					{										//	if is not same,
						if (--cnt_Key_10ms_2 == 0)
						{									//	and over debounce time.
							Key_flag	^=	_FIELD(p_InA_VJ);
						}
					} else {
						cnt_Key_10ms_2	=	4;
					}
					
					A	=	(PB ^ Key_flag/*_mode2*/) & _FIELD(p_InB_H);	//	only check the bit of p_Key_In.
					if (! ZF)
					{										//	if is not same,
						// Active: H->L
						if (!p_InB_H) {
							if (--cnt_Key_10ms_3_mode2 == 0)
							{									//	and over debounce time.
								Key_flag/*_mode2*/	^=	_FIELD(p_InB_H);
								//f_Key_Trig3_mode2	=	1;				//	so Trigger, when stable at 30 mS.
								cnt_Key_10ms_3_mode2 = 250;
							}
						} else {// Up: H->L
							Key_flag/*_mode2*/	^=	_FIELD(p_InB_H);
						}
					} else {
						if (cnt_Key_10ms_3_mode2 < 245) {
							Key_flag/*_mode2*/	^=	_FIELD(p_InB_H);
							if (!f_H_disable) {
								f_Key_TrigH = 1;
							}
						}
						cnt_Key_10ms_3_mode2 = 250;
					}
                	
					A	=	(PB ^ Key_flag/*_mode2*/) & _FIELD(p_InB_V1);	//	only check the bit of p_Key_In.
					if (! ZF)
					{										//	if is not same,
						// Active: H->L
						if (!p_InB_V1) {
							if (--cnt_Key_10ms_4_mode2 == 0)
							{									//	and over debounce time.
								Key_flag/*_mode2*/	^=	_FIELD(p_InB_V1);
								//f_Key_Trig4_mode2	=	1;				//	so Trigger, when stable at 30 mS.
								cnt_Key_10ms_4_mode2 = 250;
							}
						} else {// Up: H->L
							Key_flag/*_mode2*/	^=	_FIELD(p_InB_V1);
						}
					} else {
						if (cnt_Key_10ms_4_mode2 < 245) {
							Key_flag/*_mode2*/	^=	_FIELD(p_InB_V1);
							if (!f_V1_disable) {
								f_Key_TrigV1 = 1;
							}
						}
						cnt_Key_10ms_4_mode2 = 250;
					}

					A	=	(PB ^ Key_flag/*_mode2*/) & _FIELD(p_InB_V2);	//	only check the bit of p_Key_In.
					if (! ZF)
					{										//	if is not same,
						// Active: H->L
						if (!p_InB_V2) {
							if (--cnt_Key_10ms_5_mode2 == 0)
							{									//	and over debounce time.
								Key_flag/*_mode2*/	^=	_FIELD(p_InB_V2);
								//f_Key_Trig4_mode2	=	1;				//	so Trigger, when stable at 30 mS.
								cnt_Key_10ms_5_mode2 = 250;
							}
						} else {// Up: H->L
							Key_flag/*_mode2*/	^=	_FIELD(p_InB_V2);
						}
					} else {
						if (cnt_Key_10ms_5_mode2 < 245) {
							Key_flag/*_mode2*/	^=	_FIELD(p_InB_V2);
							if (!f_V2_disable) {
								f_Key_TrigV2 = 1;
							}
						}
						cnt_Key_10ms_5_mode2 = 250;
					}
					
					if (f_OD_LP_Trig)
					{
						f_OD_LP_Trig = 0;
						f_2k_on = 1;
						
						cnt_3s_time_1++;
						
						if (1 == cnt_3s_time_1) {
							f_led_flash = 1;
							f_vj_on = 0;
						} else if (2 == cnt_3s_time_1) {
							f_led_flash = 0;
							cnt_3s_time_1 = 0;
							// Disable Timer2 PWM
							tm2c = 0b0000_0000;// IHRC | PA3 | Period | Disable Inverse
							
							p_OutA_2K	=	0;
						}
					}
                }	
				//if (f_Key_Trig2)// VJ
				if (1)
				{			
	                //p_OutB_LED = 1;
					//f_Key_Trig2 = 0;

					// Normal Mode
					if (0 == cnt_3s_time_1) {
						// Down: L->H
						if (p_InA_VJ) {// special mode
//							if (last_vj_state != 1) {
							if ((last_vj_state != 1) && (cnt_3s_time_led == 19)) {								
								last_vj_state = 1;
								
								f_vj_on = 1;
								f_led_flash = 1;
								
								// Disable all 100HZ PWM
								p_OutA_V1 =	0;
								p_OutB_V2 =	0;
								p_OutB_H1 =	0;
								
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
									
								f_V1_on = 0;
								f_V2_on = 0;
								f_H1_on = 0;
								
//								// Enable Timer2 PWM to 2KHz
//								tm2ct = 0x0;
//								tm2b = 0b0111_1100;// 124
//								//tm2s = 0b000_01111;// 15
//								tm2s = 0b000_00111;// 7
//								tm2c = 0b0001_1000;// CLK(=IHRC/2) | PA3 | Period | Disable Inverse
							}
						} else {
//							if (last_vj_state != 0) {
							if ((last_vj_state != 0) && (cnt_3s_time_led == 19)) {								
								last_vj_state = 0;
								
								f_vj_on = 0;
								f_led_flash = 0;
								
								// Disable Timer2 PWM
//								tm2c = 0b0000_0000;// IHRC | PA3 | Period | Disable Inverse
//								p_OutB_LED = 1;
								
								if (f_last_V1_on) {
									if (!f_mode2) {
										p_OutA_V1 = 1;
									}
									
									f_V1_on = 1;
								}
								if (f_last_V2_on) {
									if (!f_mode2) {
										p_OutB_V2 = 1;
									}
									
									f_V2_on = 1;
								}
								if (f_last_H1_on) {
									if (!f_mode2) {
										p_OutB_H1 = 1;
									}
									
									f_H1_on = 1;
								}
							}
						}
					}
				}
				
				if (t16_10ms) {
					if (f_OD_disable) {
						od_disable_cnt++;
						
						if (8 == od_disable_cnt) {// 80ms debounce
							f_OD_disable = 0;
						}
					}
					
					if (f_H_disable) {
						h_disable_cnt++;
						
						if (8 == h_disable_cnt) {// 80ms debounce
							f_H_disable = 0;
						}
					}
					
					if (f_V1_disable) {
						v1_disable_cnt++;
						
						if (8 == v1_disable_cnt) {// 80ms debounce
							f_V1_disable = 0;
						}
					}
					
					if (f_V2_disable) {
						v2_disable_cnt++;
						
						if (8 == v2_disable_cnt) {// 80ms debounce
							f_V2_disable = 0;
						}
					}
					
					if (f_Key_TrigH)// CN1/H
					{
//	            	    p_OutB_LED = 1;
						f_Key_TrigH = 0;
						f_2k_on = 1;
                	
						// L -> H
						if (!p_InA_VJ) {
							cnt_3s_time_h++;
		        	
							if (1 == cnt_3s_time_h) {
								if (f_H1_on) {// Current is ON
                	                f_H1_on = 0;
                	                p_OutB_H1 = 0;
                	            } else {// Current is OFF
                	                f_H1_on = 1;
                	                
                	                if (!f_mode2) {
                	                    p_OutB_H1 = 1;
                	                }
                	            }
							} else if (2 == cnt_3s_time_h) {
								cnt_3s_time_h = 0;
                	            
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

					if (f_Key_TrigV1)// V1
					{
//	            	    p_OutB_LED = 1;
						f_Key_TrigV1 = 0;
						f_2k_on = 1;
                	
						// L -> H
						if (!p_InA_VJ) {
							cnt_3s_time_v1++;
		        	
							if (1 == cnt_3s_time_v1) {
								if (f_V1_on) {// Current is ON
                	                f_V1_on = 0;
                	                p_OutA_V1 = 0;
                	            } else {// Current is OFF
                	                f_V1_on = 1;
                	                
                	                if (!f_mode2) {
                	                    p_OutA_V1 = 1;
                	                }
                	            }
							} else if (2 == cnt_3s_time_v1) {
								cnt_3s_time_v1 = 0;
                	            
								if (f_V1_on) {// Current is ON
                	                f_V1_on = 0;
                	                p_OutA_V1 = 0;
                	            } else {// Current is OFF
                	                f_V1_on = 1;
                	                
                	                if (!f_mode2) {
                	                    p_OutA_V1 = 1;
                	                }
                	            }
							}
						}
					}

					if (f_Key_TrigV2)// V2
					{
//	            	    p_OutB_LED = 1;
						f_Key_TrigV2 = 0;
						f_2k_on = 1;
                	
						// L -> H
						if (!p_InA_VJ) {
							cnt_3s_time_v2++;
		        	
							if (1 == cnt_3s_time_v2) {
								if (f_V2_on) {// Current is ON
                	                f_V2_on = 0;
                	                p_OutB_V2 = 0;
                	            } else {// Current is OFF
                	                f_V2_on = 1;
                	                
                	                if (!f_mode2) {
                	                    p_OutB_V2 = 1;
                	                }
                	            }
							} else if (2 == cnt_3s_time_v2) {
								cnt_3s_time_v2 = 0;
                	            
								if (f_V2_on) {// Current is ON
                	                f_V2_on = 0;
                	                p_OutB_V2 = 0;
                	            } else {// Current is OFF
                	                f_V2_on = 1;
                	                
                	                if (!f_mode2) {
                	                    p_OutB_V2 = 1;
                	                }
                	            }
							}
						}
					}
				}	
                
				if (f_led_flash) {
 					if (19 == cnt_3s_time_led) {
//						p_OutB_LED = 0;
						if (f_vj_on) {
							// 2kHz
							f_2k_on = 1;
						}
						cnt_3s_time_led = 0;
//					} else if (11 == cnt_3s_time_led) {
//						p_OutB_LED = 1;
					}
					if (t16_10ms) {
				        cnt_3s_time_led++;
					}
				} else {
//					p_OutB_LED = 1;
					cnt_3s_time_led = 19;
				}
			}

			if (f_2k_on) {
				p_OutB_LED = 0;
				if ((0 == cnt_3s_time_2k) && (0 == f_2k_flag)) {					
					// Enable Timer2 PWM to 2KHz
					tm2ct = 0x0;
					tm2b = 0b1011_0110;// 124
					//tm2s = 0b000_01111;// 15
                    tm2s = 0b000_00011;// 7
					tm2c = 0b0001_1000;// CLK(=IHRC/2) | PA3 | Period | Disable Inverse
					
					f_2k_flag = 1;
				}
				
                if (t16_10ms) {
				    cnt_3s_time_2k++;
				}    
				// ring 120ms
				if (10 == cnt_3s_time_2k) {
					f_2k_flag = 0;
					f_2k_on = 0;
					cnt_3s_time_2k = 0;
					// Disable Timer2 PWM
					tm2c = 0b0000_0000;// IHRC | PA3 | Period | Disable Inverse
	                p_OutA_2K	=	0;
				}
			} else {
				p_OutB_LED = 1;
			}	

            t16_10ms = 0;
            
			break;
		}
	}
}