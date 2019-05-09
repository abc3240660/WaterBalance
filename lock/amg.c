#include <stdio.h>
#include <string.h>
#include "amg.h"

typedef unsigned char uint8_t;
typedef signed char int8_t;
typedef signed short int16_t;
typedef unsigned short uint16_t;
typedef unsigned long int uint32_t;
typedef signed long int int32_t;


// TODO: read till empty or timeout

int Uart3_Putc(uint8_t ch)
{
	return 0;
}

uint8_t Uart3_Getc()
{
	return 0;
}

void delay_ms(uint32_t time)
{
	
}

uint8_t send_buf[64] = "";

static int bno055_write_byte(uint8_t reg_addr, uint8_t reg_data)
{
	int i = 0;
	uint8_t onebyte = 0;
	
	memset(send_buf, 0, 64);
	
	send_buf[0] = 0xAA;
	send_buf[1] = 0x00;// WR
	send_buf[2] = reg_addr;
	send_buf[3] = 0x01;// LEN
	send_buf[4] = reg_data;
	
	// send data
	for (i=0; i<5; i++) {
		Uart3_Putc(send_buf[i]);
	}
	
	// wait
	delay_ms(100);
	
	onebyte = Uart3_Getc();
	
	if (0xEE == onebyte) {// NG ack response
		onebyte = Uart3_Getc();
		
		if (onebyte != 0x01) {// 0x01 - WRITE_SUCCESS
			printf("write failure code: 0x%X\r\n", onebyte);

			return -1;
		}
	} else {// invalid ack
		printf("receive invalid ack header: 0x%X\r\n", onebyte);

		return -1;
	}

	return 0;
}

static int bno055_write_bytes(uint8_t reg_addr, uint8_t *p_in, uint8_t len)
{
	int i = 0;
	uint8_t onebyte = 0;

	if ((!p_in) || (len<=0)) {
		return -1;
	}

	memset(send_buf, 0, 64);
	
	send_buf[0] = 0xAA;
	send_buf[1] = 0x00;// WR
	send_buf[2] = reg_addr;
	send_buf[3] = len;// LEN
	
	for (i=0; i<len; i++) {
		send_buf[4+i] = p_in[i];
	}

	// send data
	for (i=0; i<(len+4); i++) {
		Uart3_Putc(send_buf[i]);
	}
	
	// wait
	delay_ms(100);
	
	onebyte = Uart3_Getc();
	
	if (0xEE == onebyte) {// NG ack response
		onebyte = Uart3_Getc();
		
		if (onebyte != 0x01) {// 0x01 - WRITE_SUCCESS
			printf("write failure code: 0x%X\r\n", onebyte);

			return -1;
		}
	} else {// invalid ack
		printf("receive invalid ack header: 0x%X\r\n", onebyte);

		return -1;
	}
	
	return 0;
}

static uint8_t bno055_read_byte(uint8_t reg_addr)
{
	int i = 0;
	uint8_t onebyte = 0;

	memset(send_buf, 0, 64);

	send_buf[0] = 0xAA;
	send_buf[1] = 0x01;// RD
	send_buf[2] = reg_addr;
	send_buf[3] = 0x01;// LEN

	// send data
	for (i=0; i<4; i++) {
		Uart3_Putc(send_buf[i]);
	}

	// wait
	delay_ms(100);

	// recv ack till empty or timeout
	onebyte = Uart3_Getc();

	if (0xEE == onebyte) {// NG ack response
		onebyte = Uart3_Getc();
		printf("read failure code: 0x%X\r\n", onebyte);

		return -1;
	} else if (0xBB == onebyte) {// OK ack response
		onebyte = Uart3_Getc();// length
		
		if (onebyte != 0x01) {// length NG
			printf("receive data lenght is ng\r\n");
			return -1;
		}

		return Uart3_Getc();// data
	} else {// invalid ack
		printf("read invalid ack header: 0x%X\r\n", onebyte);

		return -1;
	}
}

