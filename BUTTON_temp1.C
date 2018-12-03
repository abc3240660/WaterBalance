#include	"extern.h"

//#define USE_5K 1
#define USE_10K 1

void pwmgx_enable_2K(void);
void pwmg2_enable_2K_50(void);
void pwmg2_disable(void);
void tm2_enable_50K();

BIT		p_InA_OD	:	PA.4;
BIT		p_InA_VJ	:	PA.5;
BIT		p_InA_V	    :	PA.7;
BIT		p_InB_H	    :	PB.1;
BIT		p_InA_QV2	:	PA.0;
BIT		p_InB_QV3	:	PB.0;

BIT		p_OutA_2K	:	PA.3;
BIT		p_OutB_V1   :	PB.2;
BIT		p_OutB_V2   :	PB.5;
BIT		p_OutA_V3   :	PA.6;
BIT		p_OutB_H1   :	PB.6;

// High is active
BIT		p_OutB_LED	:	PB.7;// LED

void	FPPA0 (void)
{
	.ADJUST_IC	SYSCLK=IHRC/1		//	SYSCLK=IHRC/4
    
    $ EOSCR		DIS_LVD_BANDGAP;

	$	p_OutA_2K		    Out, Low;
	$	p_OutB_V1		    Out, Low;
	$	p_OutB_V2		    Out, Low;
    $	p_OutA_V3		    Out, Low;
	$	p_OutB_H1		    Out, Low;
    $	p_OutB_LED		    Out, Low;// off

	$	p_InA_OD		    In;
	$	p_InA_VJ		    In;
	$	p_InA_V		        In;
	$	p_InB_H		        In;
    $	p_InA_QV2		    In;
    $	p_InB_QV3		    In;

	// IN Pull-UP
    PAPH		=		_FIELD(p_InA_OD, p_InA_V);
    PBPH		=		_FIELD(p_InB_H);
    
	$ T16M		IHRC, /4, BIT8;
//	$ T16M		IHRC, /1, BIT8;				// 32us
//	$ T16M		IHRC, /1, BIT11;			// 256us
//	$ T16M		IHRC, /1, BIT10;			// 128us
	$ TM2C		IHRC, Disable, Period, Inverse;

	BYTE	Key_Flag;
	Key_Flag			=	_FIELD(p_InA_OD, p_InA_VJ, p_InA_V, p_InB_H, p_InA_QV2, p_InB_QV3);

	BYTE	Sys_Flag	=	0;
	BIT		f_mode2				:	Sys_Flag.0;
	BIT		f_od_switch_on		:	Sys_Flag.1;// OD switch status
	BIT		f_Trig_lpress_OD	:	Sys_Flag.2;// OD long press
	BIT		f_Trig_spress_OD	:	Sys_Flag.3;// OD short press
	
	BYTE	Sys_FlagA	=	0;
	BIT		t16_10ms	:	Sys_FlagA.1;
	BIT		f_Key_Trig3	:	Sys_FlagA.3;
	BIT		f_Key_Trig4	:	Sys_FlagA.4;
    BIT		f_IN_QV2	:	Sys_FlagA.5;
    BIT		f_IN_QV3	:	Sys_FlagA.6;
    BIT		t_128us  	:	Sys_FlagA.7;    	

	BYTE	Sys_FlagB	=	0;
	BIT		f_2k_on			:	Sys_FlagB.1;
	BIT		f_rst_all_flag	:	Sys_FlagB.2;
	BIT		f_led_state		:	Sys_FlagB.3;
	BIT		f_vj_low2high	:	Sys_FlagB.5;
	
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

	BYTE	count_128us = 1;
	BYTE	count_1ms = 1;
	BYTE	count_10ms = 1;
	BYTE	count_l = 0;
//	BYTE    last_vj_state = 8;
	BYTE    last_vj_state = 1;

	// for long press
	BYTE	debounce_time_lpress_OD		=	250;				// Key debounce time = 250 ms
	BYTE	debounce_time_lpress_V		=	250;				// Key debounce time = 250 mS
	BYTE	debounce_time_lpress_H		=	250;				// Key debounce time = 250 mS

	BYTE	cnt_3s_time_4 	= 0;// CN1/H
	BYTE	cnt_3s_time_2k 	= 0;// 2KHz
	BYTE	cnt_3s_time_led = 19;// 1Hz

	BYTE	cnt_3s_time_startup 	= 0;
    BYTE	stepx = 0;
    BYTE	start = 0;
    BYTE	mid_val = 6;
    BYTE	end_val = 10;
    BYTE	end_val_cmp = 12;
	
	// TM16 Period = 256us -> produce 50us Interrupt
	// 65536 - 52736 =  12800
	// 12800 / 65536 = 25/128
	// 50us / 256us = 25/128
	//WORD	count	=	52735;
	
	WORD	count	=	56;
	stt16	count;
	
	f_mode2 = 0;
	f_2k_on = 0;

#ifdef USE_10K 
	mid_val = 6;
	end_val = 10;
#endif

#ifdef USE_5K 
	mid_val = 11;
	end_val = 20;
#endif

	end_val_cmp = end_val + 2;

	// Enable 50KHz to be the basement to timing control 100KHz
	// tm2_enable_50K();

	while (1)
	{
		if (INTRQ.T16) {// TM16 Interrupt 20KHz (=50us)
			INTRQ.T16		=	0;
			stt16	count;
			
			if (0 == count_l) {
				count_l = 1;
			} else {
				count_l = 0;
			}
			
			// time period 20KHz(=50us)
			if (1 == count_l) {
				p_OutB_H1 = 0;
			} else {
				p_OutB_H1 = 1;
			}
		}
	}
}

