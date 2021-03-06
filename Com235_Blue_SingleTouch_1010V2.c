#include    "extern.h"

//#define USE_20K 1
#define USE_10K 1

BIT            p_InB_M     :   PB.0;
BIT            p_InA_VJ    :   PA.5;
BIT            p_InB_V     :   PB.2;
BIT            p_InB_H     :   PB.1;

BIT            p_OutA_2K   :   PA.3;
BIT            p_OutA_V1   :   PA.4;
BIT            p_OutA_V2   :   PA.6;
BIT            p_OutB_V3   :   PB.5;
BIT            p_OutA_V4   :   PA.0;
BIT            p_OutB_H1   :   PB.6;

// High is active
BIT        p_OutB_LED    :    PB.7;// LED

void    FPPA0 (void)
{
    .ADJUST_IC    SYSCLK=IHRC/4        //    SYSCLK=IHRC/4

    $ EOSCR        DIS_LVD_BANDGAP;

    $    p_OutA_2K            Out, Low;
    $    p_OutA_V1            Out, Low;
    $    p_OutA_V2            Out, Low;
    $    p_OutB_V3            Out, Low;
	$    p_OutA_V4            Out, Low;
    $    p_OutB_H1            Out, Low;
    $    p_OutB_LED           Out, Low;// off

    $    p_InB_M              In;
    $    p_InA_VJ             In;
    $    p_InB_V              In;
    $    p_InB_H              In;

    // IN Pull-UP
    // PAPH        =        _FIELD();
    PBPH        =        _FIELD(p_InB_M, p_InB_H, p_InB_V);

#ifdef USE_10K
    $ T16M        IHRC, /4, BIT9;                // 256us
#endif

#ifdef USE_20K
    $ T16M        IHRC, /4, BIT8;                // 128us
#endif
    $ TM2C        IHRC, Disable, Period, Inverse;

    BYTE    Key_Flag;
    Key_Flag            =    _FIELD(p_InB_M, p_InA_VJ, p_InB_V, p_InB_H);

    BYTE    Sys_Flag    =    0;
    BIT        t16_10ms         :    Sys_Flag.1;

    BYTE    Sys_FlagB    =    0;
    BIT        f_2k_on          :    Sys_FlagB.1;
    BIT        f_led_flash      :    Sys_FlagB.2;
    BIT        f_led_state      :    Sys_FlagB.3;
    BIT        f_vj_on          :    Sys_FlagB.5;

    BYTE    Sys_FlagC    =    0;
    BIT        f_V1_on          :    Sys_FlagC.0;
    BIT        f_V2_on          :    Sys_FlagC.1;
    BIT        f_H1_on          :    Sys_FlagC.2;
    BIT        f_V3_on          :    Sys_FlagC.3;
	BIT        f_V4_on          :    Sys_FlagC.4;
	BIT        f_D_disable      :    Sys_FlagC.6;

//    pmode    Program_Mode;
//    fppen    =    0xFF;

    BYTE    count1 = 1;
    BYTE    count_l = 0;
    BYTE    count_h = 0;
    BYTE    last_vj_state = 8;

    BYTE    cnt_Key_10ms_M    =    175;
    BYTE    cnt_Key_10ms_H    =    175;
    BYTE    cnt_Key_10ms_V1   =    175;
	BYTE    cnt_Key_10ms_V2   =    175;
	BYTE    cnt_Key_10ms_V3   =    175;
	BYTE    cnt_Key_10ms_V4   =    175;

    BYTE    cnt_3s_time_1       = 0;//
    BYTE    cnt_3s_time_2k      = 0;// 2KHz
    BYTE    cnt_3s_time_led     = 0;// 1Hz
    BYTE    flash_time_laser  	= 40;
    BYTE    cnt_3s_time_startup = 0;//

    BYTE    stepx = 0;
    BYTE    stepv = 1;
    BYTE    steph = 0;
    BYTE    start = 0;
	
	BYTE    d_disable_cnt  = 0;

	BYTE    val1 = 0;// L
	BYTE    val2 = 1;// H

	// 0-DC(100%) 1-20% 2-40% 3-60% 4-80%
	BYTE	duty_mode = 0;
	
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
			if ((0 == count_l)&&(0 == count_h)) {
				if (f_H1_on) {
					p_OutB_H1 = 1;
				}
				if (f_V3_on) {
					p_OutB_V3 = 1;
				}
				if (f_V1_on) {
					p_OutA_V1 = 1;
				}
				if (f_V2_on) {
					p_OutA_V2 = 1;
				}
				if (f_V4_on) {
					p_OutA_V4 = 1;
				}
			} else if ((val1 == count_l)&&(0 == count_h)) {
				p_OutB_H1 = 0;
				p_OutB_V3 = 0;
				p_OutA_V1 = 0;
				p_OutA_V2 = 0;
				p_OutA_V4 = 0;
			} else if ((0 == count_l)&&(1 == count_h)) {
				count_l = 0;
				count_h = 0;
			}
        } else {
            if (19 == flash_time_laser) {
                p_OutB_H1 = 0;
                p_OutB_V3 = 0;
                p_OutA_V1 = 0;
                p_OutA_V2 = 0;
				p_OutA_V4 = 0;
            }
        }

        while (t16_10ms) {
            t16_10ms    =    0;

			if (f_D_disable) {
                d_disable_cnt++;
                
                if (10 == d_disable_cnt) {// 200ms debounce
                    f_D_disable = 0;
                    d_disable_cnt = 0;
                }
            }

			if (0 == start) {
				if (cnt_3s_time_startup < 156) {
					cnt_3s_time_startup++;
				}

				if (5 == cnt_3s_time_startup) {
					p_OutB_H1    =    1;

					f_V1_on = 0;
					f_V2_on = 0;
					f_H1_on = 1;
					f_V3_on = 0;
					stepx = 1;
					f_2k_on = 1;
				} else if (35 == cnt_3s_time_startup) {
					if (1 == stepx) {
						p_OutA_V1 = 1;

						f_V1_on = 1;

						stepx = 2;
						f_2k_on = 1;
					}
				} else if (65 == cnt_3s_time_startup) {
					if (2 == stepx) {
						p_OutA_V2 = 1;

						f_V2_on = 1;
						stepx = 3;
						f_2k_on = 1;
					 }
				} else if (95 == cnt_3s_time_startup) {
					if ((3 == stepx) && (0 == start)) {
						p_OutB_V3 = 1;

						f_V3_on = 1;

						stepx = 4;
						f_2k_on = 1;
					}
				} else if (125 == cnt_3s_time_startup) {
					if ((4 == stepx) && (0 == start)) {
						p_OutA_V4 = 1;

						f_V4_on = 1;
						stepx = 5;
						f_2k_on = 1;
					}
				} else if (155 == cnt_3s_time_startup) {
					if ((5 == stepx) && (0 == start)) {
						f_V1_on = 0;
						f_V2_on = 0;
						f_V3_on = 0;
						f_V4_on = 0;

						p_OutA_V1 = 0;
						p_OutA_V2 = 0;
						p_OutB_V3 = 0;
						p_OutA_V4 = 0;

						stepx = 1;
						stepv = 1;
						start = 1;
					}
				}
			} else {
				if (!p_InB_M && !p_InB_V && p_InB_H) {// M
#if 0
					cnt_Key_10ms_H    =    175;
					cnt_Key_10ms_V1   =    175;
					cnt_Key_10ms_V2   =    175;
					cnt_Key_10ms_V3   =    175;
					cnt_Key_10ms_V4   =    175;
#endif
                    if (cnt_Key_10ms_M > 0) {
                        if (--cnt_Key_10ms_M == 0) {// Long Press
							if (!f_vj_on) {
								f_2k_on = 1;
							}

							if (0 == cnt_3s_time_1) {
								cnt_3s_time_1 = 1;
								f_led_flash = 1;
							} else {
								cnt_3s_time_1 = 0;
								f_led_flash = 0;
							}
                        }

                        if (cnt_Key_10ms_M == 170) {
							if (!f_vj_on) {
								f_2k_on = 1;
							}
                        }
                    }
				} else if (p_InB_M && !p_InB_V && !p_InB_H) {// H
#if 0
					cnt_Key_10ms_M    =    175;
					cnt_Key_10ms_V1   =    175;
					cnt_Key_10ms_V2   =    175;
					cnt_Key_10ms_V3   =    175;
					cnt_Key_10ms_V4   =    175;
#endif
					if (cnt_Key_10ms_H > 0) {
						if (--cnt_Key_10ms_H == 0) {
							// Long Press
						}
						
						if (cnt_Key_10ms_H == 170) {
							if (!f_vj_on) {
								f_2k_on = 1;
							}
						}
					}
				} else if (!p_InB_M && p_InB_V && p_InB_H) {// V1
#if 0
					cnt_Key_10ms_M    =    175;
					cnt_Key_10ms_H    =    175;
					cnt_Key_10ms_V2   =    175;
					cnt_Key_10ms_V3   =    175;
					cnt_Key_10ms_V4   =    175;
#endif
					if (cnt_Key_10ms_V1 > 0) {
						if (--cnt_Key_10ms_V1 == 0) {
							// Long Press
						}
						
						if (cnt_Key_10ms_V1 == 170) {
							if (!f_vj_on) {
								f_2k_on = 1;
							}
						}
					}
				} else if (p_InB_M && p_InB_V && !p_InB_H) {// V2
#if 0
					cnt_Key_10ms_M    =    175;
					cnt_Key_10ms_H    =    175;
					cnt_Key_10ms_V1   =    175;
					cnt_Key_10ms_V3   =    175;
					cnt_Key_10ms_V4   =    175;
#endif
					if (cnt_Key_10ms_V2 > 0) {
						if (--cnt_Key_10ms_V2 == 0) {
							// Long Press
						}
						
						if (cnt_Key_10ms_V2 == 170) {
							if (!f_vj_on) {
								f_2k_on = 1;
							}
						}
					}
				} else if (!p_InB_M && p_InB_V && !p_InB_H) {// V3
#if 0
					cnt_Key_10ms_M    =    175;
					cnt_Key_10ms_H    =    175;
					cnt_Key_10ms_V1   =    175;
					cnt_Key_10ms_V2   =    175;
					cnt_Key_10ms_V4   =    175;
#endif

					if (cnt_Key_10ms_V3 > 0) {
						if (--cnt_Key_10ms_V3 == 0) {
							// Long Press
						}
						
						if (cnt_Key_10ms_V3 == 170) {
							if (!f_vj_on) {
								f_2k_on = 1;
							}
						}
					}
				} else if (p_InB_M && !p_InB_V && p_InB_H) {// V4
#if 0
					cnt_Key_10ms_M    =    175;
					cnt_Key_10ms_H    =    175;
					cnt_Key_10ms_V1   =    175;
					cnt_Key_10ms_V2   =    175;
					cnt_Key_10ms_V3   =    175;
#endif
					if (cnt_Key_10ms_V4 > 0) {
						if (--cnt_Key_10ms_V4 == 0) {
							// Long Press
						}
						
						if (cnt_Key_10ms_V4 == 170) {
							if (!f_vj_on) {
								f_2k_on = 1;
							}
						}
					}
				} else {
                    if (cnt_Key_10ms_M <= 170) {
						if (cnt_Key_10ms_M != 0) {// Only ShortPress
							if (!f_D_disable) {
								f_D_disable = 1;

								duty_mode++;
								
								if (duty_mode >= 5) {
									duty_mode  = 0;
								}

								if (1 == duty_mode) {
									val1 = 20;
									val2 = 0;
								} else if (2 == duty_mode) {
									val1 = 40;
									val2 = 0;
								} else if (3 == duty_mode) {
									val1 = 60;
									val2 = 0;
								} else if (4 == duty_mode) {
									val1 = 80;
									val2 = 0;
								} else {// 100%
									val1 = 0;
									val2 = 1;
								}

								count_l = 0;
								count_h = 0;
								flash_time_laser = 40;
							}
						}
                    }

                    if (cnt_Key_10ms_H <= 170) {
						if (cnt_Key_10ms_H != 0) {// Only ShortPress
							if (f_H1_on) {// Current is ON
								f_H1_on = 0;
								p_OutB_H1 = 0;
							} else {// Current is OFF
								f_H1_on = 1;
								p_OutB_H1 = 1;
							}
						}
                    }

                    if (cnt_Key_10ms_V1 <= 170) {
						if (cnt_Key_10ms_V1 != 0) {// Only ShortPress
							if (f_V1_on) {// Current is ON
								f_V1_on = 0;
								p_OutA_V1 = 0;
							} else {// Current is OFF
								f_V1_on = 1;
								p_OutA_V1 = 1;
							}
						}
                    }

                    if (cnt_Key_10ms_V2 <= 170) {
						if (cnt_Key_10ms_V2 != 0) {// Only ShortPress
							if (f_V2_on) {// Current is ON
								f_V2_on = 0;
								p_OutA_V2 = 0;
							} else {// Current is OFF
								f_V2_on = 1;
								p_OutA_V2 = 1;
							}
						}
                    }

                    if (cnt_Key_10ms_V3 <= 170) {
						if (cnt_Key_10ms_V3 != 0) {// Only ShortPress
							if (f_V3_on) {// Current is ON
								f_V3_on = 0;
								p_OutB_V3 = 0;
							} else {// Current is OFF
								f_V3_on = 1;
								p_OutB_V3 = 1;
							}
						}
                    }

                    if (cnt_Key_10ms_V4 <= 170) {
						if (cnt_Key_10ms_V4 != 0) {// Only ShortPress
							if (f_V4_on) {// Current is ON
								f_V4_on = 0;
								p_OutA_V4 = 0;
							} else {// Current is OFF
								f_V4_on = 1;
								p_OutA_V4 = 1;
							}
						}
                    }

					cnt_Key_10ms_M    =    175;
					cnt_Key_10ms_H    =    175;
					cnt_Key_10ms_V1   =    175;
					cnt_Key_10ms_V2   =    175;
					cnt_Key_10ms_V3   =    175;
					cnt_Key_10ms_V4   =    175;
				}

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

							count_l = 0;
							count_h = 0;
                            flash_time_laser = 40;

                            // Disable Timer2 PWM
                            tm2c = 0b0000_0000;// IHRC | PA3 | Period | Disable Inverse

                            p_OutB_LED = 1;
                        }
                    }
                } else {
					count_l = 0;
					count_h = 0;
					f_vj_on = 0;
                    last_vj_state = 8;
					flash_time_laser = 40;
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