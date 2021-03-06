#include	"extern.h"

BIT		p_InB_OD	:	PB.0;
BIT		p_InA_VJ	:	PA.5;
BIT		p_InB_V	    :	PB.2;
BIT		p_InB_H	    :	PB.1;
BIT		p_InA_QV2	:	PA.3;
BIT		p_InA_QV3	:	PA.4;

BIT		p_OutB_2K	:	PB.5;
BIT		p_OutB_V1   :	PB.7;
BIT		p_OutA_V2   :	PA.7;
BIT		p_OutA_V3   :	PA.6;
BIT		p_OutB_H1   :	PB.6;

// High is active
BIT		p_OutB_LED	:	PA.0;// LED

void	FPPA0 (void)
{
	.ADJUST_IC	SYSCLK=IHRC/4		//	SYSCLK=IHRC/4
    
    $ EOSCR		DIS_LVD_BANDGAP;

	$	p_OutB_2K		    Out, Low;
	$	p_OutB_V1		    Out, Low;
	$	p_OutA_V2		    Out, Low;
    $	p_OutA_V3		    Out, Low;
	$	p_OutB_H1		    Out, Low;
    $	p_OutB_LED		    Out, Low;// off

	$	p_InB_OD		    In;
	$	p_InA_VJ		    In;
	$	p_InB_V		        In;
	$	p_InB_H		        In;
    $	p_InA_QV2		    In;
    $	p_InA_QV3		    In;

	// IN Pull-UP
    PBPH		=		_FIELD(p_InB_OD, p_InB_V, p_InB_H);

    // 1/16M * 2^(9+1) = 64us
    // 1/16M * 2^(10+1) = 128
    $ T16M      IHRC, /1, BIT9;// 16MHz/1 = 16MHz:the time base of T16.
	$ TM2C		IHRC, Disable, Period, Inverse;

	BYTE	Key_Flag;
	Key_Flag			=	_FIELD(p_InB_OD, p_InA_VJ, p_InB_V, p_InB_H, p_InA_QV2, p_InA_QV3);

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
	
//	pmode	Program_Mode;
//	fppen	=	0xFF;

	BYTE	t16_flag;
	BYTE	count1 = 1;
	BYTE 	count2 = 0;
	BYTE	count3 = 0;
	BYTE	count4 = 0;
	BYTE	count_l = 0;
	BYTE	count_h = 0;
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
    
	f_mode2 = 1;
	f_2k_on = 0;

	while (1)
	{
		if  (INTRQ.T16)
		{
			INTRQ.T16		=	0;
			If (--count1 == 0)					//	DZSN  count
			{
				count1		=	156;				//	64uS * 156 = 10 mS 
				t16_10ms	=	1;
			}

            count_l++;

            if (100 == count_l) {
                count_l = 0;
                count_h++;
                if (100 == count_h)
                    count_h = 0;
            }
        }

        if (f_mode2) {
            if (!f_pwm_mode) {// dutyratio = 42%
                if ((0 == count_l)&&(0 == count_h)) {
                    if (f_V1_on) {
                        p_OutB_V1 = 1;
                    }
                    if (f_V3_on) {
                        p_OutA_V3 = 1;
                    }
                } else if ((66 == count_l)&&(0 == count_h)) {
                    p_OutB_V1 = 0;
                    p_OutA_V3 = 0;
                } else if ((78 == count_l)&&(0 == count_h)) {
                    if (f_V2_on) {
                        p_OutA_V2 = 1;
                    }
                    if (f_H1_on) {
                        p_OutB_H1 = 1;
                    }
                } else if ((44 == count_l)&&(1 == count_h)) {
                    p_OutA_V2 = 0;
                    p_OutB_H1 = 0;
                } else if ((56 == count_l)&&(1 == count_h)) {
                    count_l = 0;
                    count_h = 0;
                }
            } else {// dutyratio = 30%
                if ((0 == count_l)&&(0 == count_h)) {
                    if (f_V1_on) {
                        p_OutB_V1 = 1;
                    }
                    if (f_V3_on) {
                        p_OutA_V3 = 1;
                    }
                } else if ((47 == count_l)&&(0 == count_h)) {
                    p_OutB_V1 = 0;
                    p_OutA_V3 = 0;
                } else if ((78 == count_l)&&(0 == count_h)) {
                    if (f_V2_on) {
                        p_OutA_V2 = 1;
                    }
                    if (f_H1_on) {
                        p_OutB_H1 = 1;
                    }
                } else if ((25 == count_l)&&(1 == count_h)) {
                    p_OutA_V2 = 0;
                    p_OutB_H1 = 0;
                } else if ((56 == count_l)&&(1 == count_h)) {
                    count_l = 0;
                    count_h = 0;
                }
            }
        }

		while (t16_10ms)
		{
			t16_10ms	=	0;

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
                            p_OutA_V2 = 1;
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
                    if (p_InA_QV3) {// HIGH
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
                        p_OutA_V2 = 0;
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
                    p_OutA_V2 = 0;
                    p_OutA_V3 = 0;
                    
                    stepx = 1;
                    start = 1;
                }
			}

//			else if (250 == cnt_3s_time_startup) {
			if (1 == start) {
				// port change detect(both H->L and L->H)
				A	=	(PB ^ Key_Flag) & _FIELD(p_InB_OD);	//	only check the bit of p_Key_In.
				if (! ZF)
				{										//	if is not same,
					// Active: H->L
					if (!p_InB_OD) {
						if (--cnt_Key_10ms_1 == 0)
						{									//	and over debounce time.
							Key_flag	^=	_FIELD(p_InB_OD);
							f_Key_Trig1	=	1;				//	so Trigger, when stable at 3000 mS.
							cnt_Key_10ms_1	=	250;
						}

						if (cnt_Key_10ms_1 == 245) {
							f_2k_on = 1;
						}
					} else {// Up: H->L
						Key_flag	^=	_FIELD(p_InB_OD);
					}
				} else {
                    if (cnt_Key_10ms_1 < 245) {
                        if (f_pwm_mode) {
                            f_pwm_mode = 0;
                        } else {
                            f_pwm_mode = 1;
                        }
                    }
					cnt_Key_10ms_1	=	250;
				}

				A	=	(PA ^ Key_Flag) & _FIELD(p_InA_VJ);	//	only check the bit of p_Key_In.
				if (! ZF)
				{										//	if is not same,
					if (--cnt_Key_10ms_2 == 0)
					{									//	and over debounce time.
						Key_flag	^=	_FIELD(p_InA_VJ);
						f_Key_Trig2	=	1;				//	so Trigger, when stable at 30 mS.
					}
				} else {
					cnt_Key_10ms_2	=	4;
				}
				
				A	=	(PB ^ Key_flag/*_mode2*/) & _FIELD(p_InB_V);	//	only check the bit of p_Key_In.
				if (! ZF)
				{										//	if is not same,
					// Active: H->L
					if (!p_InB_V) {
						if (--cnt_Key_10ms_3_mode2 == 0)
						{									//	and over debounce time.
							Key_flag/*_mode2*/	^=	_FIELD(p_InB_V);
							//f_Key_Trig3_mode2	=	1;				//	so Trigger, when stable at 30 mS.
							cnt_Key_10ms_3_mode2 = 250;
						}
					} else {// Up: H->L
						Key_flag/*_mode2*/	^=	_FIELD(p_InB_V);
					}
				} else {
					if (cnt_Key_10ms_3_mode2 < 245) {
						Key_flag/*_mode2*/	^=	_FIELD(p_InB_V);
						f_Key_Trig3 = 1;
					}
					cnt_Key_10ms_3_mode2 = 250;
				}

				A	=	(PB ^ Key_flag/*_mode2*/) & _FIELD(p_InB_H);	//	only check the bit of p_Key_In.
				if (! ZF)
				{										//	if is not same,
					// Active: H->L
					if (!p_InB_H) {
						if (--cnt_Key_10ms_4_mode2 == 0)
						{									//	and over debounce time.
							Key_flag/*_mode2*/	^=	_FIELD(p_InB_H);
							//f_Key_Trig4_mode2	=	1;				//	so Trigger, when stable at 30 mS.
							cnt_Key_10ms_4_mode2 = 250;
						}
					} else {// Up: H->L
						Key_flag/*_mode2*/	^=	_FIELD(p_InB_H);
					}
				} else {
					if (cnt_Key_10ms_4_mode2 < 245) {
						Key_flag/*_mode2*/	^=	_FIELD(p_InB_H);
						f_Key_Trig4 = 1;
					}
					cnt_Key_10ms_4_mode2 = 250;
				}
                
				if (f_Key_Trig1)
				{
					f_Key_Trig1 = 0;
					f_2k_on = 1;
					
					cnt_3s_time_1++;
					
					if (1 == cnt_3s_time_1) {
						f_led_flash = 1;
					} else if (2 == cnt_3s_time_1) {
						f_led_flash = 0;
						cnt_3s_time_1 = 0;
                        
						// Disable 2KHz
                        pwmg0c = 0b0000_0000;// do not output PWM
						
						p_OutB_2K	=	0;
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
							if (last_vj_state != 1) {
								last_vj_state = 1;
								
								f_vj_on = 1;
								f_led_flash = 1;
								
								// Disable all 100HZ PWM
								p_OutB_V1	=	0;
								p_OutA_V2	=	0;
								p_OutB_H1	=	0;
								p_OutA_V3   =   0;
								
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
								
								// Enable PWMG0C to 2KHz
								pwmgcubl = 0b0000_0000;
								pwmgcubh = 0b0001_1111;
								
								pwmg0dtl = 0b1000_0000;
								pwmg0dth = 0b0000_1111;
			
								pwmg0c = 0b0000_0010;// PB5 PWM
								pwmgclk = 0b1100_0000;// enable PWMG CLK(=SYSCLK/16)
							}
						} else {
							if (last_vj_state != 0) {
								last_vj_state = 0;
								
								f_vj_on = 0;
								f_led_flash = 0;
								
								// Disable 2KHz
								pwmg0c = 0b0000_0000;// do not output PWM
								
								p_OutB_LED = 1;
								
								if (f_last_V1_on) {
									if (!f_mode2) {
										p_OutB_V1	=	1;
									}
									
									f_V1_on = 1;
								}
								if (f_last_V2_on) {
									if (!f_mode2) {
										p_OutA_V2	=	1;
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
					}
				}
				
				if (f_Key_Trig3)// CN1/V
				{
	                p_OutB_LED = 1;
					f_Key_Trig3 = 0;
					f_2k_on = 1;

					// L -> H
					if (!p_InA_VJ) {
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
                                    p_OutA_V2 = 1;
                                }
                                
                                f_V2_on = 1;
                                stepx = 3;
                            } else {
                                f_V1_on = 0;
                                p_OutB_V1 = 0;
                                
                                stepx = 1;
                            }
                        } else if (3 == stepx) {
                            if (p_InA_QV3) {// HIGH
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
                                p_OutA_V2 = 0;
                                
                                stepx = 1;
                            }
                        } else if (4 == stepx) {
                            f_V1_on = 0;
                            f_V2_on = 0;
                            f_V3_on = 0;
                            
                            p_OutB_V1 = 0;
                            p_OutA_V2 = 0;
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

					// L -> H
					if (!p_InA_VJ) {
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
                
				if (f_led_flash) {
					cnt_3s_time_led++;
					if (16 == cnt_3s_time_led) {
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
				} else {
					p_OutB_LED = 1;
					cnt_3s_time_led = 0;
				}
			}

			if (f_2k_on) {
				if (0 == cnt_3s_time_2k) {
					// Enable PWMG0C to 2KHz
                    pwmgcubl = 0b0000_0000;
					pwmgcubh = 0b0001_1111;
					
					pwmg0dtl = 0b1000_0000;
					pwmg0dth = 0b0000_1111;

					pwmg0c = 0b0000_0010;// PB5 PWM
                    pwmgclk = 0b1100_0000;// enable PWMG CLK(=SYSCLK/16)
				}

				cnt_3s_time_2k++;
				// ring 120ms
				if (10 == cnt_3s_time_2k) {
					f_2k_on = 0;
					cnt_3s_time_2k = 0;
                    
					// Disable 2KHz
                    pwmg0c = 0b0000_0000;// do not output PWM
	                p_OutB_2K	=	0;
				}
			}

			break;
		}
	}
}
