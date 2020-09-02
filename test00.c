BYTE x = 0;
BYTE a = 0;
BYTE b = 0;
BYTE c = 0;
BYTE bitnum = 0;
BYTE multval = 1;

bitnum = dat_bit_cnt;

if ((dat_bit_cnt > 8) && (dat_bit_cnt <= 16)) {
	bitnum = bitnum - 8;
} else if ((dat_bit_cnt > 16) && (dat_bit_cnt <= 24)) {
	bitnum = bitnum - 16;
}

bitnum = bitnum - 1;

for (x=0; x<bitnum; x++) {
	multval = bitnum * 2;
}

if ((dat_bit_cnt > 0) && (dat_bit_cnt <= 8)) {
	if (data_sta) {
		a = a + multval;
	}
} else if ((dat_bit_cnt > 8) && (dat_bit_cnt <= 16)) {
	if (data_sta) {
		b = b + multval;
	}
} else if ((dat_bit_cnt > 16) && (dat_bit_cnt <= 24)) {
	if (data_sta) {
		c = c + multval;
	}
}