static int bno055_read_bytes(uint8_t reg_addr, uint8_t len, uint8_t *p_out)
{
	int i = 0;
	uint8_t onebyte = 0;

	if ((!p_out) || (len<=0)) {
		return -1;
	}

	memset(send_buf, 0, 64);
	
	send_buf[0] = 0xAA;
	send_buf[1] = 0x01;// RD
	send_buf[2] = reg_addr;
	send_buf[3] = len;// LEN

	// send data
	for (i=0; i<4; i++) {
		Uart3_Putc(send_buf[i]);
	}

	// wait
	delay_ms(100);

	// recv ack till empty or timeout
	onebyte = Uart3_Getc();

	if (0xEE == onebyte) {// NG ack response
		onebyte = Uart3_Getc();
		printf("read failure code: 0x%X\r\n", onebyte);

		return -1;
	} else if (0xBB == onebyte) {// OK ack response
		onebyte = Uart3_Getc();// length
		
		if (onebyte != len) {// length NG
			printf("receive data lenght is ng\r\n");
			return -1;
		}
		
		for (i=0; i<onebyte; i++) {
			p_out[i] = Uart3_Getc();// length
		}
	} else {// invalid ack
		printf("read invalid ack header: 0x%X\r\n", onebyte);

		return -1;
	}
	
	return 0;
}

// to check chip id
int bno055_verify_chip()
{
	uint8_t onebyte = 0;
	
	onebyte = bno055_read_byte(BNO055_CHIP_ID);
	
	if (onebyte != 0xA0) {
		printf("verify BNO055_CHIP_ID failed\r\n");
		
		return -1;
	}
	
	onebyte = bno055_read_byte(BNO055_ACC_ID);
	
	if (onebyte != 0xFB) {
		printf("verify BNO055_ACC_ID failed\r\n");
		
		return -1;
	}
	
	onebyte = bno055_read_byte(BNO055_MAG_ID);
	
	if (onebyte != 0x32) {
		printf("verify BNO055_MAG_ID failed\r\n");
		
		return -1;
	}
	
	onebyte = bno055_read_byte(BNO055_GYRO_ID);
	
	if (onebyte != 0x0F) {
		printf("verify BNO055_GYRO_ID failed\r\n");
		
		return -1;
	}
	
	return 0;
}


