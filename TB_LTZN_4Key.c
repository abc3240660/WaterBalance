#include    "extern.h"

//#define OFF_WAR 1
//#define USE_20K 1
#define USE_10K 1

BIT     p_InB_W     :   PB.3;
BIT     p_InB_VJ    :   PB.5;
BIT     p_InB_M     :   PB.4;
BIT     p_InA_V     :   PA.3;
BIT     p_InB_H     :   PB.7;
BIT     p_InA_RF    :   PA.5;

BIT     p_OutA_2K   :   PA.4;
BIT     p_OutA_V1   :   PA.7;
BIT     p_OutB_V2   :   PB.6;
BIT     p_OutA_H1   :   PA.6;

BIT     p_OutB_H2   :   PB.0;// NC
BIT     p_OutB_DAT  :   PB.1;
BIT     p_OutB_RST  :   PB.2;

void FPPA0 (void)
{
    .ADJUST_IC    SYSCLK=IHRC/4        //    SYSCLK=IHRC/4

    $    EOSCR        DIS_LVD_BANDGAP;

    $    p_OutA_2K            Out, Low;
    $    p_OutA_V1            Out, Low;
    $    p_OutB_V2            Out, Low;
    $    p_OutA_H1            Out, Low;
    $    p_OutB_H2            Out, Low;
	$    p_OutB_RST           Out, Low;
	$    p_OutB_DAT           Out, Low;

    $    p_InB_W              In;
    $    p_InB_VJ             In;
    $    p_InB_M              In;
	$    p_InA_V              In;
    $    p_InB_H              In;
	$    p_InA_RF             In;

    // IN Pull-UP
    PAPH        =        _FIELD(p_InA_V, p_InA_RF);
    PBPH        =        _FIELD(p_InB_W, p_InB_M, p_InB_H);

#ifdef USE_10K
    $ T16M        IHRC, /4, BIT9;                // 256us
#endif

#ifdef USE_20K
    $ T16M        IHRC, /4, BIT8;                // 128us
#endif

    $ TM2C        IHRC, Disable, Period, Inverse;

    BYTE    Key_Flag;
    Key_Flag            =    _FIELD(p_InB_W, p_InB_VJ, p_InB_M, p_InB_H);

    BYTE    Sys_Flag    =    0;
    BIT        f_M_Key_Trig     :    Sys_Flag.0;
    BIT        t16_10ms         :    Sys_Flag.1;
    BIT        t16_10ms_rmt     :    Sys_Flag.2;
    BIT        f_V_Key_Trig     :    Sys_Flag.3;
    BIT        f_H_Key_Trig     :    Sys_Flag.4;
	BIT        f_M_Enable       :    Sys_Flag.5;
	BIT        f_Addr_Saved     :    Sys_Flag.5;

    BYTE    Sys_FlagB    =    0;
    BIT        f_2k_on          :    Sys_FlagB.1;
    BIT        f_led_flash      :    Sys_FlagB.2;
    BIT        f_led_state      :    Sys_FlagB.3;
    BIT        f_vj_on          :    Sys_FlagB.4;
    BIT        f_sync_ok        :    Sys_FlagB.5;
    BIT        f_last_level     :    Sys_FlagB.6;
    BIT        f_ev1527_ok      :    Sys_FlagB.7;

    BYTE    Sys_FlagC    =    0;
    BIT        f_V1_on          :    Sys_FlagC.0;
    BIT        f_V2_on          :    Sys_FlagC.1;
    BIT        f_H1_on          :    Sys_FlagC.2;
    BIT        f_H2_on          :    Sys_FlagC.3;
    BIT        f_M_disable      :    Sys_FlagC.4;
    BIT        f_H_disable      :    Sys_FlagC.5;
    BIT        f_V_disable      :    Sys_FlagC.6;
	BIT        f_D_disable      :    Sys_FlagC.7;

//    pmode    Program_Mode;
//    fppen    =    0xFF;

	BYTE    count0 = 0;
    BYTE    count1 = 1;
	BYTE    count2 = 0;
    BYTE    count_l = 0;
    BYTE    count_h = 0;
    BYTE    last_vj_state = 8;

    BYTE    cnt_Key_10ms_1    =    175;              //    Key debounce time = 40 mS
	BYTE    cnt_Key_10ms_2    =    175;              //    Key debounce time = 40 mS
    BYTE    cnt_Key_10ms_3    =    175;              //    Key debounce time = 40 mS
    BYTE    cnt_Key_10ms_4    =    175;              //    Key debounce time = 40 mS

    BYTE    cnt_3s_time_1       = 0;//
    BYTE    cnt_3s_time_2k      = 0;// 2KHz
    BYTE    cnt_3s_time_led     = 0;// 1Hz
    BYTE    flash_time_laser    = 40;
    BYTE    cnt_3s_time_startup = 0;//

    BYTE    stepx = 0;
    BYTE    stepv = 1;
	BYTE    steph = 0;
    BYTE    start = 0;
	BYTE    step_audio = 0;

    BYTE    always_low_cnt = 0;
    BYTE    always_high_cnt = 0;
    BYTE    dat_bit_cnt = 0;

    BYTE    addr_byte1 = 0;
    BYTE    addr_byte2 = 0;
    BYTE    addr_byte3 = 0;

    BYTE    tmp_byte1 = 0;
    BYTE    tmp_byte2 = 0;
    BYTE    tmp_byte3 = 0;
    BYTE    tmp_byte4 = 0;
    BYTE    ev1527_byte1 = 0;
    BYTE    ev1527_byte2 = 0;
    BYTE    ev1527_byte3 = 0;
    BYTE    ev1527_byte4 = 0;

    BYTE    m_disable_cnt = 0;
    BYTE    h_disable_cnt  = 0;
    BYTE    v_disable_cnt  = 0;
	BYTE    d_disable_cnt  = 0;
    BYTE    od_rm_long_cnt = 0;// remote OD long press
    BYTE    od_rm_rels_cnt = 20;// remote OD release
	BYTE    ir_disable_cnt = 0;

	BYTE    val1 = 0;// L
	BYTE    val2 = 1;// H

	// 0-DC(100%) 1-80% 2-60% 3-40% 4-20%
	BYTE	duty_mode = 0;
	BYTE	duty_mode_last = 0;
	
	BYTE	audio_mode = 0;

#ifdef USE_10K
    WORD    count    =    112;
#endif
#ifdef USE_20K
    WORD    count    =    64;
#endif

    f_2k_on = 0;

    while (1) {
        if (INTRQ.T16) {// = 10KHz=100us
            INTRQ.T16        =    0;
            stt16    count;

            if (--count1 == 0) {
                count1       =    100;                // 100us * 100 = 10 ms
                t16_10ms     =    1;
                t16_10ms_rmt =    1;
                
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

            if (1 == start) {
				if (audio_mode != 0) {
					if (0 == step_audio) {
						count0++;
						count2 = 0;

						if (1 == count0) {
							p_OutB_RST = 1;
						} else if (5 == count0) {// 0.4ms
							count0 = 0;
							step_audio = 1;
							
							p_OutB_RST = 0;
						}
					} else if (1 == step_audio) {
						count0++;

						if (count0 >= 100) {// 10ms
							count0 = 0;
							step_audio = 2;
						}
					} else if (2 == step_audio) {
						count0++;

						if (1 == count0) {
							p_OutB_DAT = 1;// 2 3 4 5 6
						} else if (6 == count0) {// 0.5ms
							count2++;
							
							if (count2 == audio_mode) {
								step_audio = 3;
							}

							p_OutB_DAT = 0;// 7 8 9 10(=0) 1
						} else if (10 == count0) {// 0.5ms
							count0 = 0;
						}
					} else if (3 == step_audio) {
						count0 = 0;
						count2 = 0;
						audio_mode = 0;
						step_audio = 0;
					}			
				}

                if (!f_ev1527_ok) {
                    if (!p_InA_RF) {// LOW
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
                        if (!f_last_level) {
                            // always_high_cnt=1->2->3, 3-1=2: [200us,300us)
                            // always_high_cnt=1->2->3->4->5, 5-1=4: [400us,500us)
                            // always_high_cnt=[3,5] = [200us,500us)
                            if (((always_high_cnt>=2)&&(always_high_cnt<=5))&&((always_low_cnt>=90)&&(always_low_cnt<=131))) {
                                dat_bit_cnt = 0; f_sync_ok = 1; tmp_byte1 = 0; tmp_byte2 = 0; tmp_byte3 = 0; tmp_byte4 = 0;
                            } else if ((f_sync_ok)&&((always_low_cnt>=8)&&(always_low_cnt<=15))) {
                                if ((always_high_cnt<2) || (always_high_cnt>5)) {
                                    always_low_cnt  = 0;
                                    always_high_cnt = 0;
                                    dat_bit_cnt = 0; f_sync_ok = 0; tmp_byte1 = 0; tmp_byte2 = 0; tmp_byte3 = 0; tmp_byte4 = 0;
                                } else {
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
                                        case 20: { tmp_byte4=tmp_byte4 | 0B00001000; break; }
                                        case 21: { tmp_byte4=tmp_byte4 | 0B00000100; break; }
                                        case 22: { tmp_byte4=tmp_byte4 | 0B00000010; break; }
                                        case 23: {
                                                   tmp_byte4 = tmp_byte4 | 0B00000001;
                                                   ev1527_byte1 = tmp_byte1; ev1527_byte2 = tmp_byte2;
                                                   ev1527_byte3 = tmp_byte3; ev1527_byte4 = tmp_byte4;
                                                   f_ev1527_ok = 1;
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
                }

                if (f_ev1527_ok) {
                    f_ev1527_ok = 0;
					
					if (!f_Addr_Saved) {
						addr_byte1 = tmp_byte1;
						addr_byte2 = tmp_byte2;
						addr_byte3 = tmp_byte3;

						f_Addr_Saved = 1;
					} else {
						if ((addr_byte1!=ev1527_byte1)||(addr_byte2!=ev1527_byte2)||(addr_byte3!=ev1527_byte3)) {
							ev1527_byte4 = 0;// skip
						}
					}

                    if (1 == ev1527_byte4) {// C -> OD
                        if (!f_M_disable) {// period = 200ms
                            f_M_disable = 1;

                            // short press
							// idle too long(250ms), so not long press
							// one whole eve1527 period = 45ms~52ms
                            if (od_rm_rels_cnt > 10) {// 250ms = 4~5 whole ev1527 period
								duty_mode++;
								if (5 == duty_mode) {
									duty_mode = 0;
								}

                                if (!f_vj_on) {
                                    f_2k_on = 1;
                                }
                                
                                od_rm_long_cnt = 1;
                            } else {
                                if (od_rm_long_cnt < 200) {
                                    od_rm_long_cnt++;
                                }

                                // long press
                                if (8 == od_rm_long_cnt) {// 1.6s
									if (0 == duty_mode) {
										duty_mode = 4;
									} else {
										duty_mode--;
									}

                                    f_M_Key_Trig    =    1;                //    so Trigger, when stable at 3000 mS.
                                }
                            }
                        }
                        
                        od_rm_rels_cnt = 0;
                    } else if (8 == ev1527_byte4) {// B -> V
                        if (!f_V_disable) {
                            if (!f_vj_on) {
                                f_2k_on = 1;
                            }

                            f_V_disable = 1;
                            f_V_Key_Trig = 1;
                        }
                    } else if (4 == ev1527_byte4) {// A -> H
                        if (!f_H_disable) {
                            if (!f_vj_on) {
                                f_2k_on = 1;
                            }

                            f_H_disable = 1;
                            f_H_Key_Trig = 1;
                        }
                    } else if (2 == ev1527_byte4) {// D -> X
						if (!f_D_disable) {
							if (!f_vj_on) {
								f_2k_on = 1;
							}

							f_D_disable = 1;

							duty_mode++;
							if (5 == duty_mode) {
								duty_mode = 0;
							}
						}
                    }
                } else {
                    if (t16_10ms_rmt) {// period = 10ms
                        t16_10ms_rmt = 0;

                        if (od_rm_rels_cnt < 200) {// 2s
                            od_rm_rels_cnt++;
                        }
                    }
                }
            }
        }

        // if vj mode, laser ON 30ms then OFF 220ms
        if (flash_time_laser >= 20) {
			if ((0 == count_l)&&(0 == count_h)) {
				if (f_H1_on) {
					p_OutA_H1 = 1;
				}
				if (f_H2_on) {
					p_OutB_H2 = 1;
				}
				if (f_V1_on) {
					p_OutA_V1 = 1;
				}
				if (f_V2_on) {
					p_OutB_V2 = 1;
				}
			} else if ((val1 == count_l)&&(0 == count_h)) {
				p_OutA_H1 = 0;
				p_OutB_H2 = 0;
				p_OutA_V1 = 0;
				p_OutB_V2 = 0;
			} else if ((0 == count_l)&&(1 == count_h)) {
				count_l = 0;
				count_h = 0;
			}
        } else {
            if (19 == flash_time_laser) {
                p_OutA_H1 = 0;
                p_OutB_H2 = 0;
                p_OutA_V1 = 0;
                p_OutB_V2 = 0;
            }
        }

        while (t16_10ms)
        {
            t16_10ms    =    0;

            if (f_M_disable) {
                m_disable_cnt++;
                
                if (20 == m_disable_cnt) {// 100ms debounce
                    f_M_disable = 0;
                    m_disable_cnt = 0;
                }
            }
            
            if (f_H_disable) {
                h_disable_cnt++;
                
                if (20 == h_disable_cnt) {// 200ms debounce
                    f_H_disable = 0;
                    h_disable_cnt = 0;
                }
            }
            
            if (f_V_disable) {
                v_disable_cnt++;
                
                if (20 == v_disable_cnt) {// 200ms debounce
                    f_V_disable = 0;
                    v_disable_cnt = 0;
                }
            }
			
			if (f_D_disable) {
                d_disable_cnt++;
                
                if (20 == d_disable_cnt) {// 200ms debounce
                    f_D_disable = 0;
                    d_disable_cnt = 0;
                }
            }

            if (cnt_3s_time_startup < 250) {
                cnt_3s_time_startup++;
            }

            if (65 == cnt_3s_time_startup) {
                p_OutA_H1    =    1;

                f_V1_on = 0;
                f_V2_on = 0;
                f_H1_on = 1;
                f_H2_on = 0;
                stepx = 1;
                f_2k_on = 1;
            } else if (110 == cnt_3s_time_startup) {
                if (1 == stepx) {
                    p_OutA_V1 = 1;

                    f_V1_on = 1;

                    stepx = 2;
                    f_2k_on = 1;
                }
            } else if (155 == cnt_3s_time_startup) {
                if (2 == stepx) {
                    p_OutB_V2 = 1;

                    f_V2_on = 1;
                    stepx = 3;
                    f_2k_on = 1;
                 }
            } else if (200 == cnt_3s_time_startup) {
                if ((3 == stepx) && (0 == start)) {
                    f_V1_on = 0;
                    f_V2_on = 0;
                    f_H2_on = 0;

                    p_OutA_V1 = 0;
                    p_OutB_V2 = 0;
                    p_OutB_H2 = 0;

                    stepx = 1;
                    stepv = 1;
                    start = 1;
                }
            }

            if (1 == start) {
                // port change detect(both H->L and L->H)
                A    =    (PB ^ Key_flag) & _FIELD(p_InB_M);    //    only check the bit of p_Key_In.
                if (! ZF)
                {                                        //    if is not same,
                    // Active: H->L
                    if (!p_InB_M) {
                        if (cnt_Key_10ms_1 > 0) {
                            if (--cnt_Key_10ms_1 == 0)// LongPress
                            {
								if (f_M_Enable) {
									audio_mode = 2;
									f_M_Enable = 0;
								} else {
									audio_mode = 5;
									f_M_Enable = 1;
								}
								
								if (!f_vj_on) {
                                    f_2k_on = 1;
                                }
                            }

                            if (cnt_Key_10ms_1 == 170) {
                                if (!f_vj_on) {
                                    f_2k_on = 1;
                                }
                            }
                        }
                    } else {// Up: H->L
                        Key_flag    ^=    _FIELD(p_InB_M);
                    }
                } else {
                    if (cnt_Key_10ms_1 < 170) {
                        if (cnt_Key_10ms_1 != 0) {// Only ShortPress
							if (f_M_Enable) {
								if (duty_mode > 0) {
									duty_mode--;
								}
								
								audio_mode = 3;
							}
                        } else {// LongPress release
						}
                    }

                    cnt_Key_10ms_1    =    175;
                }
				
				A    =    (PA ^ Key_flag) & _FIELD(p_InA_V);    //    only check the bit of p_Key_In.
                if (! ZF)
                {                                        //    if is not same,
                    // Active: H->L
                    if (!p_InA_V) {
                        if (cnt_Key_10ms_2 > 0) {
                            if (--cnt_Key_10ms_2 == 0) {
                                // do not support long press
                            }
                            
                            if (cnt_Key_10ms_2 == 170) {
								f_V_Key_Trig = 1;
                            }
                        }
                    } else {// Up: H->L
                        Key_flag    ^=    _FIELD(p_InA_V);
                    }
                } else {
                    cnt_Key_10ms_2 = 175;
                }

                A    =    (PB ^ Key_Flag) & _FIELD(p_InB_H);    //    only check the bit of p_Key_In.
                if (! ZF)
                {                                        //    if is not same,
                    // Active: H->L
                    if (!p_InB_H) {
                        if (cnt_Key_10ms_3 > 0) {
                            if (--cnt_Key_10ms_3 == 0) {
                                // do not support long press
                            }
                            
                            if (cnt_Key_10ms_3 == 170) {
								f_H_Key_Trig = 1;
                            }
                        }
                    } else {// Up: H->L
                        Key_flag    ^=    _FIELD(p_InB_H);
                    }
                } else {
                    cnt_Key_10ms_3 = 175;
                }

                A    =    (PB ^ Key_flag) & _FIELD(p_InB_W);    //    only check the bit of p_Key_In.
                if (! ZF)
                {                                        //    if is not same,
                    // Active: H->L
                    if (!p_InB_W) {
                        if (cnt_Key_10ms_4 > 0) {
                            if (--cnt_Key_10ms_4 == 0)// LongPress
                            {
								// do not support long press
                            }

                            if (cnt_Key_10ms_4 == 170) {
                                if (!f_vj_on) {
                                    f_2k_on = 1;
                                }
                            }
                        }
                    } else {// Up: H->L
                        Key_flag    ^=    _FIELD(p_InB_W);
                    }
                } else {
                    if (cnt_Key_10ms_4 < 170) {
                        if (cnt_Key_10ms_4 != 0) {// Only ShortPress
							if (f_M_Enable) {
								if (duty_mode < 4) {
									duty_mode++;
								}
								
								audio_mode = 4;
							} else {
								f_M_Key_Trig    =    1;
							}
                        } else {// LongPress release
						}
                    }

                    cnt_Key_10ms_4    =    175;
                }

                if (f_M_Key_Trig)
                {
                    f_M_Key_Trig = 0;

                    if (0 == cnt_3s_time_1) {
                        cnt_3s_time_1 = 1;
                        f_led_flash = 1;
                    } else {
                        cnt_3s_time_1 = 0;
                        f_led_flash = 0;
                    }
                }
				
				if (duty_mode != duty_mode_last) {
					duty_mode_last = duty_mode;
					
					if (1 == duty_mode) {
						val1 = 80;
						val2 = 0;
					} else if (2 == duty_mode) {
						val1 = 60;
						val2 = 0;
					} else if (3 == duty_mode) {
						val1 = 40;
						val2 = 0;
					} else if (4 == duty_mode) {
						val1 = 20;
						val2 = 0;
					} else {// 100%
						val1 = 0;
						val2 = 1;
					}
					
					count_l = 0;
					count_h = 0;
					flash_time_laser = 40;
				}

                // Normal Mode
                if (0 == cnt_3s_time_1) {
#ifndef OFF_WAR
                    // Down: L->H
                    if (p_InB_VJ) {// special mode
                        if (last_vj_state != 1) {
                            last_vj_state = 1;

                            f_vj_on = 1;
                            f_led_flash = 1;
#if 0
							// Enable PWMG1C to 2KHz
							pwmgcubl = 0b0000_0000;
							pwmgcubh = 0b0001_1111;

							pwmg1dtl = 0b1000_0000;
							pwmg1dth = 0b0000_1111;
#endif
							pwmg1c = 0b0000_0110;// PA4 PWM
//							pwmgclk = 0b1100_0000;// enable PWMG CLK(=SYSCLK/16)
                        }
                    } else {
                        if (last_vj_state != 0) {
                            last_vj_state = 0;

                            f_vj_on = 0;
                            f_led_flash = 0;

                            count_l = 0;
                            count_h = 0;
                            flash_time_laser = 40;

							// Disable 2KHz
							pwmg1c = 0b0000_0000;// do not output PWM
                        }
                    }
#endif
                } else {
                    count_l = 0;
                    count_h = 0;
                    f_vj_on = 0;
                    last_vj_state = 8;
                    flash_time_laser = 40;
                }

                if (f_V_Key_Trig)// CN1/V
                {
                    f_V_Key_Trig = 0;

                    if (!f_vj_on) {
                        f_2k_on = 1;
                    }

                    if (1 == stepv) {
                        p_OutA_V1 = 1;
                        p_OutB_V2 = 0;

                        f_V1_on = 1;
                        f_V2_on = 0;

                        stepv= 2;
                    } else if (2 == stepv) {
                        p_OutA_V1 = 0;
                        p_OutB_V2 = 1;

                        f_V1_on = 0;
                        f_V2_on = 1;
                        stepv = 3;
                    } else if (3 == stepv) {
                        p_OutA_V1 = 1;
                        p_OutB_V2 = 1;

                        f_V1_on = 1;
                        f_V2_on = 1;

                        stepv = 4;
                    } else if (4 == stepv) {
                        f_V1_on = 0;
                        f_V2_on = 0;

                        p_OutA_V1 = 0;
                        p_OutB_V2 = 0;

                        stepv = 1;
                    }
                }

                if (f_H_Key_Trig)// CN1/H
                {
                    f_H_Key_Trig = 0;

                    if (!f_vj_on) {
                        f_2k_on = 1;
                    }
                    steph++;

                    if (1 == steph) {
                        if (f_H1_on) {// Current is ON
                            f_H1_on = 0;
                            p_OutA_H1 = 0;
                        } else {// Current is OFF
                            f_H1_on = 1;
                            p_OutA_H1 = 1;
                        }
                    } else if (2 == steph) {
                        steph = 0;

                        if (f_H1_on) {// Current is ON
                            f_H1_on = 0;
                            p_OutA_H1 = 0;
                        } else {// Current is OFF
                            f_H1_on = 1;
                            p_OutA_H1 = 1;
                        }
                    }
                }

                if (f_led_flash) {
                    cnt_3s_time_led++;
                    if (16 == cnt_3s_time_led) {
                        if (f_led_state) {
                            f_led_state = 0;

                            if (f_vj_on) {
                                // 2kHz
                                f_2k_on = 1;
                            }
                        } else {
                            f_led_state = 1;

                            if (f_vj_on) {
                                // 2kHz
                                f_2k_on = 1;
                            }
                        }
                        cnt_3s_time_led = 0;
                    }
                } else {
                    cnt_3s_time_led = 0;
                }
            }

            if (f_2k_on) {
                if (0 == cnt_3s_time_2k) {
                    // Enable PWMG1C to 2KHz
                    pwmgcubl = 0b0000_0000;
                    pwmgcubh = 0b0001_1111;

                    pwmg1dtl = 0b1000_0000;
                    pwmg1dth = 0b0000_1111;

                    pwmg1c = 0b0000_0110;// PA4 PWM
                    pwmgclk = 0b1100_0000;// enable PWMG CLK(=SYSCLK/16)
                }

                cnt_3s_time_2k++;
                // ring 120ms
                if (10 == cnt_3s_time_2k) {
                    f_2k_on = 0;
                    cnt_3s_time_2k = 0;

                    // Disable 2KHz
                    pwmg1c = 0b0000_0000;// do not output PWM
                }
            }

            break;
        }
    }
}