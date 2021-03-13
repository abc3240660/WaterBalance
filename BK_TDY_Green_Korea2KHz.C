#include    "extern.h"

//#define USE_20K 1
#define USE_10K 1
#define GREEN_PWM 1

BIT     p_InA_VJ    :   PA.5;// PB.5 -> PA.5
BIT     p_InB_H     :   PB.7;// PB.2 -> PB.5
BIT     p_InB_OD    :   PB.6;// PB.1 -> PB.6
BIT     p_InB_V     :   PB.5;// PA.7 -> PB.7
BIT     p_InA_RF    :   PA.0;// PA.6 -> PA.0

BIT     p_OutA_2K   :   PA.3;// PB.6 -> PA.3
BIT     p_OutA_V1   :   PA.7;// PA.0 -> PA.7
BIT     p_OutA_V2   :   PA.4;// OK
BIT     p_OutA_H1   :   PA.6;// PB.0 -> PA.6
BIT     p_OutB_H2   :   PB.0;// PA.3 -> PB0.3

void FPPA0 (void)
{
    .ADJUST_IC    SYSCLK=IHRC/4        //    SYSCLK=IHRC/4

    $    EOSCR        DIS_LVD_BANDGAP;

    $    p_OutA_2K            Out, Low;
    $    p_OutA_V1            Out, Low;
    $    p_OutA_V2            Out, Low;
    $    p_OutA_H1            Out, Low;
    $    p_OutB_H2            Out, Low;

    $    p_InB_H              In;
    $    p_InA_VJ             In;
    $    p_InB_OD             In;
    $    p_InB_V              In;

    // IN Pull-UP
    PAPH        =        _FIELD(p_InA_RF);
    PBPH        =        _FIELD(p_InB_H, p_InB_V, p_InB_OD);

#ifdef USE_10K
    $ T16M        IHRC, /4, BIT9;                // 256us
#endif

#ifdef USE_20K
    $ T16M        IHRC, /4, BIT8;                // 128us
#endif

    $ TM2C        IHRC, Disable, Period, Inverse;

    BYTE    Key_Flag;
    Key_Flag            =    _FIELD(p_InB_H, p_InA_VJ, p_InB_OD, p_InB_V);

    BYTE    Sys_Flag    =    0;
    BIT        f_Key_Trig1      :    Sys_Flag.0;
    BIT        t16_10ms         :    Sys_Flag.1;
    BIT        t16_10ms_rmt     :    Sys_Flag.2;
    BIT        f_Key_Trig3      :    Sys_Flag.3;
    BIT        f_Key_Trig4      :    Sys_Flag.4;
	BIT        t16_1ms          :    Sys_Flag.5;
	BIT        f_StartCount     :    Sys_Flag.6;
	BIT        f_Addr_Saved     :    Sys_Flag.7;

    BYTE    Sys_FlagB    =    0;
	BIT        f_Duty_Switch    :    Sys_FlagB.0;
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

    BYTE    Sys_FlagD    =    0;
	BIT        f_839KHz_mode    :    Sys_FlagD.5;
	
//    pmode    Program_Mode;
//    fppen    =    0xFF;

	BYTE    count0 = 1;
    BYTE    count1 = 1;
	BYTE    count2 = 0;
	BYTE    count_x = 1;
    BYTE    count_l = 0;
    BYTE    count_h = 0;
    BYTE    last_vj_state = 8;

    BYTE    cnt_Key_10ms_1    =    175;              //    Key debounce time = 40 mS

    BYTE    cnt_Key_10ms_3    =    175;                //    Key debounce time = 40 mS
    BYTE    cnt_Key_10ms_4    =    175;                //    Key debounce time = 40 mS

    BYTE    cnt_3s_time_1       = 0;//
    BYTE    cnt_3s_time_2k      = 0;// 2KHz
    BYTE    cnt_3s_time_led     = 0;// 1Hz
    BYTE    flash_time_laser    = 40;
    BYTE    cnt_3s_time_startup = 0;//

    BYTE    stepx = 0;
    BYTE    stepv = 0;
    BYTE    steph = 0;
    BYTE    start = 0;

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

    BYTE    h_rm_long_cnt = 0;// remote OD long press
    BYTE    h_rm_rels_cnt = 20;// remote OD release

	BYTE    ir_disable_cnt = 0;

	BYTE	mid_val = 50;// + Max: 45 ---> V1/V3 MaxTimePeriod: 0%-45%, H1/V2 MaxTimePeriod: 50%-95%
	BYTE	duty_mode = 40;// Default
	BYTE	safe_duty = 40;

	BYTE    val1 = duty_mode;
	BYTE    val2 = mid_val + duty_mode;

	BYTE    count_2s = 1;
	BYTE    count_10ms = 1;
	
#ifdef USE_10K
    WORD    count    =    112;
#endif
#ifdef USE_20K
    WORD    count    =    64;
#endif

    f_2k_on = 0;

    while (1) {
        if (INTRQ.T16) {// = 112->100us or 432->20us
            INTRQ.T16        =    0;
            stt16    count;

#if 0// 实测25us
			if (0 == count2) {
				count2 = 1;
				p_OutB_H2 = 1;
			} else {
				count2 = 0;
				p_OutB_H2 = 0;
			}
#endif

			if (!f_839KHz_mode) {// normal mode
				count0 = 4;
			} else {// special mode
				if (--count0 == 0) {// 20us * 5 = 100 us
					count0 = 4;
				}
			}

			if (4 == count0) {
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
			}

            if (flash_time_laser >= 20) {
				if (f_839KHz_mode) {// special mode
					count_x--;

					if (0 == count_x) {
						count_x = 20;// 实测96us，5.2KHz
					}
				}

                count_l++;

                if (100 == count_l) {
                    count_l = 0;
                    count_h++;
                    if (100 == count_h)
                        count_h = 0;
                }
            } else {
				count_x = 1;
			}

			if (4 == count0) {
				if (1 == start) {
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

						if (2 == ev1527_byte4) {// C -> OD
							if (!f_M_disable) {// period = 200ms
								f_M_disable = 1;

								// short press
								// idle too long(250ms), so not long press
								// one whole eve1527 period = 45ms~52ms
								if (od_rm_rels_cnt > 15) {// 250ms = 4~5 whole ev1527 period									
									if (40 == duty_mode) {
										duty_mode = 30;
									} else if (30 == duty_mode) {
										duty_mode = 20;
									} else if (20 == duty_mode) {
										duty_mode = 10;
									} else if (10 == duty_mode) {
										duty_mode = 40;
//									} else if ((duty_mode>40) && (duty_mode<=45)) {
//										duty_mode = 40;
									}
		
//									if (duty_mode > safe_duty) {
//										count_2s = 0;
//										count_10ms = 0;
//										f_StartCount = 1;
//									}
		
									f_Duty_Switch = 1;
		
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
										if (40 == duty_mode) {
											duty_mode = 10;
										} else if (30 == duty_mode) {
											duty_mode = 40;
										} else if (20 == duty_mode) {
											duty_mode = 30;
										} else if (10 == duty_mode) {
											duty_mode = 20;
//										} else if ((duty_mode>=40) && (duty_mode<45)) {
//											duty_mode = 45;
										}
		
//										if (duty_mode > safe_duty) {
//											count_2s = 0;
//											count_10ms = 0;
//											f_StartCount = 1;
//										}
		
										f_Duty_Switch = 1;
		
										f_Key_Trig1    =    1;                //    so Trigger, when stable at 3000 mS.
									}
								}
							}
							
							od_rm_rels_cnt = 0;
						} else if (4 == ev1527_byte4) {// B -> V
							if (!f_V_disable) {
								if (!f_vj_on) {
									f_2k_on = 1;
								}

								f_V_disable = 1;
								f_Key_Trig3 = 1;
							}
						} else if (8 == ev1527_byte4) {// A -> H
							if (!f_H_disable) {
								f_H_disable = 1;

								// short press
								// idle too long(250ms), so not long press
								// one whole eve1527 period = 45ms~52ms
								if (h_rm_rels_cnt > 10) {// 250ms = 4~5 whole ev1527 period									
									f_Key_Trig4 = 1;
									h_rm_long_cnt = 1;
								} else {
									if (h_rm_long_cnt < 200) {
										h_rm_long_cnt++;
									}

									// long press
									if (8 == h_rm_long_cnt) {// 1.6s
										if (f_839KHz_mode) {
											count_l = 0;
											count_h = 0;
											count = 112;
											f_839KHz_mode = 0;
										} else {
											count = 432;
											f_839KHz_mode = 1;
										}

										f_Key_Trig4 = 1;
									}
								}
							}
							
							h_rm_rels_cnt = 0;
						} else if (1 == ev1527_byte4) {// D -> X
							if (!f_D_disable) {
								if (!f_vj_on) {
									f_2k_on = 1;
								}

								f_D_disable = 1;

								if (40 == duty_mode) {
									duty_mode = 30;
								} else if (30 == duty_mode) {
									duty_mode = 20;
								} else if (20 == duty_mode) {
									duty_mode = 10;
								} else if (10 == duty_mode) {
									duty_mode = 40;
//								} else if ((duty_mode>50) && (duty_mode<=60)) {
//									duty_mode = 50;
								}
		
//								if (duty_mode > safe_duty) {
//									count_2s = 0;
//									count_10ms = 0;
//									f_StartCount = 1;
//								}
		
								f_Duty_Switch = 1;
							}
						}
					} else {
						if (t16_10ms_rmt) {// period = 10ms
							t16_10ms_rmt = 0;

							if (od_rm_rels_cnt < 200) {// 2s
								od_rm_rels_cnt++;
							}
							
							if (h_rm_rels_cnt < 200) {// 2s
								h_rm_rels_cnt++;
							}
						}
					}
				}
			}
        }

        // if vj mode, laser ON 30ms then OFF 220ms
        if (flash_time_laser >= 20) {
			if (!f_839KHz_mode) {// normal mode
				if ((0 == count_l)&&(0 == count_h)) {
					if (f_V1_on) {
						p_OutA_V1 = 1;
					}
					if (f_H2_on) {
						p_OutB_H2 = 1;
					}
				} else if ((val1 == count_l)&&(0 == count_h)) {
					p_OutA_V1 = 0;
					p_OutB_H2 = 0;
				} else if ((mid_val == count_l)&&(0 == count_h)) {
					if (f_V2_on) {
						p_OutA_V2 = 1;
					}
					if (f_H1_on) {
						p_OutA_H1 = 1;
					}
				} else if ((val2 == count_l)&&(0 == count_h)) {
					p_OutA_V2 = 0;
					p_OutA_H1 = 0;
				} else if ((0 == count_l)&&(1 == count_h)) {
					count_l = 0;
					count_h = 0;
				}
			} else {// special mode
				if (20 == count_x) {
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
						p_OutA_V2 = 1;
					}
				} else if (12 == count_x) {
					p_OutA_H1 = 0;
					p_OutB_H2 = 0;
					p_OutA_V1 = 0;
					p_OutA_V2 = 0;
				}
			}
        } else {
            if (19 == flash_time_laser) {
                p_OutA_H1 = 0;
                p_OutB_H2 = 0;
                p_OutA_V1 = 0;
                p_OutA_V2 = 0;
            }
        }

        while (t16_10ms)
        {
            t16_10ms    =    0;

#if 0
			if (f_StartCount) {
				count_10ms++;
				if (200 == count_10ms) {// 10ms * 200 = 2s
					count_10ms = 0;
					count_2s++;

					// if (50 == count_2s) {// 2s * 50 = 100s = 1.6min
					if (250 == count_2s) {// 2s * 250 = 500s = 8.3min
						count_2s = 0;
						
						if (duty_mode > safe_duty) {
							duty_mode--;
							f_Duty_Switch = 1;
						} else {
							f_StartCount = 0;
						}
					}
				}
			}
#endif
            if (f_M_disable) {
                m_disable_cnt++;
                
                if (20 == m_disable_cnt) {// 200ms debounce
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

            if (cnt_3s_time_startup < 126) {
                cnt_3s_time_startup++;
            }

            if (5 == cnt_3s_time_startup) {
                p_OutA_H1    =    1;

                f_V1_on = 0;
                f_V2_on = 0;
                f_H1_on = 1;
                f_H2_on = 0;
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
                    p_OutB_H2 = 1;

                    f_H2_on = 1;

                    stepx = 4;
                    f_2k_on = 1;
                }
            } else if (125 == cnt_3s_time_startup) {
                if ((4 == stepx) && (0 == start)) {
                    f_V1_on = 0;
                    f_V2_on = 0;
                    f_H2_on = 0;

                    p_OutA_V1 = 0;
                    p_OutA_V2 = 0;
                    p_OutB_H2 = 0;

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
                A    =    (PB ^ Key_flag) & _FIELD(p_InB_OD);    //    only check the bit of p_Key_In.
                if (! ZF)
                {                                        //    if is not same,
                    // Active: H->L
                    if (!p_InB_OD) {
                        if (cnt_Key_10ms_1 > 0) {
                            if (--cnt_Key_10ms_1 == 0)
                            {                                    //    and over debounce time.
                                f_Key_Trig1    =    1;                //    so Trigger, when stable at 3000 mS.
                            }

                            if (cnt_Key_10ms_1 == 170) {
                                if (!f_vj_on) {
                                    f_2k_on = 1;
                                }
                            }
                        }
                    } else {// Up: H->L
                        Key_flag    ^=    _FIELD(p_InB_OD);
                    }
                } else {
                    if (cnt_Key_10ms_1 <= 170) {
                        if (cnt_Key_10ms_1 != 0) {// Only ShortPress
							if (40 == duty_mode) {
								duty_mode = 30;
							} else if (30 == duty_mode) {
								duty_mode = 20;
							} else if (20 == duty_mode) {
								duty_mode = 10;
							} else if (10 == duty_mode) {
								duty_mode = 40;
//							} else if ((duty_mode>40) && (duty_mode<=45)) {
//								duty_mode = 40;
							}

//							if (duty_mode > safe_duty) {
//								count_2s = 0;
//								count_10ms = 0;
//								f_StartCount = 1;
//							}

							f_Duty_Switch = 1;
                        }
                    }

                    cnt_Key_10ms_1    =    175;
                }

                A    =    (PB ^ Key_Flag) & _FIELD(p_InB_V);    //    only check the bit of p_Key_In.
                if (! ZF)
                {                                        //    if is not same,
                    // Active: H->L
                    if (!p_InB_V) {
                        if (cnt_Key_10ms_3 > 0) {
                            if (--cnt_Key_10ms_3 == 0) {
                                // do not support long press
                            }
                        }
                    } else {// Up: H->L
                        Key_flag    ^=    _FIELD(p_InB_V);
                    }
                } else {
					if (cnt_Key_10ms_3 <= 170) {
                        if (cnt_Key_10ms_3 != 0) {// Only ShortPress
							f_Key_Trig3 = 1;
						}
					}

                    cnt_Key_10ms_3 = 175;
                }

                A    =    (PB ^ Key_flag) & _FIELD(p_InB_H);    //    only check the bit of p_Key_In.
                if (! ZF)
                {                                        //    if is not same,
                    // Active: H->L
                    if (!p_InB_H) {
                        if (cnt_Key_10ms_4 > 0) {
                            if (--cnt_Key_10ms_4 == 0) {
                                if (f_839KHz_mode) {
									count_l = 0;
									count_h = 0;
									count = 112;
									f_839KHz_mode = 0;
								} else {
									count = 432;
									f_839KHz_mode = 1;
								}
								
								if (!f_vj_on) {
									f_2k_on = 1;
								}
                            }
                        }
                    } else {// Up: H->L
                        Key_flag    ^=    _FIELD(p_InB_H);
                    }
                } else {
					if (cnt_Key_10ms_4 <= 170) {
                        if (cnt_Key_10ms_4 != 0) {// Only ShortPress
							f_Key_Trig4 = 1;
						}
					}

                    cnt_Key_10ms_4 = 175;
                }

				if (f_Duty_Switch) {
					f_Duty_Switch = 0;
					
					val1 = duty_mode;
					val2 = mid_val + duty_mode;

					count_l = 0;
					count_h = 0;
					flash_time_laser = 40;
				}

                if (f_Key_Trig1)
                {
                    f_Key_Trig1 = 0;

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

                // Normal Mode
                if (0 == cnt_3s_time_1) {
                    // Down: L->H
                    if (p_InA_VJ) {// special mode
                        if (last_vj_state != 1) {
                            last_vj_state = 1;

                            f_vj_on = 1;
                            f_led_flash = 1;

                            // Enable PWMG1C to 2KHz
                            pwmgcubl = 0b0000_0000;
                            pwmgcubh = 0b0001_1111;

                            pwmg2dtl = 0b1000_0000;
                            pwmg2dth = 0b0000_1111;

                            pwmg2c = 0b0000_0010;// PB6 PWM
                            pwmgclk = 0b1100_0000;// enable PWMG CLK(=SYSCLK/16)
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
                            pwmg2c = 0b0000_0000;// do not output PWM


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
                        p_OutA_V1 = 1;
                        p_OutA_V2 = 0;

                        f_V1_on = 1;
                        f_V2_on = 0;

                        stepv= 2;
                    } else if (2 == stepv) {
                        p_OutA_V1 = 0;
                        p_OutA_V2 = 1;

                        f_V1_on = 0;
                        f_V2_on = 1;
                        stepv = 3;
                    } else if (3 == stepv) {
                        p_OutA_V1 = 1;
                        p_OutA_V2 = 1;

                        f_V1_on = 1;
                        f_V2_on = 1;

                        stepv = 4;
                    } else if (4 == stepv) {
                        f_V1_on = 0;
                        f_V2_on = 0;

                        p_OutA_V1 = 0;
                        p_OutA_V2 = 0;

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
                        p_OutA_H1 = 1;
                        p_OutB_H2 = 0;

                        f_H1_on = 1;
                        f_H2_on = 0;

                        steph = 2;
                    } else if (2 == steph) {
                        p_OutA_H1 = 0;
                        p_OutB_H2 = 1;

                        f_H1_on = 0;
                        f_H2_on = 1;

                        steph = 3;
                    } else if (3 == steph) {
                        p_OutA_H1 = 1;
                        p_OutB_H2 = 1;

                        f_H1_on = 1;
                        f_H2_on = 1;

                        steph = 4;
                    } else if (4 == steph) {
                        f_H1_on = 0;
                        f_H2_on = 0;

                        p_OutA_H1 = 0;
                        p_OutB_H2 = 0;

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
                    // Enable PWMG0C to 2KHz
                    pwmgcubl = 0b0000_0000;
                    pwmgcubh = 0b0001_1111;

                    pwmg2dtl = 0b1000_0000;
                    pwmg2dth = 0b0000_1111;

                    pwmg2c = 0b0000_0110;// PB6 PWM
                    pwmgclk = 0b1100_0000;// enable PWMG CLK(=SYSCLK/16)
                }

                cnt_3s_time_2k++;
                // ring 120ms
                if (10 == cnt_3s_time_2k) {
                    f_2k_on = 0;
                    cnt_3s_time_2k = 0;

                    // Disable 2KHz
                    pwmg2c = 0b0000_0000;// do not output PWM
                    p_OutA_2K    =    0;
                }
            }

            break;
        }
    }
}