// calibrate accelerometer & gyroscope
void bno055_calibrate_accel_gyro(float * dest1, float * dest2) 
{
	uint8_t data[6]; // data array to hold accelerometer and gyro x, y, z, data
	uint16_t ii = 0, sample_count = 0;
	int32_t gyro_bias[3]  = {0, 0, 0}, accel_bias[3] = {0, 0, 0};
	int16_t accel_temp[3] = {0, 0, 0};
	int16_t gyro_temp[3] = {0, 0, 0};
	
	printf("Accel/Gyro Calibration: Put device on a level surface and keep motionless! Wait......\r\n");
	delay_ms(400);
  
	// Select page 0 to read sensors
	bno055_write_byte(BNO055_PAGE_ID, 0x00);
	// Select BNO055 system operation mode as AMG for calibration
	bno055_write_byte(BNO055_OPR_MODE, CONFIGMODE );
	delay_ms(25);
	bno055_write_byte(BNO055_OPR_MODE, AMG);
   
	// In NDF fusion mode, accel full scale is at +/- 4g, ODR is 62.5 Hz, set it the same here
	bno055_write_byte(BNO055_ACC_CONFIG, NormalA << 5 | ABW_31_25Hz << 2 | AFS_4G );
	sample_count = 256;
	for(ii = 0; ii < sample_count; ii++) {
		accel_temp[0] = 0;
		accel_temp[1] = 0;
		accel_temp[2] = 0;
		bno055_read_bytes(BNO055_ACC_DATA_X_LSB, 6, &data[0]);  // Read the six raw data registers into data array
		accel_temp[0] = (int16_t) (((int16_t)data[1] << 8) | data[0]) ; // Form signed 16-bit integer for each sample in FIFO
		accel_temp[1] = (int16_t) (((int16_t)data[3] << 8) | data[2]) ;
		accel_temp[2] = (int16_t) (((int16_t)data[5] << 8) | data[4]) ;
		accel_bias[0]  += (int32_t) accel_temp[0];
		accel_bias[1]  += (int32_t) accel_temp[1];
		accel_bias[2]  += (int32_t) accel_temp[2];
		delay_ms(20);  // at 62.5 Hz ODR, new accel data is available every 16 ms
	}
    accel_bias[0]  /= (int32_t) sample_count;  // get average accel bias in mg
    accel_bias[1]  /= (int32_t) sample_count;
    accel_bias[2]  /= (int32_t) sample_count;
    
	if (accel_bias[2] > 0L) {
		accel_bias[2] -= (int32_t) 1000;
	} else { // Remove gravity from the z-axis accelerometer bias calculation
		accel_bias[2] += (int32_t) 1000;
	}

    dest1[0] = (float) accel_bias[0];  // save accel biases in mg for use in main program
    dest1[1] = (float) accel_bias[1];  // accel data is 1 LSB/mg
    dest1[2] = (float) accel_bias[2];          

	// In NDF fusion mode, gyro full scale is at +/- 2000 dps, ODR is 32 Hz
	bno055_write_byte(BNO055_GYRO_CONFIG_0, GBW_23Hz << 3 | GFS_2000DPS );
	bno055_write_byte(BNO055_GYRO_CONFIG_1, NormalG);
	for(ii = 0; ii < sample_count; ii++) {
		gyro_temp[0] = 0;
		gyro_temp[1] = 0;
		gyro_temp[2] = 0;
		bno055_read_bytes(BNO055_GYR_DATA_X_LSB, 6, &data[0]);  // Read the six raw data registers into data array
		gyro_temp[0] = (int16_t) (((int16_t)data[1] << 8) | data[0]) ;  // Form signed 16-bit integer for each sample in FIFO
		gyro_temp[1] = (int16_t) (((int16_t)data[3] << 8) | data[2]) ;
		gyro_temp[2] = (int16_t) (((int16_t)data[5] << 8) | data[4]) ;
		gyro_bias[0]  += (int32_t) gyro_temp[0];
		gyro_bias[1]  += (int32_t) gyro_temp[1];
		gyro_bias[2]  += (int32_t) gyro_temp[2];
		delay_ms(35);  // at 32 Hz ODR, new gyro data available every 31 ms
	}
    gyro_bias[0]  /= (int32_t) sample_count;  // get average gyro bias in counts
    gyro_bias[1]  /= (int32_t) sample_count;
    gyro_bias[2]  /= (int32_t) sample_count;
 
    dest2[0] = (float) gyro_bias[0]/16.;  // save gyro biases in dps for use in main program
    dest2[1] = (float) gyro_bias[1]/16.;  // gyro data is 16 LSB/dps
    dest2[2] = (float) gyro_bias[2]/16.;          

	// Return to config mode to write accelerometer biases in offset register
	// This offset register is only used while in fusion mode when accelerometer full-scale is +/- 4g
	bno055_write_byte(BNO055_OPR_MODE, CONFIGMODE );
	delay_ms(25);
  
	//write biases to accelerometer offset registers ad 16 LSB/dps
	bno055_write_byte(BNO055_ACC_OFFSET_X_LSB, (int16_t)accel_bias[0] & 0xFF);
	bno055_write_byte(BNO055_ACC_OFFSET_X_MSB, ((int16_t)accel_bias[0] >> 8) & 0xFF);
	bno055_write_byte(BNO055_ACC_OFFSET_Y_LSB, (int16_t)accel_bias[1] & 0xFF);
	bno055_write_byte(BNO055_ACC_OFFSET_Y_MSB, ((int16_t)accel_bias[1] >> 8) & 0xFF);
	bno055_write_byte(BNO055_ACC_OFFSET_Z_LSB, (int16_t)accel_bias[2] & 0xFF);
	bno055_write_byte(BNO055_ACC_OFFSET_Z_MSB, ((int16_t)accel_bias[2] >> 8) & 0xFF);
  
	// Check that offsets were properly written to offset registers
	// Serial.println("Average accelerometer bias = "); 
	// Serial.println((int16_t)((int16_t)readByte(BNO055_ADDRESS, BNO055_ACC_OFFSET_X_MSB) << 8 | readByte(BNO055_ADDRESS, BNO055_ACC_OFFSET_X_LSB))); 
	// Serial.println((int16_t)((int16_t)readByte(BNO055_ADDRESS, BNO055_ACC_OFFSET_Y_MSB) << 8 | readByte(BNO055_ADDRESS, BNO055_ACC_OFFSET_Y_LSB))); 
	// Serial.println((int16_t)((int16_t)readByte(BNO055_ADDRESS, BNO055_ACC_OFFSET_Z_MSB) << 8 | readByte(BNO055_ADDRESS, BNO055_ACC_OFFSET_Z_LSB)));

	//write biases to gyro offset registers
	bno055_write_byte(BNO055_GYR_OFFSET_X_LSB, (int16_t)gyro_bias[0] & 0xFF);
	bno055_write_byte(BNO055_GYR_OFFSET_X_MSB, ((int16_t)gyro_bias[0] >> 8) & 0xFF);
	bno055_write_byte(BNO055_GYR_OFFSET_Y_LSB, (int16_t)gyro_bias[1] & 0xFF);
	bno055_write_byte(BNO055_GYR_OFFSET_Y_MSB, ((int16_t)gyro_bias[1] >> 8) & 0xFF);
	bno055_write_byte(BNO055_GYR_OFFSET_Z_LSB, (int16_t)gyro_bias[2] & 0xFF);
	bno055_write_byte(BNO055_GYR_OFFSET_Z_MSB, ((int16_t)gyro_bias[2] >> 8) & 0xFF);
  
	// Select BNO055 system operation mode
	bno055_write_byte(BNO055_OPR_MODE, NDOF);

	// Check that offsets were properly written to offset registers
	// Serial.println("Average gyro bias = "); 
	// Serial.println((int16_t)((int16_t)readByte(BNO055_ADDRESS, BNO055_GYR_OFFSET_X_MSB) << 8 | readByte(BNO055_ADDRESS, BNO055_GYR_OFFSET_X_LSB))); 
	// Serial.println((int16_t)((int16_t)readByte(BNO055_ADDRESS, BNO055_GYR_OFFSET_Y_MSB) << 8 | readByte(BNO055_ADDRESS, BNO055_GYR_OFFSET_Y_LSB))); 
	// Serial.println((int16_t)((int16_t)readByte(BNO055_ADDRESS, BNO055_GYR_OFFSET_Z_MSB) << 8 | readByte(BNO055_ADDRESS, BNO055_GYR_OFFSET_Z_LSB)));

	printf("Accel/Gyro Calibration done!\r\n");
}

