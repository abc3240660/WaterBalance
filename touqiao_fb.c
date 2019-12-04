#include    "extern.h"

//#define USE_20K 1
#define USE_10K 1

BIT            p_InA_OD    :   PA.4;
BIT            p_InA_VJ    :   PA.5;
BIT            p_InA_V     :   PA.7;
BIT            p_InB_H     :   PB.1;
BIT            p_InA_QV2   :   PA.0;
BIT            p_InB_QV3   :   PB.0;

BIT            p_OutA_2K   :   PA.3;
BIT            p_OutB_V1   :   PB.2;
BIT            p_OutB_V2   :   PB.5;
BIT            p_OutA_V3   :   PA.6;
BIT            p_OutB_H1   :   PB.6;

// High is active
BIT        p_OutB_LED    :    PB.7;// LED

void    FPPA0 (void)
{
    .ADJUST_IC    SYSCLK=IHRC/4        //    SYSCLK=IHRC/4

    $ EOSCR        DIS_LVD_BANDGAP;

    $     p_OutA_2K              Out, Low;
    $    p_OutB_V1            Out, Low;
    $     p_OutB_V2              Out, Low;
    $    p_OutA_V3            Out, Low;
    $    p_OutB_H1            Out, Low;
    $    p_OutB_LED           Out, Low;// off

    $    p_InA_OD            In;
    $    p_InA_VJ             In;
    $    p_InA_V                In;
    $    p_InB_H              In;
    $    p_InA_QV2            In;
    $    p_InB_QV3            In;

    // IN Pull-UP
    PAPH        =        _FIELD(p_InA_OD, p_InA_V);
    PBPH        =        _FIELD(p_InB_H);

#ifdef USE_10K
    $ T16M        IHRC, /4, BIT9;                // 256us
#endif

#ifdef USE_20K
    $ T16M        IHRC, /4, BIT8;                // 128us
#endif
    $ TM2C        IHRC, Disable, Period, Inverse;

    BYTE    Key_Flag;
    Key_Flag            =    _FIELD(p_InA_OD, p_InA_VJ, p_InA_V, p_InB_H, p_InA_QV2, p_InB_QV3);

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

//    pmode    Program_Mode;
//    fppen    =    0xFF;

    BYTE    count1 = 1;
    BYTE    count_l = 0;
    BYTE    count_h = 0;
    BYTE    last_vj_state = 8;

    BYTE    cnt_Key_10ms_1    =    250;            //    Key debounce time = 40 mS

    BYTE    cnt_Key_10ms_3    =    250;      //    Key debounce time = 40 mS
    BYTE    cnt_Key_10ms_4    =    250;      //    Key debounce time = 40 mS

    BYTE    cnt_3s_time_1     = 0;
    BYTE    cnt_3s_time_2k    = 0;// 2KHz
    BYTE    cnt_3s_time_led   = 0;// 1Hz
    BYTE    flash_time_laser  	= 40;

    BYTE    cnt_3s_time_startup     = 0;
    BYTE    stepx = 0;
    BYTE    stepv = 0;
    BYTE    steph = 0;
    BYTE    start = 0;

#ifdef USE_10K
    WORD    count    =    112;
#endif
#ifdef USE_20K
    WORD    count    =    64;
#endif

    f_mode2 = 0;// default DC
    f_2k_on = 0;

    while (1) {
        if (INTRQ.T16) {// = 10KHz=100us
            INTRQ.T16        =    0;
            stt16    count;

            if (--count1 == 0) {
                count1      =    100;                // 100us * 100 = 10 ms
                t16_10ms    =    1;
				
                if (f_vj_on) {
                    flash_time_laser--;

                    if (flash_time_laser <= 0) {
                        flash_time_laser = 40;
                    }
                }
            }

            if (flash_time_laser >= 20) {
				count_l++;

				if (100 == count_l) {
					count_l = 0;
					count_h++;
					if (100 == count_h)
						count_h = 0;
				}
			}

        }

		// if vj mode, laser ON 30ms then OFF 220ms
        if (flash_time_laser >= 20) {
			if (f_mode2) {// dutyratio = 60% (MCU High)
                if ((0 == count_l)&&(0 == count_h)) {
                    if (f_V1_on) {
                        p_OutB_V1 = 1;
                    }
                    if (f_V3_on) {
                        p_OutA_V3 = 1;
                    }
                } else if ((40 == count_l)&&(0 == count_h)) {
                    if (f_V2_on) {
                        p_OutB_V2 = 1;
                    }
                    if (f_H1_on) {
                        p_OutB_H1 = 1;
                    }            
                } else if ((60 == count_l)&&(0 == count_h)) {
                    p_OutB_V1 = 0;
                    p_OutA_V3 = 0;
                } else if ((0 == count_l)&&(1 == count_h)) {
                    p_OutB_H1 = 0;
                    p_OutB_V2 = 0;

                    count_l = 0;
                    count_h = 0;
                }
            } else {
                if (f_H1_on) {
                    p_OutB_H1 = 1;
                }
                if (f_V1_on) {
                    p_OutB_V1 = 1;
                }
                if (f_V2_on) {
                    p_OutB_V2 = 1;
                }
                if (f_V3_on) {
                    p_OutA_V3 = 1;
                }
            }
        } else {
            if (19 == flash_time_laser) {
                p_OutB_H1 = 0;
                p_OutB_V1 = 0;
                p_OutB_V2 = 0;
                p_OutA_V3 = 0;
            }
        }

        while (t16_10ms)
        {
            t16_10ms    =    0;

            if (cnt_3s_time_startup < 250) {
                cnt_3s_time_startup++;
            }

            if (65 == cnt_3s_time_startup) {
                if (!f_mode2) {
                    p_OutB_H1    =    1;
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
                    stepv = 1;
                    start = 1;
                }
            }

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
                                f_Key_Trig1    =    1;                //    so Trigger, when stable at 3000 mS.
                                cnt_Key_10ms_1    =    250;
                            }

                            if (cnt_Key_10ms_1 == 245) {
                                f_2k_on = 1;
                            }
                        }
                    } else {// Up: H->L
                        Key_flag    ^=    _FIELD(p_InA_OD);
                    }
                } else {
                    if (cnt_Key_10ms_1 < 245) {
						if (!f_vj_on) {
							if (f_mode2) {
								f_mode2 = 0;
							} else {
								f_mode2 = 1;
							}
						}
                    }
                    cnt_Key_10ms_1    =    250;
                }

                A    =    (PA ^ Key_flag) & _FIELD(p_InA_V);    //    only check the bit of p_Key_In.
                if (! ZF)
                {                                        //    if is not same,
                    // Active: H->L
                    if (!p_InA_V) {
                        if (--cnt_Key_10ms_3 == 0)
                        {                                    //    and over debounce time.
                            Key_flag    ^=    _FIELD(p_InA_V);
                            cnt_Key_10ms_3 = 250;
                        }
                    } else {// Up: H->L
                        Key_flag    ^=    _FIELD(p_InA_V);
                    }
                } else {
                    if (cnt_Key_10ms_3 < 245) {
                        Key_flag    ^=    _FIELD(p_InA_V);
						if (!f_vj_on) {
							f_Key_Trig3 = 1;
						}
                    }
                    cnt_Key_10ms_3 = 250;
                }

                A    =    (PB ^ Key_flag) & _FIELD(p_InB_H);    //    only check the bit of p_Key_In.
                if (! ZF)
                {                                        //    if is not same,
                    // Active: H->L
                    if (!p_InB_H) {
                        if (--cnt_Key_10ms_4 == 0)
                        {                                    //    and over debounce time.
                            Key_flag    ^=    _FIELD(p_InB_H);
                            cnt_Key_10ms_4 = 250;
                        }
                    } else {// Up: H->L
                        Key_flag    ^=    _FIELD(p_InB_H);
                    }
                } else {
                    if (cnt_Key_10ms_4 < 245) {
                        Key_flag    ^=    _FIELD(p_InB_H);
                        if (!f_vj_on) {
							f_Key_Trig4 = 1;
						}
                    }
                    cnt_Key_10ms_4 = 250;
                }

                if (f_Key_Trig1)
                {
                    f_Key_Trig1 = 0;
                    f_2k_on = 1;

                    if (0 == cnt_3s_time_1) {
                        cnt_3s_time_1 = 1;
                        f_led_flash = 1;
                    } else {
                        cnt_3s_time_1 = 0;
                        f_led_flash = 0;
                    }
#if 0
                    if (!f_led_flash) {
                        f_led_flash = 1;
                    } else {
                        f_led_flash = 0;
#if 0
                        // Disable 2KHz
                        tm2c = 0b0000_0000;// IHRC | PA3 | Period | Disable Inverse

                        p_OutA_2K    =    0;
#endif
                    }
#endif
                }

                if (1)
                {
                    // Normal Mode
                    if (0 == cnt_3s_time_1) {
                        // Down: L->H
                        if (p_InA_VJ) {// special mode
                            if (last_vj_state != 1) {
                                last_vj_state = 1;

                                f_vj_on = 1;
                                f_led_flash = 1;

                                
                                // Enable Timer2 PWM to 2KHz
                                tm2ct = 0x0;
                                tm2b = 0b0111_1100;// 124

                                tm2s = 0b000_00111;// 7

                                tm2c = 0b0001_1000;// CLK(=IHRC/2) | PA3 | Period | Disable Inverse
                            }
                        } else {
                            if (last_vj_state != 0) {
                                last_vj_state = 0;

                                f_vj_on = 0;
                                f_led_flash = 0;

                                flash_time_laser = 40;

                                // Disable Timer2 PWM
                                tm2c = 0b0000_0000;// IHRC | PA3 | Period | Disable Inverse

                                p_OutB_LED = 1;
                            }
                        }
                    } else {
                        f_vj_on = 0;
                        last_vj_state = 8;
                        flash_time_laser = 40;
                    }
                }

                if (f_Key_Trig3)// CN1/V
                {
                    p_OutB_LED = 1;
                    f_Key_Trig3 = 0;
                    f_2k_on = 1;

                    if (1 == stepv) {
                        if (!f_mode2) {
                            p_OutB_V1 = 1;
                        }

                        f_V1_on = 1;

                        stepv = 2;
                    } else if (2 == stepv) {
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
                            stepv = 3;
                        } else {
                            f_V1_on = 0;
                            p_OutB_V1 = 0;

                            stepv = 1;
                        }
                    } else if (3 == stepv) {
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

                            stepv = 4;
                        } else {
                            f_V1_on = 0;
                            f_V2_on = 0;

                            p_OutB_V1 = 0;
                                p_OutB_V2 = 0;

                            stepv = 1;
                        }
                    } else if (4 == stepv) {
                        f_V1_on = 0;
                        f_V2_on = 0;
                        f_V3_on = 0;

                        p_OutB_V1 = 0;
                        p_OutB_V2 = 0;
                        p_OutA_V3 = 0;

                        stepv = 1;
                    }
                }

                if (f_Key_Trig4)// CN1/H
                {
                    p_OutB_LED = 1;
                    f_Key_Trig4 = 0;
                    f_2k_on = 1;

                    steph++;

                    if (1 == steph) {
                        if (f_H1_on) {// Current is ON
                            f_H1_on = 0;
                            p_OutB_H1 = 0;
                        } else {// Current is OFF
                            f_H1_on = 1;

                            if (!f_mode2) {
                                p_OutB_H1 = 1;
                            }
                        }
                    } else if (2 == steph) {
                        steph = 0;

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
                    // Enable Timer2 PWM to 2KHz
                    tm2ct = 0x0;
                    tm2b = 0b0111_1100;// 124
                    //tm2s = 0b000_01111;// 15
                    tm2s = 0b000_00111;// 7
                    tm2c = 0b0001_1000;// CLK(=IHRC/2) | PA3 | Period | Disable Inverse
                }

                cnt_3s_time_2k++;
                // ring 120ms
                if (10 == cnt_3s_time_2k) {
                    f_2k_on = 0;
                    cnt_3s_time_2k = 0;

                    // Disable Timer2 PWM
                    tm2c = 0b0000_0000;// IHRC | PA3 | Period | Disable Inverse
                    p_OutA_2K    =    0;
                }
            }

            break;
        }
    }
}