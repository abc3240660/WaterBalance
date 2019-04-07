#include	"extern.h"

BIT p_In3    :    PB.7;// Cmptor Input(0-Valid, 1-Invalid)

BIT p_Out6_V1    :    PA.6;// V1'
BIT p_Out1_V2    :    PB.5;// V2'
BIT p_Out2_H     :    PB.6;// H'

void	FPPA0 (void)
{
	.ADJUST_IC    SYSCLK = IHRC/4// SYSCLK = IHRC/4 = 4MHz

	$	p_Out6_V1		    Out, Low;
	$	p_Out1_V2		    Out, Low;
	$	p_Out2_H		    Out, Low;

	$	p_In3		    In;

//    PBPH		=		_FIELD(p_In3);

	$ T16M        IHRC, /1, BIT10;// 16MHz/1 = 16MHz:the time base of T16.
	$ TM2C		IHRC, Disable, Period, Inverse; 	

	BYTE	Sys_FlagB	=	0;
    BIT     f_InB_X_last    :   Sys_FlagB.6;
    BIT     f_sync_trig     :   Sys_FlagB.7;

    BYTE    sync_sta = 0;
    BYTE    data_sta = 0;
    BYTE    l_cnt_sync = 0;
    BYTE    h_cnt_sync = 0;
//    BYTE    l_cnt_dat = 0;
    BYTE    h_cnt_dat = 0;
	
	BYTE    dat_bit_cnt = 0;
    BYTE    l_low_cnt = 0;

	while (1)
	{
		if (INTRQ.T16) {// 128us
			INTRQ.T16		=	0;
			
            if (!p_In3) {// LOW
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
							p_Out2_H = 1;
						} else {
							p_Out2_H = 0;
						}
					} else if (22 == dat_bit_cnt) {
						if (data_sta) {
							p_Out6_V1 = 1;
						} else {
							p_Out6_V1 = 0;
						}
					} else if (23 == dat_bit_cnt) {
						if (data_sta) {
							p_Out1_V2 = 1;
						} else {
							p_Out1_V2 = 0;
						}
					} else if (24 == dat_bit_cnt) {
						if (data_sta) {
							p_Out6_V1 = 1;
						} else {
							p_Out6_V1 = 0;
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
		}
	}
}	