// calibrate magnetometer
void bno055_calibrate_mag(float * dest1) 
{
	uint8_t data[6]; // data array to hold mag x, y, z, data
	uint16_t ii = 0, sample_count = 0;
	int32_t mag_bias[3] = {0, 0, 0};
	int16_t mag_max[3] = {0, 0, 0}, mag_min[3] = {0, 0, 0};
	int16_t mag_temp[3] = {0, 0, 0};
	int jj = 0;
	
	printf("Mag Calibration: Wave device in a figure eight until done!\r\n");
	delay_ms(400);
	
	// Select page 0 to read sensors
	bno055_write_byte(BNO055_PAGE_ID, 0x00);
	// Select BNO055 system operation mode as NDOF for calibration
	bno055_write_byte(BNO055_OPR_MODE, CONFIGMODE );
	delay_ms(25);
	bno055_write_byte(BNO055_OPR_MODE, AMG );

	// In NDF fusion mode, mag data is in 16 LSB/microTesla, ODR is 20 Hz in forced mode
	sample_count = 256;
	for(ii = 0; ii < sample_count; ii++) {
		mag_temp[0] = 0;
		mag_temp[1] = 0;
		mag_temp[2] = 0;
		bno055_read_bytes(BNO055_MAG_DATA_X_LSB, 6, &data[0]);  // Read the six raw data registers into data array
		mag_temp[0] = (int16_t) (((int16_t)data[1] << 8) | data[0]) ;   // Form signed 16-bit integer for each sample in FIFO
		mag_temp[1] = (int16_t) (((int16_t)data[3] << 8) | data[2]) ;
		mag_temp[2] = (int16_t) (((int16_t)data[5] << 8) | data[4]) ;
		for (jj = 0; jj < 3; jj++) {
			if (ii == 0) {
				mag_max[jj] = mag_temp[jj]; // Offsets may be large enough that mag_temp[i] may not be bipolar! 
				mag_min[jj] = mag_temp[jj]; // This prevents max or min being pinned to 0 if the values are unipolar...
			} else {
				if(mag_temp[jj] > mag_max[jj]) mag_max[jj] = mag_temp[jj];
				if(mag_temp[jj] < mag_min[jj]) mag_min[jj] = mag_temp[jj];
			}
		}
		delay_ms(105);  // at 10 Hz ODR, new mag data is available every 100 ms
	}
	
	// printf("mag x min/max:"); Serial.println(mag_max[0]); Serial.println(mag_min[0]);
	// printf("mag y min/max:"); Serial.println(mag_max[1]); Serial.println(mag_min[1]);
	// printf("mag z min/max:"); Serial.println(mag_max[2]); Serial.println(mag_min[2]);
	
	mag_bias[0]  = (mag_max[0] + mag_min[0])/2;  // get average x mag bias in counts
	mag_bias[1]  = (mag_max[1] + mag_min[1])/2;  // get average y mag bias in counts
	mag_bias[2]  = (mag_max[2] + mag_min[2])/2;  // get average z mag bias in counts
		
	dest1[0] = (float) mag_bias[0] / 1.6;  // save mag biases in mG for use in main program
	dest1[1] = (float) mag_bias[1] / 1.6;  // mag data is 1.6 LSB/mg
	dest1[2] = (float) mag_bias[2] / 1.6;          
	
	// Return to config mode to write mag biases in offset register
	// This offset register is only used while in fusion mode when magnetometer sensitivity is 16 LSB/microTesla
	bno055_write_byte(BNO055_OPR_MODE, CONFIGMODE );
	delay_ms(25);
	
	//write biases to accelerometer offset registers as 16 LSB/microTesla
	bno055_write_byte(BNO055_MAG_OFFSET_X_LSB, (int16_t)mag_bias[0] & 0xFF);
	bno055_write_byte(BNO055_MAG_OFFSET_X_MSB, ((int16_t)mag_bias[0] >> 8) & 0xFF);
	bno055_write_byte(BNO055_MAG_OFFSET_Y_LSB, (int16_t)mag_bias[1] & 0xFF);
	bno055_write_byte(BNO055_MAG_OFFSET_Y_MSB, ((int16_t)mag_bias[1] >> 8) & 0xFF);
	bno055_write_byte(BNO055_MAG_OFFSET_Z_LSB, (int16_t)mag_bias[2] & 0xFF);
	bno055_write_byte(BNO055_MAG_OFFSET_Z_MSB, ((int16_t)mag_bias[2] >> 8) & 0xFF);
	
	// Check that offsets were properly written to offset registers
	// Serial.println("Average magnetometer bias = "); 
	// Serial.println((int16_t)((int16_t)readByte(BNO055_ADDRESS, BNO055_MAG_OFFSET_X_MSB) << 8 | readByte(BNO055_ADDRESS, BNO055_MAG_OFFSET_X_LSB))); 
	// Serial.println((int16_t)((int16_t)readByte(BNO055_ADDRESS, BNO055_MAG_OFFSET_Y_MSB) << 8 | readByte(BNO055_ADDRESS, BNO055_MAG_OFFSET_Y_LSB))); 
	// Serial.println((int16_t)((int16_t)readByte(BNO055_ADDRESS, BNO055_MAG_OFFSET_Z_MSB) << 8 | readByte(BNO055_ADDRESS, BNO055_MAG_OFFSET_Z_LSB)));
	// Select BNO055 system operation mode
	bno055_write_byte(BNO055_OPR_MODE, NDOF);
	delay_ms(25);
	
	printf("Mag Calibration done!\r\n");
}

