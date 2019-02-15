#include "MobitLTEBG96Serial.h"

unsigned int bufferHead;
char rxBuffer[RX_BUFFER_LENGTH];
int errorCode = -1;

// TBD: Wait Implement
int WriteToBG96Serial(char *data_buf)
{
    return 0;
}

// TBD: Wait Implement
int ReadFromBG96Serial()
{
    return 0;
}

// TBD: Wait Implement
unsigned long millis()
{
    return 0;
}

// TBD: Wait Implement
void delay_ms(unsigned int val)
{
}

void InitSerial()
{
    cleanBuffer();
}

bool sendDataAndCheck(const char *data_buf, const char *ok_str, const char *err_str, unsigned int timeout)
{
    delay_ms(100);

    // to read recv fifo till empty
    while(ReadFromBG96Serial()>=0);

    int data_len = strlen(data_buf);
    int send_bytes = WriteToBG96Serial(data_buf);

#ifdef UART_DEBUG
    printf("\r\n");
    printf("%s", data_buf);
    printf("\r\n");
    printf("Send Data len :");
    printf("%d", send_bytes);
    printf("\r\n");
#endif

    if (send_bytes == data_len) {
        if (readResponseAndSearch_also(ok_str, err_str, timeout)) {
            return true;
        }
    }

    return false;
}


bool sendATcommand(const char *command)
{
    delay_ms(100);

    // to read recv fifo till empty
    while(ReadFromBG96Serial()>=0);

    WriteToBG96Serial("AT");

    int cmd_len = strlen(command);
    int send_bytes = WriteToBG96Serial(command);

#if defined UART_DEBUG
    printf("\r\n");
    printf("-> ");
    printf("AT");
    printf("%s", command);
    printf("\r\n");
#endif
    if (send_bytes != cmd_len){
        return false;
    }
    WriteToBG96Serial("\r\n");
    return true;
}


unsigned int readResponseByteToBuffer()
{
    char c = ReadFromBG96Serial();

    rxBuffer[bufferHead] = c;
    bufferHead = (bufferHead + 1) % RX_BUFFER_LENGTH;

#if defined UART_DEBUG
    if (c == '\n'){
        printf("%c", c);
        printf("<- ");
    }else {
        printf("%c", c);
    }
#endif

    return 1;
}

unsigned int readResponseToBuffer(unsigned int timeout)
{
    unsigned long start_time = millis();
    unsigned int recv_len = 0;

    cleanBuffer();
    while (millis() - start_time < timeout * 1000UL) {
        if (serialAvailable()) {
            recv_len += readResponseByteToBuffer();
        }
    }
    return recv_len;
}

Cmd_Response_t readResponseAndSearchChr(const char test_chr, unsigned int timeout)
{
    unsigned long start_time = millis();
    unsigned int recv_len = 0;

    cleanBuffer();
    while (millis() - start_time < timeout * 1000UL) {
        if (serialAvailable()) {
            recv_len += readResponseByteToBuffer();
            if (searchChrBuffer(test_chr)) {
                return SUCCESS_RESPONSE;
            }
        }
    }
    if (recv_len > 0){
        return UNKNOWN_RESPONSE;
    } else {
        return TIMEOUT_RESPONSE;
    }
}

Cmd_Response_t readResponseAndSearch(const char *test_str, unsigned int timeout)
{
    unsigned long start_time = millis();
    unsigned int recv_len = 0;

    cleanBuffer();
    while (millis() - start_time < timeout * 1000UL) {
        if (serialAvailable()) {
            recv_len += readResponseByteToBuffer();
            if (searchStrBuffer(test_str)) {
                return SUCCESS_RESPONSE;
            }
        }
    }
    if (recv_len > 0) {
        return UNKNOWN_RESPONSE;
    } else {
        return TIMEOUT_RESPONSE;
    }
}

Cmd_Response_t readResponseAndSearch_also(const char *test_str, const char *e_test_str, unsigned int timeout)
{
    unsigned long start_time = millis();
    unsigned int recv_len = 0;

    errorCode = -1;
    cleanBuffer();
    while (millis() - start_time < timeout * 1000UL) {
        if (serialAvailable()) {
            recv_len += readResponseByteToBuffer();
            if (searchStrBuffer(test_str)) {
                return SUCCESS_RESPONSE;
            } else if (searchStrBuffer(e_test_str)) {
                start_time = millis();
                while (millis() - start_time < 100UL) {
                    if (serialAvailable()) {
                        recv_len += readResponseByteToBuffer();
                    }
                }
                char *str_buf = searchStrBuffer(": ");
                if (str_buf != NULL) {
                    char err_code[8];
                    strcpy(err_code, str_buf + 2);
                    char *end_buf = strstr(err_code, "\r\n");
                    *end_buf = '\0';
                    errorCode = atoi(err_code);
                }
                return FIAL_RESPONSE;
            }
        }
    }
    if (recv_len > 0){
        return UNKNOWN_RESPONSE;
    } else {
        return TIMEOUT_RESPONSE;
    }
}

Cmd_Response_t sendAndSearchChr(const char *command, const char test_chr, unsigned int timeout)
{
    for (int i = 0; i < 3; i++) {
        if (sendATcommand(command)) {
            if (readResponseAndSearchChr(test_chr, timeout) == SUCCESS_RESPONSE) {
                return SUCCESS_RESPONSE;
            }
        }
    }
    return TIMEOUT_RESPONSE;
}

Cmd_Response_t sendAndSearch(const char *command, const char *test_str, unsigned int timeout)
{
    for (int i = 0; i < 3; i++) {
        if (sendATcommand(command)) {
            if (readResponseAndSearch(test_str, timeout) == SUCCESS_RESPONSE) {
                return SUCCESS_RESPONSE;
            }
        }
    }
    return TIMEOUT_RESPONSE;
}

Cmd_Response_t sendAndSearch_also(const char *command, const char *test_str, const char *e_test_str, unsigned int timeout)
{
    Cmd_Response_t resp_status = UNKNOWN_RESPONSE;

    for (int i = 0; i < 3; i++) {
        if (sendATcommand(command)) {
            resp_status = readResponseAndSearch_also(test_str, e_test_str, timeout);
            return resp_status;
        }
    }
    return resp_status;
}

char *searchStrBuffer(const char *test_str)
{
    int buf_len = strlen((const char *)rxBuffer);

    if (buf_len < RX_BUFFER_LENGTH) {
        return strstr((const char *)rxBuffer, test_str);
    } else {
        return NULL;
    }
}

char *searchChrBuffer(const char test_chr)
{
    int buf_len = strlen((const char *)rxBuffer);

    if (buf_len < RX_BUFFER_LENGTH) {
        return strchr((const char *)rxBuffer, test_chr);
    } else {
        return NULL;
    }
}

bool returnErrorCode(int *s_err_code)
{
    *s_err_code = -1;

    if (errorCode != -1) {
        *s_err_code = errorCode;
        return true;
    }

    return false;
}

void cleanBuffer()
{
    memset(rxBuffer, '\0', RX_BUFFER_LENGTH);
    bufferHead = 0;
}

// TBD: Wait Implement
int serialAvailable()
{
    unsigned int ret;

    return ret;
}