// PWMG0/1/2 Share the same Freq but different duty ratio
// Setting PWM's Freq to 2KHz
// Fpwm_freq = Fpwm_clk / (CB + 1) = 4M/8/250 = 2KHz
// [6:4]: 000-1,  001-2,  010-4,  011-8,  
//        100-16, 101-32, 110-64, 111-128
void pwmgx_enable_2K(void)
{
	pwmgcubl = 0b0100_0000;
	pwmgcubh = 0b0011_1110;// CB = {pwmgcubh[7:0], pwmgcubl[7:6]} = 249
	
	pwmgclk = 0b1011_0000;// Fpwm_clk = = SYSCLK / 8
}

// Enable PWMG2 Output with 50% duty ratio at 2kHz
void pwmg2_enable_2K_50(void)
{
	pwmg2dtl = 0b0010_0000;// DB0 = pwmg0dtl[5] = 1
	pwmg2dth = 0b0001_1111;// DB10_1 = {pwmg0dth[7:0], pwmg0dtl[7:6]}

	// Fpwm_duty = [DB10_1 + DB0*0.5 + 0.5] / (CB + 1) = (DB10_1 + 1) / 250
	pwmg2c = 0b0000_0110;// PA3 PWM
}

// Disable PWMG2
void pwmg2_disable(void)
{
	pwmg2c = 0b0000_0000;// disable PWMG2
	pwmgclk = 0b0011_0000;// disable PWMGX
}

// Enable Timer2 PWM to 50KHz -> Interrupt = 100KHz(one period have 2 interrupt)
// freq = Fsrc / 2 / (K+1) / S1 / (S2+1) = 4M / 2 / 1 / 4 / 10 = 50KHz
void tm2_enable_50K()
{
	tm2ct = 0x0;
	tm2b = 0b0000_0000;// K
    tm2s = 0b0_01_01001;// S1=^-^[6:5]=(01->4), S2=[4:0]=9
	tm2c = 0b0001_0000;// CLK(=IHRC/4) | Disable Output | Period | Disable Inverse
}