// read the X/Y/Z axis of acceleration
void bno055_read_accel(int16_t *p_out)
{
	uint8_t byteData[6];  // x/y/z accel register data stored here
	bno055_read_bytes(BNO055_ACC_DATA_X_LSB, 6, &byteData[0]);  // Read the six raw data registers into data array
	p_out[0] = ((int16_t)byteData[1] << 8) | byteData[0];      // Turn the MSB and LSB into a signed 16-bit value
	p_out[1] = ((int16_t)byteData[3] << 8) | byteData[2];  
	p_out[2] = ((int16_t)byteData[5] << 8) | byteData[4]; 
}

// read the X/Y/Z axis of gyroscope
void bno055_read_gyro(int16_t *p_out)
{
	uint8_t byteData[6];  // x/y/z gyro register data stored here
	bno055_read_bytes(BNO055_GYR_DATA_X_LSB, 6, &byteData[0]);  // Read the six raw data registers sequentially into data array
	p_out[0] = ((int16_t)byteData[1] << 8) | byteData[0];       // Turn the MSB and LSB into a signed 16-bit value
	p_out[1] = ((int16_t)byteData[3] << 8) | byteData[2];  
	p_out[2] = ((int16_t)byteData[5] << 8) | byteData[4]; 
}

// read the X/Y/Z axis of magnetometer
void bno055_read_mag(int16_t *p_out)
{
	uint8_t byteData[6];  // x/y/z gyro register data stored here
	bno055_read_bytes(BNO055_MAG_DATA_X_LSB, 6, &byteData[0]);  // Read the six raw data registers sequentially into data array
	p_out[0] = ((int16_t)byteData[1] << 8) | byteData[0];       // Turn the MSB and LSB into a signed 16-bit value
	p_out[1] = ((int16_t)byteData[3] << 8) | byteData[2];  
	p_out[2] = ((int16_t)byteData[5] << 8) | byteData[4];
}

