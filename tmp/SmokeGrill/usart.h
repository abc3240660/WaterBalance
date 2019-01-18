#ifndef __USART_H
#define __USART_H
#include "stdio.h"	
#include "sys.h" 
#define USART_MAX_RECV_LEN  			1024*51

#define EN_USART1_RX 			1

#define FILE_MD5_MAX_LEN  32
#define SSL_MAX_LEN (FILE_MD5_MAX_LEN/2)

extern u8* USART_RX_BUF;
extern u8* USART_RX_BUF_BAK;

extern u32 USART_RX_STA;
void uart_init(u32 bound);
int run_cmd_from_usart(u8 *data, u16 num);
#endif


