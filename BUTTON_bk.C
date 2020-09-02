#include	"extern.h"

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
	.ADJUST_IC	SYSCLK=IHRC/4		//	SYSCLK=IHRC/4
    
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
    
//	$ T16M		IHRC, /1, BIT8;				// 32us
	$ T16M		IHRC, /1, BIT11;			// 256us
//	$ T16M		IHRC, /1, BIT10;			// 128us
	$ TM2C		IHRC, Disable, Period, Inverse;

	$ INTEN		TM2;

	BYTE	Key_Flag;
	Key_Flag			=	_FIELD(p_InA_OD, p_InA_VJ, p_InA_V, p_InB_H, p_InA_QV2, p_InB_QV3);
	
	BYTE	Sys_FlagA	=	0;
	BIT		t16_10ms	:	Sys_FlagA.1;
	BIT		f_Key_Trig3	:	Sys_FlagA.3;
	BIT		f_Key_Trig4	:	Sys_FlagA.4;
    BIT		f_IN_QV2	:	Sys_FlagA.5;
    BIT		f_IN_QV3	:	Sys_FlagA.6;
    BIT		t_128us  	:	Sys_FlagA.7;    	

	BYTE	count_128us = 1;
	BYTE	count_1ms = 1;
	BYTE	count_10ms = 1;
	BYTE	count_1s = 1;
	BYTE	count_l = 0;
	
	// TM16 Period = 256us -> produce 20us Interrupt
	// 65536 - 65024 =  512
	// 512 / 65536 = 1/128
	// 2us / 256us = 1/128
	WORD	count	=	65023;
	stt16	count;
	
	// Enable 50KHz to be the basement to timing control 100KHz
	// tm2_enable_50K();

	while (1)
	{
		if (INTRQ.T16) {// TM16 Interrupt 50KHz (=20us)
			INTRQ.T16		=	0;
			stt16	count;

			if (--count_1ms == 0) {
				count_1ms		=	100;				// 10us * 100 = 1 ms

				if (--count_10ms == 0) {
					count_10ms		=	10;				// 1ms * 10 = 10 ms
					t16_10ms	=	1;
				}
			}
			
			if (t16_10ms) {
				t16_10ms	=	0;
				
				if (--count_1s == 0) {
					count_1s		=	100;				// 10ms * 100 = 1 s
					count_l++;
					
					if (25 == count_l) {
						count_l = 0;
					}
					
					if (0 == count_l) {
						p_OutB_LED = 1;						
						p_OutB_V1 = 1;
					} else if (5 == count_l) {
						p_OutB_LED = 0;
						p_OutB_V2 = 1;
					} else if (10 == count_l) {
						p_OutB_LED = 1;
						p_OutA_V3 = 1;
					} else if (15 == count_l) {
						p_OutB_LED = 0;
						p_OutB_H1 = 1;
					} else if (20 == count_l) {
						p_OutB_LED = 1;
						p_OutB_V1 = 0;
						p_OutB_V2 = 0;
						p_OutA_V3 = 0;
						p_OutB_H1 = 0;
					}
					
				}
			}
		}
	}
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
