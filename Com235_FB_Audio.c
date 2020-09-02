#include    "extern.h"

//#define USE_20K 1
#define USE_10K 1

BIT            p_InA_OD    :   PA.4;
BIT            p_InA_VJ    :   PA.5;
BIT            p_InA_V     :   PA.7;
BIT            p_InB_H     :   PB.1;
BIT            p_OutA_RST  :   PA.0;
BIT            p_OutB_DAT  :   PB.0;

BIT            p_OutA_2K   :   PA.3;
BIT            p_OutB_V1   :   PB.2;
BIT            p_OutB_V2   :   PB.5;
BIT            p_OutA_H2   :   PA.6;
BIT            p_OutB_H1   :   PB.6;

// High is active
BIT        p_OutB_LED    :    PB.7;// LED

void    FPPA0 (void)
{
    .ADJUST_IC    SYSCLK=IHRC/4        //    SYSCLK=IHRC/4

    $ EOSCR        DIS_LVD_BANDGAP;

    $    p_OutA_2K            Out, Low;
    $    p_OutB_V1            Out, Low;
    $    p_OutB_V2            Out, Low;
    $    p_OutA_H2            Out, Low;
    $    p_OutB_H1            Out, Low;
    $    p_OutB_LED           Out, Low;// off
    $    p_OutA_RST           Out, Low;
    $    p_OutB_DAT           Out, Low;

    $    p_InA_OD             In;
    $    p_InA_VJ             In;
    $    p_InA_V              In;
    $    p_InB_H              In;

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
    Key_Flag            =    _FIELD(p_InA_OD, p_InA_VJ, p_InA_V, p_InB_H);

    BYTE    Sys_Flag    =    0;
    BIT        f_Key_Trig1      :    Sys_Flag.0;
    BIT        t16_10ms         :    Sys_Flag.1;
    BIT        f_Key_Trig3      :    Sys_Flag.3;
    BIT        f_Key_Trig4      :    Sys_Flag.4;
	BIT        f_M_Enable       :    Sys_Flag.7;

    BYTE    Sys_FlagB    =    0;
    BIT        f_2k_on          :    Sys_FlagB.1;
    BIT        f_led_flash      :    Sys_FlagB.2;
    BIT        f_led_state      :    Sys_FlagB.3;
    BIT        f_vj_on          :    Sys_FlagB.5;

    BYTE    Sys_FlagC    =    0;
    BIT        f_V1_on          :    Sys_FlagC.0;
    BIT        f_V2_on          :    Sys_FlagC.1;
    BIT        f_H1_on          :    Sys_FlagC.2;
    BIT        f_H2_on          :    Sys_FlagC.3;

//    pmode    Program_Mode;
//    fppen    =    0xFF;

	BYTE    count0 = 0;
    BYTE    count1 = 1;
	BYTE    count2 = 0;
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
    BYTE    cnt_3s_time_startup = 0;//

    BYTE    stepx = 0;
    BYTE    stepv = 1;
    BYTE    steph = 0;
    BYTE    start = 0;
	BYTE    step_audio = 0;

	BYTE    val1 = 0;// L
	BYTE    val2 = 1;// H

	// 0-DC(100%) 1-80% 2-60% 3-40% 4-20%
	BYTE	duty_mode = 0;
	
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
			
			if (audio_mode != 0) {
				if (0 == step_audio) {
					count0++;
					count2 = 0;

					if (1 == count0) {
						p_OutA_RST = 1;
					} else if (5 == count0) {// 0.4ms
						count0 = 0;
						step_audio = 1;
						
						p_OutA_RST = 0;
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
        }

		// if vj mode, laser ON 30ms then OFF 220ms
        if (flash_time_laser >= 20) {
			if ((0 == count_l)&&(0 == count_h)) {
				if (f_H1_on) {
					p_OutB_H1 = 1;
				}
				if (f_H2_on) {
					p_OutA_H2 = 1;
				}
				if (f_V1_on) {
					p_OutB_V1 = 1;
				}
				if (f_V2_on) {
					p_OutB_V2 = 1;
				}
			} else if ((val1 == count_l)&&(0 == count_h)) {
				p_OutB_H1 = 0;
				p_OutA_H2 = 0;
				p_OutB_V1 = 0;
				p_OutB_V2 = 0;
			} else if ((0 == count_l)&&(1 == count_h)) {
				count_l = 0;
				count_h = 0;
			}
        } else {
            if (19 == flash_time_laser) {
                p_OutB_H1 = 0;
                p_OutA_H2 = 0;
                p_OutB_V1 = 0;
                p_OutB_V2 = 0;
            }
        }

        while (t16_10ms)
        {
            t16_10ms    =    0;

            if (cnt_3s_time_startup < 250) {
                cnt_3s_time_startup++;
            }

            if (25 == cnt_3s_time_startup) {
                p_OutB_H1    =    1;

                f_V1_on = 0;
                f_V2_on = 0;
                f_H1_on = 1;
                f_H2_on = 0;
                stepx = 1;
                f_2k_on = 1;
            } else if (70 == cnt_3s_time_startup) {
                if (1 == stepx) {
                    p_OutB_V1 = 1;

                    f_V1_on = 1;

                    stepx = 2;
                    f_2k_on = 1;
                }
            } else if (115 == cnt_3s_time_startup) {
                if (2 == stepx) {
                    p_OutB_V2 = 1;

                    f_V2_on = 1;
                    stepx = 3;
                    f_2k_on = 1;
                 }
            } else if (160 == cnt_3s_time_startup) {
                if ((3 == stepx) && (0 == start)) {
                    p_OutA_H2 = 1;

                    f_H2_on = 1;

                    stepx = 4;
                    f_2k_on = 1;
                }
            } else if (209 == cnt_3s_time_startup) {
                if ((4 == stepx) && (0 == start)) {
                    f_V1_on = 0;
                    f_V2_on = 0;
                    f_H2_on = 0;

                    p_OutB_V1 = 0;
                    p_OutB_V2 = 0;
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

            if (1 == start) {
                // port change detect(both H->L and L->H)
                A    =    (PA ^ Key_Flag) & _FIELD(p_InA_OD);    //    only check the bit of p_Key_In.
                if (! ZF)
                {                                        //    if is not same,
                    // Active: H->L
                    if (!p_InA_OD) {
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
                        Key_flag    ^=    _FIELD(p_InA_OD);
                    }
                } else {
                    if (cnt_Key_10ms_1 < 170) {
                        if (cnt_Key_10ms_1 != 0) {// Only ShortPress
                            f_Key_Trig1    =    1;
							audio_mode = 1;
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
                        if (cnt_Key_10ms_3 > 0) {
                            if (--cnt_Key_10ms_3 == 0) {
                                // do not support long press
                            }
                            
                            if (cnt_Key_10ms_3 == 170) {
								if (f_M_Enable) {
									audio_mode = 3;

									if (!f_vj_on) {
										f_2k_on = 1;
									}

									if (duty_mode < 4) {
										duty_mode++;
									}
									
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
								} else {
									f_Key_Trig3 = 1;
								}
                            }
                        }
                    } else {// Up: H->L
                        Key_flag    ^=    _FIELD(p_InA_V);
                    }
                } else {
                    cnt_Key_10ms_3 = 175;
                }

                A    =    (PB ^ Key_flag) & _FIELD(p_InB_H);    //    only check the bit of p_Key_In.
                if (! ZF)
                {                                        //    if is not same,
                    // Active: H->L
                    if (!p_InB_H) {
                        if (cnt_Key_10ms_4 > 0) {
                            if (--cnt_Key_10ms_4 == 0) {
                                // do not support long press
                            }
                            
                            if (cnt_Key_10ms_4 == 170) {
								if (f_M_Enable) {
									audio_mode = 4;

									if (!f_vj_on) {
										f_2k_on = 1;
									}

									if (duty_mode > 0) {
										duty_mode--;
									}
									
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
								} else {
									f_Key_Trig4 = 1;
								}
                            }
                        }
                    } else {// Up: H->L
                        Key_flag    ^=    _FIELD(p_InB_H);
                    }
                } else {
                    cnt_Key_10ms_4 = 175;
                }

                if (f_Key_Trig1)
                {
                    f_Key_Trig1 = 0;

#if 0
                    if (!f_vj_on) {
                        f_2k_on = 1;
                    }
#endif

                    if (0 == cnt_3s_time_1) {
                        cnt_3s_time_1 = 1;
                        f_led_flash = 1;
                    } else {
                        cnt_3s_time_1 = 0;
                        f_led_flash = 0;
                    }
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
							tm2c = 0b0000_0000;// IHRC | PB2 | Period | Disable Inverse
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
                    f_Key_Trig3 = 0;

                    if (!f_vj_on) {
                        f_2k_on = 1;
                    }

					if (1 == stepv) {
						p_OutB_V1 = 1;
						p_OutB_V2 = 0;

						f_V1_on = 1;
						f_V2_on = 0;

						stepv= 2;
					} else if (2 == stepv) {
						p_OutB_V1 = 0;
						p_OutB_V2 = 1;

						f_V1_on = 0;
						f_V2_on = 1;
						stepv = 3;
					} else if (3 == stepv) {
						p_OutB_V1 = 1;
						p_OutB_V2 = 1;

						f_V1_on = 1;
						f_V2_on = 1;

						stepv = 4;
					} else if (4 == stepv) {
						f_V1_on = 0;
						f_V2_on = 0;

						p_OutB_V1 = 0;
						p_OutB_V2 = 0;

						stepv = 1;
					}
                }

                if (f_Key_Trig4)// CN1/H
                {
                    f_Key_Trig4 = 0;

                    if (!f_vj_on) {
                        f_2k_on = 1;
                    }

					if (1 == steph) {
						p_OutB_H1 = 1;
						p_OutA_H2 = 0;

						f_H1_on = 1;
						f_H2_on = 0;

						steph = 2;
					} else if (2 == steph) {
						p_OutB_H1 = 0;
						p_OutA_H2 = 1;

						f_H1_on = 0;
						f_H2_on = 1;

						steph = 3;
					} else if (3 == steph) {
						p_OutB_H1 = 1;
						p_OutA_H2 = 1;

						f_H1_on = 1;
						f_H2_on = 1;

						steph = 4;
					} else if (4 == steph) {
						f_H1_on = 0;
						f_H2_on = 0;

						p_OutB_H1 = 0;
						p_OutA_H2 = 0;

						steph = 1;
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
                    // Enable Timer2 PWM to 2KHz
                    tm2ct = 0x0;
                    tm2b = 0b0111_1100;// 124

                    tm2s = 0b000_00111;// 7

                    tm2c = 0b0001_1000;// CLK(=IHRC/2) | PA3 | Period | Disable Inverse
                }

                cnt_3s_time_2k++;
                // ring 120ms
                if (10 == cnt_3s_time_2k) {
                    f_2k_on = 0;
                    cnt_3s_time_2k = 0;

					// Disable Timer2 PWM
					tm2c = 0b0000_0000;// IHRC | PB2 | Period | Disable Inverse
                    p_OutA_2K    =    0;
                }
            }

            break;
        }
    }
}