// read the W/X/Y/Z axis of quaternion
void bno055_read_quat(int16_t *p_out)
{
	uint8_t byteData[8];  // x/y/z gyro register data stored here
	bno055_read_bytes(BNO055_QUA_DATA_W_LSB, 8, &byteData[0]);  // Read the six raw data registers sequentially into data array
	p_out[0] = ((int16_t)byteData[1] << 8) | byteData[0];       // Turn the MSB and LSB into a signed 16-bit value
	p_out[1] = ((int16_t)byteData[3] << 8) | byteData[2];  
	p_out[2] = ((int16_t)byteData[5] << 8) | byteData[4];
	p_out[3] = ((int16_t)byteData[7] << 8) | byteData[6];
}

// read the heading/roll/pitch of euler
void bno055_read_eul(int16_t *p_out)
{
	uint8_t byteData[6];  // x/y/z gyro register data stored here
	bno055_read_bytes(BNO055_EUL_HEADING_LSB, 6, &byteData[0]);  // Read the six raw data registers sequentially into data array
	p_out[0] = ((int16_t)byteData[1] << 8) | byteData[0];       // Turn the MSB and LSB into a signed 16-bit value
	p_out[1] = ((int16_t)byteData[3] << 8) | byteData[2];  
	p_out[2] = ((int16_t)byteData[5] << 8) | byteData[4];
}

// read the X/Y/Z axis of linear acceleration
void bno055_read_lia(int16_t *p_out)
{
	uint8_t byteData[6];  // x/y/z gyro register data stored here
	bno055_read_bytes(BNO055_LIA_DATA_X_LSB, 6, &byteData[0]);  // Read the six raw data registers sequentially into data array
	p_out[0] = ((int16_t)byteData[1] << 8) | byteData[0];       // Turn the MSB and LSB into a signed 16-bit value
	p_out[1] = ((int16_t)byteData[3] << 8) | byteData[2];  
	p_out[2] = ((int16_t)byteData[5] << 8) | byteData[4];
}

// read the X/Y/Z axis of gravity vector
void bno055_read_grv(int16_t *p_out)
{
	uint8_t byteData[6];  // x/y/z gyro register data stored here
	bno055_read_bytes(BNO055_GRV_DATA_X_LSB, 6, &byteData[0]);  // Read the six raw data registers sequentially into data array
	p_out[0] = ((int16_t)byteData[1] << 8) | byteData[0];       // Turn the MSB and LSB into a signed 16-bit value
	p_out[1] = ((int16_t)byteData[3] << 8) | byteData[2];  
	p_out[2] = ((int16_t)byteData[5] << 8) | byteData[4];
}

void bno055_read_calibrate_state(void)
{
	uint8_t temp = 0;

	bno055_read_bytes(BNO055_CALIB_STAT, 1, &temp);

	printf("system calibration status = %d\r\n", (0xC0&temp)>>6);
	printf("gyro calibration status = %d\r\n", (0xC0&temp)>>4);
	printf("accel calibration status = %d\r\n", (0x0C&temp)>>2);
	printf("mag calibration status = %d\r\n", (0x03&temp)>>0);
}

// 1 C = 1 LSB
// 2 F = 1 LSB
int8_t bno055_read_temp()
{
	uint8_t temp = 0;

	bno055_read_bytes(BNO055_TEMP, 1, &temp);

	return temp;
}

