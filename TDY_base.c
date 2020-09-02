#include    "extern.h"

//#define USE_20K 1
#define USE_10K 1

BIT     p_InB_OD    :   PB.2;
BIT     p_InB_VJ    :   PB.5;
BIT     p_InB_V     :   PB.1;
BIT     p_InA_H     :   PA.7;
BIT     p_InA_RF    :   PA.6;

BIT     p_OutB_2K   :   PB.6;
BIT     p_OutA_V1   :   PA.0;
BIT     p_OutA_V2   :   PA.4;
BIT     p_OutB_H1   :   PB.0;
BIT     p_OutA_H2   :   PA.3;

void FPPA0 (void)
{
    .ADJUST_IC    SYSCLK=IHRC/4        //    SYSCLK=IHRC/4

    $    EOSCR        DIS_LVD_BANDGAP;

    $    p_OutB_2K            Out, Low;
    $    p_OutA_V1            Out, Low;
    $    p_OutA_V2            Out, Low;
    $    p_OutB_H1            Out, Low;
    $    p_OutA_H2            Out, Low;

    $    p_InB_OD             In;
    $    p_InB_VJ             In;
    $    p_InB_V              In;
    $    p_InA_H              In;

    // IN Pull-UP
    PAPH        =        _FIELD(p_InA_H, p_InA_RF);
    PBPH        =        _FIELD(p_InA_H, p_InB_OD, p_InB_V);

#ifdef USE_10K
    $ T16M        IHRC, /4, BIT9;                // 256us
#endif

#ifdef USE_20K
    $ T16M        IHRC, /4, BIT8;                // 128us
#endif

    $ TM2C        IHRC, Disable, Period, Inverse;

    BYTE    Key_Flag;
    Key_Flag            =    _FIELD(p_InB_OD, p_InB_VJ, p_InB_V, p_InA_H);

    BYTE    Sys_Flag    =    0;
    BIT        f_Key_Trig1      :    Sys_Flag.0;
    BIT        t16_10ms         :    Sys_Flag.1;
    BIT        f_Key_Trig3      :    Sys_Flag.3;
    BIT        f_Key_Trig4      :    Sys_Flag.4;
    BIT        f_IN_QV2         :    Sys_Flag.5;
    BIT        f_IN_QH2         :    Sys_Flag.6;

    BYTE    Sys_FlagB    =    0;
    BIT        f_mode2          :    Sys_FlagB.0;
    BIT        f_2k_on          :    Sys_FlagB.1;
    BIT        f_led_flash      :    Sys_FlagB.2;
    BIT        f_led_state      :    Sys_FlagB.3;
    BIT        f_vj_on          :    Sys_FlagB.4;
	BIT		   f_sync_ok		:	 Sys_FlagB.5;
	BIT		   f_last_level		:	 Sys_FlagB.6;
	BIT		   f_ev1527_ok		:	 Sys_FlagB.7;

    BYTE    Sys_FlagC    =    0;
    BIT        f_V1_on          :    Sys_FlagC.0;
    BIT        f_V2_on          :    Sys_FlagC.1;
    BIT        f_H1_on          :    Sys_FlagC.2;
    BIT        f_H2_on          :    Sys_FlagC.3;

    BYTE    Sys_FlagD    =    0;
    BIT        f_last_V1_on     :    Sys_FlagD.0;
    BIT        f_last_V2_on     :    Sys_FlagD.1;
    BIT        f_last_H1_on     :    Sys_FlagD.2;
    BIT        f_last_H2_on     :    Sys_FlagD.3;

//    pmode    Program_Mode;
//    fppen    =    0xFF;

    BYTE    count1 = 1;
    BYTE    count_l = 0;
    BYTE    count_h = 0;
    BYTE    last_vj_state = 8;

    BYTE    cnt_Key_10ms_1    =    250;              //    Key debounce time = 40 mS
    BYTE    cnt_Key_10ms_2    =    4;                //    Key debounce time = 40 mS
    BYTE    cnt_Key_10ms_3    =    4;                //    Key debounce time = 40 mS

    BYTE    cnt_Key_10ms_3_mode2    =    250;                //    Key debounce time = 40 mS
    BYTE    cnt_Key_10ms_4_mode2    =    250;                //    Key debounce time = 40 mS

    BYTE    cnt_3s_time_1       = 0;//
    BYTE    cnt_3s_time_2k      = 0;// 2KHz
    BYTE    cnt_3s_time_led     = 0;// 1Hz
    BYTE    cnt_3s_time_startup = 0;//

    BYTE    stepx = 0;
    BYTE    stepv = 0;
    BYTE    steph = 0;
    BYTE    start = 0;

    BYTE    always_low_cnt = 0;
    BYTE    always_high_cnt = 0;
    BYTE    dat_bit_cnt = 0;

    BYTE    tmp_byte1 = 0;
    BYTE    tmp_byte2 = 0;
    BYTE    tmp_byte3 = 0;
    BYTE    tmp_byte4 = 0;
    BYTE    ev1527_byte1 = 0;
    BYTE    ev1527_byte2 = 0;
    BYTE    ev1527_byte3 = 0;
    BYTE    ev1527_byte4 = 0;

#ifdef USE_10K
    WORD    count    =    112;
#endif
#ifdef USE_20K
    WORD    count    =    64;
#endif

    f_mode2 = 1;
    f_2k_on = 0;

#if 0// TODO
    if (p_InA_QV2) {// HIGH
#endif
    if (1) {
        f_IN_QH2 = 1;
    } else {
        f_IN_QH2 = 0;
    }

#if 0// TODO
    if (p_InA_QV2) {// HIGH
#endif
    if (1) {
        f_IN_QV2 = 1;
    } else {
        f_IN_QV2 = 0;
    }

    while (1) {
        if (INTRQ.T16) {// = 10KHz=100us
            INTRQ.T16        =    0;
            stt16    count;

            if (--count1 == 0) {
                count1      =    100;                // 100us * 100 = 10 ms
                t16_10ms    =    1;
            }

            // 1->100
            count_l++;

            if (100 == count_l) {
                count_l = 0;
                count_h++;
                if (100 == count_h)
                    count_h = 0;
            }

            if (1 == start) {
                if (!f_ev1527_ok) {
                    if (!p_InA_RF) {// LOW
//                        p_InB_V = 0;
                        always_low_cnt++;

                        if (always_low_cnt >= 141) {
                            always_low_cnt  = 0;
                            always_high_cnt = 0;
                            dat_bit_cnt = 0;
                            f_sync_ok = 0;
                            tmp_byte1 = 0;
                            tmp_byte2 = 0;
                            tmp_byte3 = 0;
                            tmp_byte4 = 0;
                        }

                        f_last_level = 0;
                    } else {
//                        p_InB_V = 1;

                        if (!f_last_level) {
#if 0
                            if (0 == count_x) {
                                count_x = 1;
                                p_OutA_V2 = 0;
                            } else {
                                count_x = 0;
                                p_OutA_V2 = 1;
                            }
#endif
                            // always_high_cnt=1->2->3, 3-1=2: [200us,300us)
                            // always_high_cnt=1->2->3->4->5, 5-1=4: [400us,500us)
                            // always_high_cnt=[3,5] = [200us,500us)
                            if (((always_high_cnt>=2)&&(always_high_cnt<=5))&&((always_low_cnt>=100)&&(always_low_cnt<=131))) {
                                dat_bit_cnt = 0; f_sync_ok = 1; tmp_byte1 = 0; tmp_byte2 = 0; tmp_byte3 = 0; tmp_byte4 = 0;
                            } else if ((f_sync_ok)&&((always_low_cnt>=8)&&(always_low_cnt<=15))) {
                                if ((always_high_cnt<2) || (always_high_cnt>5)) {
                                    always_low_cnt  = 0;
                                    always_high_cnt = 0;
                                    dat_bit_cnt = 0; f_sync_ok = 0; tmp_byte1 = 0; tmp_byte2 = 0; tmp_byte3 = 0; tmp_byte4 = 0;
                                } else {
#if 1
                                    if (0 == dat_bit_cnt) {
                                        p_InB_V = 1;
                                    }

                                    if (10 == dat_bit_cnt) {
                                        p_InB_V = 0;
                                    }

                                    if (15 == dat_bit_cnt) {
                                        p_InB_V = 1;
                                    }

                                    if (dat_bit_cnt >= 20) {
                                        p_InB_V = 0;
                                    }
#endif
                                    if (23 == dat_bit_cnt) {
                                        ev1527_byte1 = tmp_byte1; ev1527_byte2 = tmp_byte2;
                                        ev1527_byte3 = tmp_byte3; ev1527_byte4 = tmp_byte4;
                                        f_ev1527_ok = 1;
                                    }

                                    dat_bit_cnt++;
                                    if (dat_bit_cnt > 24) {
                                        always_low_cnt  = 0;
                                        always_high_cnt = 0;
                                        dat_bit_cnt = 0; f_sync_ok = 0; tmp_byte1 = 0; tmp_byte2 = 0; tmp_byte3 = 0; tmp_byte4 = 0;
                                    }
                                }
                            } else if ((f_sync_ok)&&((always_low_cnt>=2)&&(always_low_cnt<=5))) {
                                if ((always_high_cnt<8) || (always_high_cnt>15)) {
                                    always_low_cnt  = 0;
                                    always_high_cnt = 0;
                                    dat_bit_cnt = 0; f_sync_ok = 0; tmp_byte1 = 0; tmp_byte2 = 0; tmp_byte3 = 0; tmp_byte4 = 0;
                                } else {
#if 1
                                    if (0 == dat_bit_cnt) {
                                        p_InB_V = 1;
                                    }

                                    if (10 == dat_bit_cnt) {
                                        p_InB_V = 0;
                                    }

                                    if (15 == dat_bit_cnt) {
                                        p_InB_V = 1;
                                    }
                                    if (dat_bit_cnt >= 20) {
                                        p_InB_V = 1;
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
                                                   tmp_byte4 = tmp_byte4 | 0B00010000;
                                                   ev1527_byte1 = tmp_byte1; ev1527_byte2 = tmp_byte2;
                                                   ev1527_byte3 = tmp_byte3; ev1527_byte4 = tmp_byte4;
                                                   f_ev1527_ok = 1;
                                                   f_2k_on = 1;
                                                   break;
                                        }
                                        default: { break; }
                                    }

                                    dat_bit_cnt++;
                                    if (dat_bit_cnt > 24) {
                                        dat_bit_cnt = 0; f_sync_ok = 0; tmp_byte1 = 0; tmp_byte2 = 0; tmp_byte3 = 0; tmp_byte4 = 0;
                                    }
                                }
                            } else {
                                dat_bit_cnt = 0; f_sync_ok = 0; tmp_byte1 = 0; tmp_byte2 = 0; tmp_byte3 = 0; tmp_byte4 = 0;
                            }

                            always_low_cnt  = 0;
                            always_high_cnt = 0;
                        } else {
                            // do nothing
                        }

                        if (always_high_cnt >= 16) {
                            always_low_cnt  = 0;
                            always_high_cnt = 0;
                            dat_bit_cnt = 0; f_sync_ok = 0; tmp_byte1 = 0; tmp_byte2 = 0; tmp_byte3 = 0; tmp_byte4 = 0;
                        }

                        always_high_cnt++;

                        f_last_level = 1;
                    }
                } else {
                    p_InB_V = 0;
                }

                if (f_ev1527_ok) {
                    f_2k_on = 1;
                    f_ev1527_ok = 0;

                    if (1 == ev1527_byte4) {
                        f_Key_Trig1 = 1;
                    } else if (2 == ev1527_byte4) {
                        f_Key_Trig3 = 1;
                    } else if (4 == ev1527_byte4) {
                        f_Key_Trig4 = 1;
                    } else if (8 == ev1527_byte4) {
                    }
                }
            }
        }

        if (f_mode2) {
            if ((1 == count_l)&&(0 == count_h)) {
                if (f_V1_on) {
                    p_OutA_V1 = 1;
                }
                if (f_H2_on) {
                    p_OutA_H2 = 1;
                }
                p_OutB_H1 = 0;
                p_OutA_V2 = 0;
            } else if ((40 == count_l)&&(0 == count_h)) {
                p_OutA_V1 = 0;
                p_OutA_H2 = 0;
                if (f_V2_on) {
                    p_OutA_V2 = 1;
                }
                if (f_H1_on) {
                    p_OutB_H1 = 1;
                }
            } else if ((78 == count_l)&&(0 == count_h)) {
                count_l = 0;
                count_h = 0;
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
                f_H2_on = 0;
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
                    if (f_IN_QV2) {
                        if (!f_mode2) {
                            p_OutA_V2 = 1;
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
            } else if (200 == cnt_3s_time_startup) {
                if ((3 == stepx) && (0 == start)) {
                    if (f_IN_QH2) {
                        if (!f_mode2) {
                            p_OutA_H2 = 1;
                        }

                        f_H2_on = 1;

                        stepx = 4;
                        f_2k_on = 1;
                    } else {
                        f_V1_on = 0;
                        f_V2_on = 0;

                        p_OutA_V1 = 0;
                        p_OutA_V2 = 0;
                        start = 1;
                        stepx = 1;
                    }
                }
            } else if (249 == cnt_3s_time_startup) {
                if ((4 == stepx) && (0 == start)) {
                    f_V1_on = 0;
                    f_V2_on = 0;
                    f_H2_on = 0;

                    p_OutA_V1 = 0;
                    p_OutA_V2 = 0;
                    p_OutA_H2 = 0;

                    stepx = 1;
                    stepv = 1;
                    start = 1;

                    if (f_H1_on) {
                        steph = 2;
                    } else {
                        steph = 1;
                    }
                }
            }

//            else if (250 == cnt_3s_time_startup) {
            if (1 == start) {
                // port change detect(both H->L and L->H)
                A    =    (PB ^ Key_Flag) & _FIELD(p_InB_OD);    //    only check the bit of p_Key_In.
                if (! ZF)
                {                                        //    if is not same,
                    // Active: H->L
                    if (!p_InB_OD) {
                        if (--cnt_Key_10ms_1 == 0)
                        {                                    //    and over debounce time.
                            Key_flag    ^=    _FIELD(p_InB_OD);
                            f_Key_Trig1    =    1;                //    so Trigger, when stable at 3000 mS.
                            cnt_Key_10ms_1    =    250;
                        }

                        if (cnt_Key_10ms_1 == 245) {
                            f_2k_on = 1;
                        }
                    } else {// Up: H->L
                        Key_flag    ^=    _FIELD(p_InB_OD);
                    }
                } else {
                    cnt_Key_10ms_1    =    250;
                }

                A    =    (PB ^ Key_flag/*_mode2*/) & _FIELD(p_InB_V);    //    only check the bit of p_Key_In.
                if (! ZF)
                {                                        //    if is not same,
                    // Active: H->L
                    if (!p_InB_V) {
                        if (--cnt_Key_10ms_3_mode2 == 0)
                        {                                    //    and over debounce time.
                            Key_flag/*_mode2*/    ^=    _FIELD(p_InB_V);
                            //f_Key_Trig3_mode2    =    1;                //    so Trigger, when stable at 30 mS.
                            cnt_Key_10ms_3_mode2 = 250;
                        }
                    } else {// Up: H->L
                        Key_flag/*_mode2*/    ^=    _FIELD(p_InB_V);
                    }
                } else {
                    if (cnt_Key_10ms_3_mode2 < 245) {
                        Key_flag/*_mode2*/    ^=    _FIELD(p_InB_V);
                        f_Key_Trig3 = 1;
                    }
                    cnt_Key_10ms_3_mode2 = 250;
                }

                A    =    (PA ^ Key_flag/*_mode2*/) & _FIELD(p_InA_H);    //    only check the bit of p_Key_In.
                if (! ZF)
                {                                        //    if is not same,
                    // Active: H->L
                    if (!p_InA_H) {
                        if (--cnt_Key_10ms_4_mode2 == 0)
                        {                                    //    and over debounce time.
                            Key_flag/*_mode2*/    ^=    _FIELD(p_InA_H);
                            //f_Key_Trig4_mode2    =    1;                //    so Trigger, when stable at 30 mS.
                            cnt_Key_10ms_4_mode2 = 250;
                        }
                    } else {// Up: H->L
                        Key_flag/*_mode2*/    ^=    _FIELD(p_InA_H);
                    }
                } else {
                    if (cnt_Key_10ms_4_mode2 < 245) {
                        Key_flag/*_mode2*/    ^=    _FIELD(p_InA_H);
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
                        pwmg1c = 0b0000_0000;// do not output PWM

                        p_OutB_2K    =    0;
                    }
                }

                if (1)
                {
                    // p_OutB_LED = 1;

                    // Normal Mode
                    if (0 == cnt_3s_time_1) {
                        // Down: L->H
                        if (p_InB_VJ) {// special mode
                            if (last_vj_state != 1) {
                                last_vj_state = 1;

                                f_vj_on = 1;
                                f_led_flash = 1;

                                // Disable all 100HZ PWM
                                p_OutA_V1    =    0;
                                p_OutA_V2    =    0;
                                p_OutB_H1    =    0;
                                p_OutA_H2    =    0;

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

                                if (f_H2_on) {
                                    f_last_H2_on = 1;
                                } else {
                                    f_last_H2_on = 0;
                                }

                                f_V1_on = 0;
                                f_V2_on = 0;
                                f_H1_on = 0;
                                f_H2_on = 0;

                                // Enable PWMG1C to 2KHz
                                pwmgcubl = 0b0000_0000;
                                pwmgcubh = 0b0001_1111;

                                pwmg1dtl = 0b1000_0000;
                                pwmg1dth = 0b0000_1111;

                                pwmg1c = 0b0000_0010;// PB6 PWM
                                pwmgclk = 0b1100_0000;// enable PWMG CLK(=SYSCLK/16)
                            }
                        } else {
                            if (last_vj_state != 0) {
                                last_vj_state = 0;

                                f_vj_on = 0;
                                f_led_flash = 0;

                                // Disable 2KHz
                                pwmg1c = 0b0000_0000;// do not output PWM

                                // p_OutB_LED = 1;

                                if (f_last_V1_on) {
                                    if (!f_mode2) {
                                        p_OutA_V1    =    1;
                                    }

                                    f_V1_on = 1;
                                }
                                if (f_last_V2_on) {
                                    if (!f_mode2) {
                                        p_OutA_V2    =    1;
                                    }

                                    f_V2_on = 1;
                                }
                                if (f_last_H1_on) {
                                    if (!f_mode2) {
                                        p_OutB_H1    =    1;
                                    }

                                    f_H1_on = 1;
                                }
                                if (f_last_H2_on) {
                                    if (!f_mode2) {
                                        p_OutA_H2    =    1;
                                    }

                                    f_H2_on = 1;
                                }
                            }
                        }
                    }
                }

                if (f_Key_Trig3)// CN1/V
                {
                    // p_OutB_LED = 1;
                    f_Key_Trig3 = 0;
                    f_2k_on = 1;

                    // L -> H
                    if (!p_InB_VJ) {
                        if (1 == stepv) {
                            if (!f_mode2) {
                                p_OutA_V1 = 1;
                            }

                            f_V1_on = 1;

                            stepv= 2;
                        } else if (2 == stepv) {
#if 0// TODO
                            if (p_InA_QV2) {// HIGH
#endif
                            if (1) {
                                f_IN_QV2 = 1;
                            } else {
                                f_IN_QV2 = 0;
                            }

                            if (f_IN_QV2) {
                                if (!f_mode2) {
                                    p_OutA_V2 = 1;
                                }

                                f_V2_on = 1;
                                stepv = 3;
                            } else {
                                f_V1_on = 0;
                                p_OutA_V1 = 0;

                                stepv = 1;
                            }
                        } else if (3 == stepv) {
                            f_V1_on = 0;
                            f_V2_on = 0;

                            p_OutA_V1 = 0;
                            p_OutA_V2 = 0;

                            stepv = 1;
                        }
                    }
                }

                if (f_Key_Trig4)// CN1/H
                {
                    // p_OutB_LED = 1;
                    f_Key_Trig4 = 0;
                    f_2k_on = 1;

                    // L -> H
                    if (!p_InB_VJ) {
                        if (1 == steph) {
                            if (!f_mode2) {
                                p_OutB_H1 = 1;
                            }

                            f_H1_on = 1;

                            steph = 2;
                        } else if (2 == steph) {
                            if (f_IN_QH2) {
                                if (!f_mode2) {
                                    p_OutA_H2 = 1;
                                }

                                f_H2_on = 1;
                                steph = 3;
                            } else {
                                f_H1_on = 0;
                                p_OutB_H1 = 0;

                                steph = 1;
                            }
                        } else if (3 == steph) {
                            f_H1_on = 0;
                            f_H2_on = 0;

                            p_OutB_H1 = 0;
                            p_OutA_H2 = 0;

                            steph = 1;
                        }
                    }
                }

                if (f_led_flash) {
                    cnt_3s_time_led++;
                    if (16 == cnt_3s_time_led) {
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
                if (0 == cnt_3s_time_2k) {
                    // Enable PWMG0C to 2KHz
                    pwmgcubl = 0b0000_0000;
                    pwmgcubh = 0b0001_1111;

                    pwmg1dtl = 0b1000_0000;
                    pwmg1dth = 0b0000_1111;

                    pwmg1c = 0b0000_0010;// PB6 PWM
                    pwmgclk = 0b1100_0000;// enable PWMG CLK(=SYSCLK/16)
                }

                cnt_3s_time_2k++;
                // ring 120ms
                if (10 == cnt_3s_time_2k) {
                    f_2k_on = 0;
                    cnt_3s_time_2k = 0;

                    // Disable 2KHz
                    pwmg1c = 0b0000_0000;// do not output PWM
                    p_OutB_2K    =    0;
                }
            }

            break;
        }
    }
}
