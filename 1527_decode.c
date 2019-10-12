// LCk     = 90us or 102us
// 4*LCK   = 360us or 408us
// 12*LCK  = 1080us or 1224us
// 124*LCK = 11160us or 12648us

void interrupt ISR(void)// Period = 100us
{
    if (!RC5) {// 检测到低电平,低电平时间加1,记录本次电平状态
		always_low_cnt++;

		// 剔除长时间为低,导致计数溢出,却也能符合if (!last_level)中的逻辑判断的情形
		if (always_low_cnt >= 141) {// 非法序列,状态清零, always_low_cnt >= 14000us is invalid(because always_high_cnt max = 124*LCK = 12648us)
			always_low_cnt=0;
			always_high_cnt=0;
			bit_cnt=0; sync_ok=0; tmp_byte1=0; tmp_byte2=0; tmp_byte3=0; tmp_byte4=0;
		}

		last_level=0;
	} else {
		if (!last_level) {// 检测到从低到高的跳变时(=新周期伊始),就去解析上一个周期的电平状态
			// always_high_cnt=1->2->3, 3-1=2: [200us,300us)
			// always_high_cnt=1->2->3->4->5, 5-1=4: [400us,500us)
			// always_high_cnt=[3,5] = [200us,500us)
			if (((always_high_cnt>=3)&&(always_high_cnt<=5))&&((always_low_cnt>=110)&&(always_low_cnt<=131))) {// 上一个周期序列为同步码, always_low_cnt=[110,131] = [10900us,13000us)
				bit_cnt=0; sync_ok=1; tmp_byte1=0; tmp_byte2=0; tmp_byte3=0; tmp_byte4=0;
		    } else if ((sync_ok)&&((always_low_cnt>=10)&&(always_low_cnt<=15))) {// 上一个周期序列为低电平, always_low_cnt=[10,15] = [900us,1400us)
				if ((always_high_cnt<3) || (always_high_cnt>5)) {// 非法序列,状态清零
					always_low_cnt=0;
					always_high_cnt=0;
					bit_cnt=0; sync_ok=0; tmp_byte1=0; tmp_byte2=0; tmp_byte3=0; tmp_byte4=0;
				} else {
					if (23 == bit_cnt) {
						ev1527_byte1=tmp_byte1;ev1527_byte2=tmp_byte2;// 将接收到的编码复制到解码寄存器中
						ev1527_byte3=tmp_byte3;ev1527_byte4=tmp_byte4;// 将接收到的编码复制到解码寄存器中
						ev1527_ok=1;                                  // 通知解码子程序可以解码了
					}

					bit_cnt++;    // BIT位数++
					if (bit_cnt > 24) {// 非法序列,状态清零,防止因为溢出导致的意外
						always_low_cnt=0;
						always_high_cnt=0;
						bit_cnt=0; sync_ok=0; tmp_byte1=0; tmp_byte2=0; tmp_byte3=0; tmp_byte4=0;
					}
				}
		    } else if ((sync_ok)&&((always_low_cnt>=3)&&(always_low_cnt<=5))) {// 上一个周期序列为高电平, // always_low_cnt=[3,5] = [200us,500us)
				if ((always_high_cnt<10) || (always_high_cnt>15)) {// 非法序列,状态清零
					always_low_cnt=0;
					always_high_cnt=0;
					bit_cnt=0; sync_ok=0; tmp_byte1=0; tmp_byte2=0; tmp_byte3=0; tmp_byte4=0;
				} else {
					switch (bit_cnt) {
						case 0 : { tmp_byte1=tmp_byte1 | 0B10000000; break; }//遥控编码第1位
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
						case 20: { tmp_byte4=tmp_byte4 | 0B10000000; break; }//按键状态第1位
						case 21: { tmp_byte4=tmp_byte4 | 0B01000000; break; }
						case 22: { tmp_byte4=tmp_byte4 | 0B00100000; break; }
						case 23: {
								   tmp_byte4=tmp_byte4 | 0B00010000; 
								   ev1527_byte1=tmp_byte1;ev1527_byte2=tmp_byte2;// 将接收到的编码复制到解码寄存器中
								   ev1527_byte3=tmp_byte3;ev1527_byte4=tmp_byte4;// 将接收到的编码复制到解码寄存器中
								   ev1527_ok=1;                                  // 通知解码子程序可以解码了
								   break; 
						}
						default: { break; }
					} 

					bit_cnt++;// BIT位数++
					if (bit_cnt > 24) {// 非法序列,状态清零,防止因为溢出导致的意外
						bit_cnt=0; sync_ok=0; tmp_byte1=0; tmp_byte2=0; tmp_byte3=0; tmp_byte4=0;
					}
				}
            } else {// 上一个周期序列为非法序列,状态清零
				bit_cnt=0; sync_ok=0; tmp_byte1=0; tmp_byte2=0; tmp_byte3=0; tmp_byte4=0;
			}

			always_low_cnt=0;
			always_high_cnt=0;
		} else {
			// 如果上一次也是高电平,只需要安静计数即可
		}

		// 剔除长时间为高,导致计数溢出,却也能符合if (!last_level)中的逻辑判断的情形
		if (always_high_cnt >= 16) {// 非法序列,状态清零, always_high_cnt >= 1500us is invalid(because always_high_cnt max = 12*LCK = 1224us)
			always_low_cnt=0;
			always_high_cnt=0;
			bit_cnt=0; sync_ok=0; tmp_byte1=0; tmp_byte2=0; tmp_byte3=0; tmp_byte4=0;
		}

		always_high_cnt++;

        last_level=1;// 记录本次电平状态
    }
}