// read system status & self test result & system error
void bno055_read_status(uint8_t *sys_stat, uint8_t *st_ret, uint8_t * sys_err)
{
	uint8_t temp = 0;

	if (!sys_stat || !st_ret || !sys_err) {
		return;
	}

	// Select page 0 to read sensors
	bno055_write_byte(BNO055_PAGE_ID, 0x00);

	/* system status
	 0 = idel
	 1 = system error
	 2 = initializing peripherals
	 3 = system initialization
     4 = executing self-test
	 5 = sensor fusio algorithm running
	 6 = system running without fusion algorithms
	*/
	bno055_read_bytes(BNO055_SYS_STATUS, 1, &temp);
	*sys_stat = temp;

	/* self test result
	 0 = fail
	 1 = pass
	 BIT0: accelerometer self test
	 BIT1: magnetometer self test
	 BIT2: gyroscope self test
	 BIT3: mcu self test
	*/
	bno055_read_bytes(BNO055_ST_RESULT, 1, &temp);
	*st_ret = temp;
	
	/*
	 0 = no error
	 1 = peripheral initialization error
	 2 = system initialization error
	 3 = self test result failed
	 4 = register map value out of range
	 5 = register map address out of range
	 6 = register map write error
	 7 = BNO low power mode not available for selected operation mode
	 8 = accelerometer power mode not available
	 9 = fusion algorithm configuration error
	 A = sensor configuration error
	*/
	bno055_read_bytes(BNO055_SYS_ERR, 1, &temp);
	*sys_err = temp;
}

// change the chip's axis remap
int8_t bno055_set_axis_remap(uint8_t mode)
{
	// Select BNO055 config mode
	bno055_write_byte(BNO055_OPR_MODE, CONFIGMODE);

	bno055_write_byte(BNO055_AXIS_MAP_CONFIG, mode);

	bno055_write_byte(BNO055_OPR_MODE, NDOF);

	return 0;
}

// change the chip's axis sign
int8_t bno055_set_axis_sign(uint8_t mode)
{
	// Select BNO055 config mode
	bno055_write_byte(BNO055_OPR_MODE, CONFIGMODE);

	bno055_write_byte(BNO055_AXIS_MAP_SIGN, mode);

	bno055_write_byte(BNO055_OPR_MODE, NDOF);

	return 0;
}

int8_t bno055_enter_suspend_mode()
{
	// Select BNO055 config mode
	bno055_write_byte(BNO055_OPR_MODE, CONFIGMODE);
	delay_ms(25);

	bno055_write_byte(BNO055_PWR_MODE, 0x02);
	delay_ms(25);

	bno055_write_byte(BNO055_OPR_MODE, NDOF);
	delay_ms(25);

	return 0;
}

int8_t bno055_enter_normal_mode()
{
	// Select BNO055 config mode
	bno055_write_byte(BNO055_OPR_MODE, CONFIGMODE);
	delay_ms(25);

	bno055_write_byte(BNO055_PWR_MODE, 0x00);
	delay_ms(25);

	bno055_write_byte(BNO055_OPR_MODE, NDOF);
	delay_ms(25);

	return 0;
}

void bno055_initial()
{
	// Select BNO055 config mode
	bno055_write_byte(BNO055_OPR_MODE, CONFIGMODE);
	delay_ms(25);
	// Select page 1 to configure sensors
	bno055_write_byte(BNO055_PAGE_ID, 0x01);

#if 0
	// Configure ACC
	bno055_write_byte(BNO055_ACC_CONFIG, NormalA << 5 | ABW_31_25Hz << 2 | AFS_2G);
	// Configure GYR
	bno055_write_byte(BNO055_GYRO_CONFIG_0, GBW_23Hz << 3 | GFS_250DPS);
	bno055_write_byte(BNO055_GYRO_CONFIG_1, NormalG);
	// Configure MAG
	bno055_write_byte(BNO055_MAG_CONFIG, Normal << 5 | Regular << 3 | MODR_10Hz);
#endif

	// Select page 0 to read sensors
	bno055_write_byte(BNO055_PAGE_ID, 0x00);
	
	// TEMP_SRC REG:
	// BIT1~0: 00-Accelerometer; 01-Gyroscope
	// Select BNO055 gyro temperature source 
	bno055_write_byte(BNO055_TEMP_SOURCE, 0x01);
	
	// UNIT_SEL REG: default 0
	// BIT7:(Oritention Mode) 0-Windows ; 1-Android
	// BIT4:(Temp Unit) 0-C ; 1-F
	// BIT2:(Euler Unit) 0-Degrees ; 1-Radians
	// BIT1:(Gyro Angular Rate) 0-dps ; 1-rps
	// BIT0:(Acceleration Unit) 0-m/s2 ; 1-mg
	// Select BNO055 sensor units (temperature in degrees C, rate in dps, accel in mg)
	bno055_write_byte(BNO055_UNIT_SEL, 0x01 );
	
	// Select BNO055 system power mode
	bno055_write_byte(BNO055_PWR_MODE, Normalpwr);
	
	// Select BNO055 system operation mode
	bno055_write_byte(BNO055_OPR_MODE, NDOF);

	delay_ms(25);
}
  
