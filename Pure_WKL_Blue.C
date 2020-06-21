#include    "extern.h"

#define USE_20K 1
//#define USE_10K 1

BIT            p_InB_VJ    :   PB.2;
BIT            p_InA_V     :   PA.3;
BIT            p_InA_H     :   PA.4;
BIT            p_InA_OD    :   PA.0;
BIT            p_InB_X1    :   PB.5;// OutCtrl
BIT            p_InA_X2    :   PA.5;// High Monitor

BIT            p_OutB_2K   :   PB.1;
BIT            p_OutA_V1   :   PA.7;
BIT            p_OutB_V2   :   PB.7;
BIT            p_OutB_V3   :   PB.6;
BIT            p_OutA_H1   :   PA.6;

// High is active
BIT            p_OutB_LED    :    PB.0;// LED

void    FPPA0 (void)
{
    .ADJUST_IC    SYSCLK=IHRC/4        //    SYSCLK=IHRC/4

    $ EOSCR        DIS_LVD_BANDGAP;

    $    p_OutB_2K            Out, Low;
    $    p_OutA_V1            Out, Low;
    $    p_OutB_V2            Out, Low;
    $    p_OutB_V3            Out, Low;
    $    p_OutA_H1            Out, Low;
    $    p_OutB_LED           Out, High;// on
	$    p_InB_X1             Out, Low;

    $    p_InA_OD             In;// Monitor Low
    $    p_InB_VJ             In;// Monitor High
    $    p_InA_V              In;// Monitor Low
    $    p_InA_H              In;// Monitor Low
	$    p_InA_X2             In;// Monitor High

    // IN Pull-UP
    PAPH        =        _FIELD(p_InA_OD, p_InA_V, p_InA_H);

#ifdef USE_10K
	// 1/(16M/4) * 2^(9+1) = 256us
    $ T16M        IHRC, /4, BIT9;                // 256us
#endif

#ifdef USE_20K
	// 1/(16M/4) * 2^(8+1) = 128us
    // $ T16M        IHRC, /4, BIT8;                // 128us
	$ T16M        IHRC, /4, BIT10;                // 512us
#endif
    $ TM2C        IHRC, Disable, Period, Inverse;

    BYTE    Key_Flag;
    Key_Flag            =    _FIELD(p_InA_OD, p_InA_V, p_InA_H);

    BYTE    Sys_Flag    =    0;
    BIT        f_Key_Trig1      :    Sys_Flag.0;
    BIT        t16_10ms         :    Sys_Flag.1;
    BIT        f_Key_Trig3      :    Sys_Flag.3;
    BIT        f_Key_Trig4      :    Sys_Flag.4;
    BIT        f_IN_QV2         :    Sys_Flag.5;
    BIT        f_IN_QV3         :    Sys_Flag.6;

    BYTE    Sys_FlagB    =    0;
    BIT        f_mode2          :    Sys_FlagB.0;
    BIT        f_2k_on          :    Sys_FlagB.1;
    BIT        f_led_flash      :    Sys_FlagB.2;
    BIT        f_led_state      :    Sys_FlagB.3;
    BIT        f_vj_on          :    Sys_FlagB.5;

    BYTE    Sys_FlagC    =    0;
    BIT        f_V1_on          :    Sys_FlagC.0;
    BIT        f_V2_on          :    Sys_FlagC.1;
    BIT        f_H1_on          :    Sys_FlagC.2;
    BIT        f_V3_on          :    Sys_FlagC.3;
	BIT        count2           :    Sys_FlagC.4;
	BIT        count3           :    Sys_FlagC.5;

//    pmode    Program_Mode;
//    fppen    =    0xFF;

    BYTE    count1 = 1;
    BYTE    count_l = 0;
    BYTE    count_h = 0;
    BYTE    last_vj_state = 8;

    BYTE    cnt_Key_10ms_1    =    175;              //    Key debounce time = 40 mS

    BYTE    cnt_Key_10ms_3    =    175;                //    Key debounce time = 40 mS
    BYTE    cnt_Key_10ms_4    =    175;                //    Key debounce time = 40 mS

    BYTE    cnt_3s_time_1       = 0;//
    BYTE    cnt_3s_time_2k      = 0;// 2KHz
    BYTE    cnt_3s_time_led     = 0;// 1Hz
    BYTE    flash_time_laser  	= 40;
	BYTE    flash_time_beep  	= 12;
    BYTE    cnt_3s_time_startup = 0;//

    BYTE    stepx = 1;
    BYTE    stepv = 1;
    BYTE    steph = 1;
    BYTE    start = 1;
	BYTE    beep_time = 0;

#ifdef USE_10K
	// (512-112)/512 * 128 = 100us
    WORD    count    =    112;
#endif
#ifdef USE_20K
	// (256-64)/256 * 64us = 48us
	// (256-16)/256 * 64us = 60us
	// (1024-304)/1024 * 256us = 180us
    // WORD    count    =    317; // 176.75us -> 179us
	// WORD    count    =    280;
	WORD    count    =    290; // 183.5 -> 182us H + 188us L
#endif

    f_mode2 = 0;// default DC
    f_2k_on = 0;

	if (p_InA_X2) {// if Pin7 High, Pwr On
		p_InB_X1 = 1;
	}

	while (0) {
		if (INTRQ.T16) {// = 20KHz=50us
            INTRQ.T16        =    0;
            stt16    count;
			
			if (count1) {
				count1 = 0;
				p_OutA_V1 = 1;
			} else {
				count1 = 1;
				p_OutA_V1 = 0;
			}
		}
	}

    while (1) {
        if (INTRQ.T16) {// 180us
            INTRQ.T16        =    0;
            stt16    count;

            if (--count1 == 0) {
                count1      =    55;                // 180us * 55 = 10 ms
                t16_10ms    =    1;
				
                if (f_vj_on) {
                    flash_time_laser--;

                    if (flash_time_laser <= 0) {
                        flash_time_laser = 40;
                    }
                }
            }

            if (flash_time_laser >= 20) {
				if (!count2) {
					count2 = 1;
					count_l++;

					if (100 == count_l) {
						count_l = 0;
						count_h++;
						if (100 == count_h)
							count_h = 0;
					}
				} else {
					count2 = 0;
				}
			}
			
			if (f_2k_on) {
				if (beep_time) {
					p_OutB_2K = 0;
					beep_time = 0;
				} else {
					p_OutB_2K = 1;
					beep_time = 1;
				}
#if 0
				beep_time++;

				if (1 == beep_time) {
					p_OutB_2K = 1;// 2 3 4
				} else if (4 == beep_time) {
					p_OutB_2K = 0;// 5 6 1
				} else if (6 == beep_time) {
					beep_time = 0;
				}
// #else
				p_OutB_2K = 1;
#endif
			} else {
				p_OutB_2K = 0;
			}
        }

		// if vj mode, laser ON 30ms then OFF 220ms
        if (flash_time_laser >= 20) {
			if (f_mode2) {// dutyratio = 60% (MCU High)
                if ((0 == count_l)&&(0 == count_h)) {
                    if (f_V1_on) {
                        p_OutA_V1 = 1;
                    }
                    if (f_V3_on) {
                        p_OutB_V3 = 1;
                    }
                } else if ((40 == count_l)&&(0 == count_h)) {
                    if (f_V2_on) {
                        p_OutB_V2 = 1;
                    }
                    if (f_H1_on) {
                        p_OutA_H1 = 1;
                    }            
                } else if ((60 == count_l)&&(0 == count_h)) {
                    p_OutA_V1 = 0;
                    p_OutB_V3 = 0;
                } else if ((0 == count_l)&&(1 == count_h)) {
                    p_OutA_H1 = 0;
                    p_OutB_V2 = 0;

                    count_l = 0;
                    count_h = 0;
                } else if ((count_l != 0)&&(1 == count_h)) {
                    count_l = 0;
                    count_h = 0;
                }
            } else {
                if (f_H1_on) {
                    p_OutA_H1 = 1;
                }
                if (f_V1_on) {
                    p_OutA_V1 = 1;
                }
                if (f_V2_on) {
                    p_OutB_V2 = 1;
                }
                if (f_V3_on) {
                    p_OutB_V3 = 1;
                }
            }
        } else {
            if (19 == flash_time_laser) {
                p_OutA_H1 = 0;
                p_OutA_V1 = 0;
                p_OutB_V2 = 0;
                p_OutB_V3 = 0;
            }
        }

        while (t16_10ms)
        {
            t16_10ms    =    0;

            if (cnt_3s_time_startup < 250) {
                cnt_3s_time_startup++;
            }
#if 0
            if (25 == cnt_3s_time_startup) {
#if 0
                if (!f_mode2) {
                    p_OutA_H1    =    1;
                }

                f_V1_on = 0;
                f_V2_on = 0;
                f_H1_on = 1;
                f_V3_on = 0;
                stepx = 1;
                f_2k_on = 1;
#endif
            } else if (70 == cnt_3s_time_startup) {
#if 0
                if (1 == stepx) {
                    if (!f_mode2) {
                        p_OutA_V1 = 1;
                    }

                    f_V1_on = 1;

                    stepx = 2;
                    f_2k_on = 1;
                }
#endif
            } else if (115 == cnt_3s_time_startup) {
#if 0
                if (2 == stepx) {
                    f_IN_QV2 = 1;

                    if (f_IN_QV2) {
                        if (!f_mode2) {
                            p_OutB_V2 = 1;
                        }

                        f_V2_on = 1;
                        stepx = 3;
                        f_2k_on = 1;
                    } else {
                        f_V1_on = 0;
                        p_OutA_V1 = 0;
                        start = 1;
                        stepx = 1;
                    }
                 }
#endif
            } else if (160 == cnt_3s_time_startup) {
#if 0
                if ((3 == stepx) && (0 == start)) {
                    f_IN_QV3 = 1;

                    if (f_IN_QV3) {
                        if (!f_mode2) {
                            p_OutB_V3 = 1;
                        }

                        f_V3_on = 1;

                        stepx = 4;
                        f_2k_on = 1;
                    } else {
                        f_V1_on = 0;
                        f_V2_on = 0;

                        p_OutA_V1 = 0;
                        p_OutB_V2 = 0;
                        start = 1;
                        stepx = 1;
                    }
                }
#endif
            } else if (209 == cnt_3s_time_startup) {
#if 0
                if ((4 == stepx) && (0 == start)) {
                    f_V1_on = 0;
                    f_V2_on = 0;
                    f_V3_on = 0;

                    p_OutA_V1 = 0;
                    p_OutB_V2 = 0;
                    p_OutB_V3 = 0;

                    stepx = 1;
                    stepv = 1;
                    start = 1;
                }
#endif
            }
#endif

            if (1 == start) {
                // port change detect(both H->L and L->H)
                A    =    (PA ^ Key_Flag) & _FIELD(p_InA_OD);    //    only check the bit of p_Key_In.
                if (! ZF)
                {                                        //    if is not same,
                    // Active: H->L
                    if (!p_InA_OD) {
                        if (cnt_Key_10ms_1 > 0) {
                            if (--cnt_Key_10ms_1 == 0)
                            {                                    //    and over debounce time.
#if 0
								if (f_mode2) {// PWM
									f_mode2 = 0;// DC
								}

								count_l = 0;
								count_h = 0;
								flash_time_laser = 40;

                                f_Key_Trig1    =    1;                //    so Trigger, when stable at 3000 mS.
#endif
                            }

                            if (cnt_Key_10ms_1 == 170) {
#if 0
								if (f_mode2) {
									f_mode2 = 0;
								} else {
									f_mode2 = 1;
								}

								count_l = 0;
								count_h = 0;
								flash_time_laser = 40;

								if (!f_vj_on) {
									f_2k_on = 1;
								}
#endif
                            }
                        }
                    } else {// Up: L->H
                        Key_flag    ^=    _FIELD(p_InA_OD);
                    }
                } else {
                    if (cnt_Key_10ms_1 < 170) {
						if (cnt_Key_10ms_1 != 0) {// Only ShortPress
						}

						if (count3) {
							if (p_InA_X2) {// if Pin7 High, Pwr Off
								p_InB_X1 = 0;
							}
						}

						count3 = 1;
                    }

                    cnt_Key_10ms_1    =    175;
                }

                A    =    (PA ^ Key_flag) & _FIELD(p_InA_V);    //    only check the bit of p_Key_In.
                if (! ZF)
                {                                        //    if is not same,
                    // Active: H->L
                    if (!p_InA_V) {
						if (cnt_Key_10ms_3 > 0) {
							if (--cnt_Key_10ms_3 == 0) {
								// do not support long press
							}
							
							if (cnt_Key_10ms_3 == 170) {
								f_Key_Trig3 = 1;
							}
						}
                    } else {// Up: L->H
                        Key_flag    ^=    _FIELD(p_InA_V);
                    }
                } else {
                    cnt_Key_10ms_3 = 175;
                }

                A    =    (PA ^ Key_flag) & _FIELD(p_InA_H);    //    only check the bit of p_Key_In.
                if (! ZF)
                {                                        //    if is not same,
                    // Active: H->L
                    if (!p_InA_H) {
						if (cnt_Key_10ms_4 > 0) {
							if (--cnt_Key_10ms_4 == 0) {
								// do not support long press
							}
							
							if (cnt_Key_10ms_4 == 170) {
								f_Key_Trig4 = 1;
							}
						}
                    } else {// Up: L->H
                        Key_flag    ^=    _FIELD(p_InA_H);
                    }
                } else {
                    cnt_Key_10ms_4 = 175;
                }

                if (f_Key_Trig1)
                {
                    f_Key_Trig1 = 0;

					if (!f_vj_on) {
					//	f_2k_on = 1;
					}

                    if (0 == cnt_3s_time_1) {
                        cnt_3s_time_1 = 1;
						f_led_flash = 1;
                    } else {
                        cnt_3s_time_1 = 0;
						f_led_flash = 0;
                    }
                }
				
				if (p_InA_X2) {// if Pin7 High, Disable VJ Warning
					cnt_3s_time_1 = 1;
				} else {
					cnt_3s_time_1 = 0;
				}

                // Normal Mode
                if (0 == cnt_3s_time_1) {
                    // Down: L->H
                    if (p_InB_VJ) {// special mode
                        if (last_vj_state != 1) {
                            last_vj_state = 1;

							f_2k_on = 1;
                            f_vj_on = 1;
                            f_led_flash = 1;
                        }
                    } else {
                        if (last_vj_state != 0) {
                            last_vj_state = 0;

                            f_vj_on = 0;
                            f_led_flash = 0;

							count_l = 0;
							count_h = 0;
                            flash_time_laser = 40;

                            // Disable Timer2 PWM
							p_OutB_2K    =    0;

                            // p_OutB_LED = 1;
                        }
                    }
                } else {
					count_l = 0;
					count_h = 0;
					f_vj_on = 0;
                    last_vj_state = 8;
					flash_time_laser = 40;
				}

                if (f_Key_Trig3)// CN1/V
                {
                    // p_OutB_LED = 1;
                    f_Key_Trig3 = 0;

					if (!f_vj_on) {
					//	f_2k_on = 1;
					}

                    if (1 == stepv) {
                        if (!f_mode2) {
                            p_OutA_V1 = 1;
                        }

                        f_V1_on = 1;

                        stepv = 2;
                    } else if (2 == stepv) {
                        f_IN_QV2 = 1;

                        if (f_IN_QV2) {
                            if (!f_mode2) {
                            	p_OutB_V2 = 1;
                            }

                            f_V2_on = 1;
                            stepv = 3;
                        } else {
                            f_V1_on = 0;
                            p_OutA_V1 = 0;

                            stepv = 1;
                        }
                    } else if (3 == stepv) {
                        f_IN_QV3 = 1;

                        if (f_IN_QV3) {
                            if (!f_mode2) {
                                p_OutB_V3 = 1;
                            }

                            f_V3_on = 1;

                            stepv = 4;
                        } else {
                            f_V1_on = 0;
                            f_V2_on = 0;

                            p_OutA_V1 = 0;
                            p_OutB_V2 = 0;

                            stepv = 1;
                        }
                    } else if (4 == stepv) {
                        f_V1_on = 0;
                        f_V2_on = 0;
                        f_V3_on = 0;

                        p_OutA_V1 = 0;
                        p_OutB_V2 = 0;
                        p_OutB_V3 = 0;

                        stepv = 1;
                    }
                }

                if (f_Key_Trig4)// CN1/H
                {
                    // p_OutB_LED = 1;
                    f_Key_Trig4 = 0;

					if (!f_vj_on) {
					//	f_2k_on = 1;
					}
                    steph++;

                    if (1 == steph) {
                        if (f_H1_on) {// Current is ON
                            f_H1_on = 0;
                            p_OutA_H1 = 0;
                        } else {// Current is OFF
                            f_H1_on = 1;

                            if (!f_mode2) {
                                p_OutA_H1 = 1;
                            }
                        }
                    } else if (2 == steph) {
                        steph = 0;

                        if (f_H1_on) {// Current is ON
                            f_H1_on = 0;
                            p_OutA_H1 = 0;
                        } else {// Current is OFF
                            f_H1_on = 1;

                            if (!f_mode2) {
                                p_OutA_H1 = 1;
                            }
                        }
                    }
                }

                if (f_led_flash) {
                    cnt_3s_time_led++;
                    if (44 == cnt_3s_time_led) {
                        if (f_led_state) {
                            // p_OutB_LED = 0;
                            f_led_state = 0;

                            if (f_vj_on) {
                                // 2kHz
                                f_2k_on = 1;
                            }
                        } else {
                            // p_OutB_LED = 1;
                            f_led_state = 1;

                            if (f_vj_on) {
                                // 2kHz
                                f_2k_on = 1;
                            }
                        }

                        cnt_3s_time_led = 0;
                    }
                } else {
                    // p_OutB_LED = 1;
                    cnt_3s_time_led = 0;
                }
            }

            if (f_2k_on) {
                cnt_3s_time_2k++;
                // ring 120ms
                if (23 == cnt_3s_time_2k) {
                    f_2k_on = 0;
					beep_time = 0;
                    cnt_3s_time_2k = 0;

                    // Disable Timer2 PWM
                    p_OutB_2K    =    0;
                }
            }

            break;
        }
    }
}