void bno055_setup()
{
	float gyroBias[3]  = {0, 0, 0};// for gyroscope
	float magBias[3]   = {0, 0, 0};// for magnetometer
	float accelBias[3] = {0, 0, 0};// for accelerometer

	bno055_verify_chip();

#if 0
	bno055_calibrate_accel_gyro(accelBias, gyroBias);
	printf("avarage accelerometer bias(mg) = %f, %f, %f\r\n", accelBias[0], accelBias[1], accelBias[2]);
	printf("avarage gyro bias(dps) = %f, %f, %f\r\n", gyroBias[0], gyroBias[1], gyroBias[2]);
	bno055_calibrate_mag(magBias);
	printf("avarage magnetometer bias(mg) = %f, %f, %f\r\n", magBias[0], magBias[1], magBias[2]);
#endif

	bno055_read_calibrate_state();

	bno055_initial();
}
  
void bno055_loop()
{
	int16_t raw_data[4] = {0};

	float ax, ay, az;// for acceleration
	float gx, gy, gz;// for gyroscope
	float mx, my, mz; // for magnetometer
	float qw, qx, qy, qz;// for quaternion

	float pitch, yaw, roll;// for euler
	float lia_x, lia_y, lia_z;// for linear acceleration
	float grv_x, grv_y, grv_z;// for gravity vector

	// 1 mg = 1 LSB
	// 1 m/s^2 = 100 LSB
	bno055_read_accel(raw_data);
	ax = (float)raw_data[0] / 16;
	ay = (float)raw_data[1] / 16;
	az = (float)raw_data[2] / 16;

	printf("acceleration data: x=%f, y=%f, z=%f\r\n", ax, ay, az);

	// 1 dps = 16 LSB
	// 1 rps = 900 LSB
	bno055_read_gyro(raw_data);
	gx = (float)raw_data[0] / 16;
	gy = (float)raw_data[1] / 16;
	gz = (float)raw_data[2] / 16;

	printf("gyroscope data: x=%f, y=%f, z=%f\r\n", gx, gy, gz);

	// 1 ut = 16 LSB
	bno055_read_mag(raw_data);
	mx = (float)raw_data[0] / 16;
	my = (float)raw_data[1] / 16;
	mz = (float)raw_data[2] / 16;

	printf("magnetometer data: x=%f, y=%f, z=%f\r\n", mx, my, mz);

	// 1 quaternion = 2^14 LSB
	bno055_read_quat(raw_data);
	qw = (float)raw_data[0] / 16384;
	qx = (float)raw_data[1] / 16384;
	qy = (float)raw_data[2] / 16384;
	qz = (float)raw_data[3] / 16384;

	printf("quaternion data: qw=%f, qx=%f, qy=%f, qz=%f\r\n", qw, qx, qy, qz);

	// 1 degrees = 16 LSB
	// 1 radians = 900 LSB
	bno055_read_eul(raw_data);
	yaw = (float)raw_data[0] / 16;
	roll = (float)raw_data[1] / 16;
	pitch = (float)raw_data[2] / 16;

	printf("euler data: yaw=%f, roll=%f, pitch=%f\r\n", yaw, roll, pitch);

	// 1 mg = 1 LSB
	// 1 m/s^2 = 100 LSB
	bno055_read_lia(raw_data);
	lia_x = (float)raw_data[0];
	lia_y = (float)raw_data[1];
	lia_z = (float)raw_data[2];

	printf("linear acceleration data: x=%f, y=%f, z=%f\r\n", lia_x, lia_y, lia_z);

	// 1 mg = 1 LSB
	// 1 m/s^2 = 100 LSB
	bno055_read_grv(raw_data);
	grv_x = (float)raw_data[0];
	grv_y = (float)raw_data[1];
	grv_z = (float)raw_data[2];

	printf("grv data: x=%f, y=%f, z=%f\r\n", grv_x, grv_y, grv_z);
}
  
int main(void)
{
	return 0;

}
