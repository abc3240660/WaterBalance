#include "MobitBG96.h"

int errorCode = -1;
unsigned int bufferHead = 0;
char rxBuffer[RX_BUFFER_LENGTH] = "";

/////////////////////////////////// Wait Implement ///////////////////////////////////
void SetPinDir(u8 pin_num, u8 dir)
{
}

void SetPinVal(u8 pin_num, u8 val)
{
}

int WriteToBG96Serial(char *data_buf)
{
    return 0;
}

int ReadFromBG96Serial()
{
    return 0;
}

int serialAvailable()
{
    unsigned int ret;

    return ret;
}

unsigned long millis()
{
    return 0;
}

void delay_ms(unsigned int val)
{
}

/////////////////////////////////// BG96 Common ///////////////////////////////////
bool InitModule()
{
    SetPinDir(RESET_PIN, OUTPUT);
    SetPinVal(RESET_PIN, LOW);
    SetPinDir(POWKEY_PIN, OUTPUT);
    delay_ms(100);
    SetPinVal(POWKEY_PIN, HIGH);
    delay_ms(200);
    SetPinVal(POWKEY_PIN, LOW);
    if (readResponseToBuffer(5)) {
        if (searchStrBuffer(RESPONSE_READY)) {
            return true;
        }
    }
    delay_ms(1000);
    SetPinVal(POWKEY_PIN, HIGH);
    delay_ms(2000);
    SetPinVal(POWKEY_PIN, LOW);
    if (readResponseToBuffer(5)) {
        if (searchStrBuffer(RESPONSE_POWER_DOWN)) {
            delay_ms(10000);
        }
    }

    return false;
}

bool SetDevCommandEcho(bool echo)
{
    const char *cmd;

    if (echo == true) {
        cmd = "E1";
    } else {
        cmd = "E0";
    }
    if (sendAndSearch(cmd, RESPONSE_OK, 2)) {
        return true;
    }
    return false;
}

bool GetDevInformation(char *inf)
{
    if (sendAndSearch(DEV_INFORMATION, RESPONSE_OK, 2)) {
        char *end_buf = searchStrBuffer(RESPONSE_CRLF_OK);
        *end_buf = '\0';
        strcpy(inf, rxBuffer);
        return true;
    }
    return false;
}

bool GetDevVersion(char *ver)
{
    if (sendAndSearch(DEV_VERSION, RESPONSE_OK, 2)) {
        char *end_buf = searchStrBuffer(RESPONSE_CRLF_OK);
        *end_buf = '\0';
        strcpy(ver, rxBuffer);
        return true;
    }
    return false;
}

bool GetDevIMEI(char *imei)
{
    if (sendAndSearch(DEV_IMEI, RESPONSE_OK, 2)) {
        char *end_buf = searchStrBuffer(RESPONSE_CRLF_OK);
        *end_buf = '\0';
        strcpy(imei, rxBuffer);
        return true;
    }
    return false;
}

Cmd_Response_t SetDevFunctionality(Functionality_t mode)
{
    char cmd[16] = "";
    Cmd_Response_t fun_status;

    strcpy(cmd, DEV_FUN_LEVEL);
    switch (mode)
    {
        case MINIMUM_FUNCTIONALITY:
            strcat(cmd, "=0");
            break;
        case FULL_FUNCTIONALITY:
            strcat(cmd, "=1");
            break;
        case DISABLE_RF:
            strcat(cmd, "=4");
            break;
        default:
            return UNKNOWN_RESPONSE;
    }
    fun_status = sendAndSearch_also(cmd, RESPONSE_OK, RESPONSE_ERROR, 15);
    return fun_status;
}

bool DevLocalRate(unsigned long *rate, Cmd_Status_t status)
{
    int i = 0;
    char cmd[16] = "";
    char buf[16] = "";

    strcpy(cmd, DEV_LOCAL_RATE);
    if (status == READ_MODE) {
        strcat(cmd, "?");
        if (sendAndSearch(cmd, RESPONSE_OK, 2)) {
            char *sta_buf = searchStrBuffer(": ");
            char *end_buf = searchStrBuffer(RESPONSE_CRLF_OK);
            *end_buf = '\0';
            *rate = atol(sta_buf + 2);
            return true;
        }
    } else if (status == WRITE_MODE) {
        for (i = 0; i < sizeof(Band_list)/sizeof(Band_list[0]); i++) {
            if (*rate == Band_list[i]) {
                memset(buf, 0, 16);
                sprintf(buf, "=%ld;&W", *rate);
                strcat(cmd, buf);
                if (sendAndSearch(cmd, RESPONSE_OK, 2)) {
                    return true;
                }
            }
        }
    }
    return false;
}

bool GetDevSimIMSI(char *imsi)
{
    if (sendAndSearch(DEV_SIM_IMSI, RESPONSE_OK, 2)) {
        char *end_buf = searchStrBuffer(RESPONSE_CRLF_OK);
        *end_buf = '\0';
        strcpy(imsi, rxBuffer);
        return true;
    }
    return false;
}

bool DevSimPIN(char *pin, Cmd_Status_t status)
{
    char cmd[16] = "";

    strcpy(cmd, DEV_SIM_PIN);
    if (status == READ_MODE) {
        strcat(cmd, "?");
        if (sendAndSearch(cmd, "READY",2)) {
            //pin = "READY";
            return true;
        }
    } else if (status == WRITE_MODE) {
        char buf[16];
        sprintf(buf, "=\"%s\"", pin);
        strcat(cmd, buf);
        if (sendAndSearch(cmd, RESPONSE_OK, 2)) {
            return true;
        }
    }
    return false;
}

bool GetDevSimICCID(char *iccid)
{
    if (sendAndSearch(DEV_SIM_ICCID, RESPONSE_OK, 2)) {
        char *end_buf = searchStrBuffer(RESPONSE_CRLF_OK);
        *end_buf = '\0';
        char *sta_buf = searchStrBuffer(": ");
        strcpy(iccid, sta_buf + 2);
        return true;
    }
    return false;
}

Net_Status_t DevNetRegistrationStatus()
{
    char cmd[16] = "";
    Net_Status_t n_status = NOT_REGISTERED;

    strcpy(cmd, DEV_NET_STATUS_G);
    strcat(cmd, "?");
    if (sendAndSearch(cmd, RESPONSE_OK, 2)) {
        char *end_buf = searchStrBuffer(RESPONSE_CRLF_OK);
        *end_buf = '\0';
        char *sta_buf = searchChrBuffer(',');
        n_status = atoi(sta_buf + 1);
        switch (n_status)
        {
            case REGISTERED:
            case REGISTERED_ROAMING:
                return n_status;
            default:
                break;
        }
    }

    strcpy(cmd, DEV_EPS_NET_STATUS);
    strcat(cmd, "?");
    if (sendAndSearch(cmd, RESPONSE_OK, 2)) {
        char *end_buf = searchStrBuffer(RESPONSE_CRLF_OK);
        *end_buf = '\0';
        char *sta_buf = searchChrBuffer(',');
        n_status = atoi(sta_buf + 1);
        switch (n_status)
        {
            case REGISTERED:
            case REGISTERED_ROAMING:
                return n_status;
            default:
                break;
        }
    }
    return n_status;
}

bool GetDevNetSignalQuality(unsigned int *rssi)
{
    if (sendAndSearch(DEV_NET_RSSI, RESPONSE_OK, 2)) {
        char *sta_buf = searchStrBuffer(": ");
        char *end_buf = searchChrBuffer(',');
        *end_buf = '\0';
        *rssi = atoi(sta_buf + 2);
        return true;
    }
    return false;
}

Cmd_Response_t ScanOperatorNetwork(char *net)
{
    char cmd[16] = "";
    Cmd_Response_t scan_status;

    strcpy(cmd, DEV_NET_OPERATOR);
    strcat(cmd, "=?");
    scan_status = sendAndSearch_also(cmd, RESPONSE_OK, RESPONSE_ERROR, 180);
    if (scan_status == SUCCESS_RESPONSE) {
        char *end_buf = searchStrBuffer(RESPONSE_CRLF_OK);
        *end_buf = '\0';
        char *sta_buf = searchStrBuffer(": ");
        strcpy(net, sta_buf + 2);
    } else if (scan_status == FIAL_RESPONSE) {
        char *sta_buf = searchStrBuffer(": ");
        strcpy(net, sta_buf + 2);
    }
    return scan_status;
}

Cmd_Response_t DevOperatorNetwork(unsigned int *mode, unsigned int *format, char *oper, Net_Type_t *act, Cmd_Status_t status)
{
    char cmd[16] = "";
    Cmd_Response_t oper_status = UNKNOWN_RESPONSE;

    strcpy(cmd, DEV_NET_OPERATOR);
    if (status == READ_MODE) {
        strcat(cmd, "?");
        oper_status = sendAndSearch_also(cmd, RESPONSE_OK, RESPONSE_ERROR, 5);
        if (oper_status == SUCCESS_RESPONSE) {
            char *end_buf = searchStrBuffer(RESPONSE_CRLF_OK);
            *end_buf = '\0';
            char *sta_buf = searchStrBuffer(": ");
            char message[64] = "";
            char *p[5] = {NULL};
            int i = 0;
            strcpy(message, sta_buf + 2);
            p[0] = strtok(message, ",");
            while (p[i] != NULL) {
                i++;
                p[i] = strtok(NULL, ",");
            }
            p[i] = '\0';
            *mode = atoi(p[0]);
            *format = atoi(p[1]);
            strcpy(oper,p[2]);
            *act = atoi(p[3]);
        }
    } else if (status == WRITE_MODE) {
        char buf[32] = "";
        sprintf(buf, "=%d,%d,\"%s\",%d", *mode, *format, oper, *act);
        strcat(cmd, buf);
        oper_status = sendAndSearch_also(cmd, RESPONSE_OK, RESPONSE_ERROR, 30);
    }
    return oper_status;
}

bool GetDevNetworkInformation(char *type, char *oper, char *band, char *channel)
{
    if (sendAndSearch(DEV_NET_INFORMATION, RESPONSE_OK, 2)) {
        char *sta_buf = searchStrBuffer(": ");
        char *end_buf = searchStrBuffer(RESPONSE_CRLF_OK);
        *end_buf = '\0';
        char message[64] = "";
        char *p[5] = {NULL};
        int i = 0;
        strcpy(message, sta_buf + 2);
        p[0] = strtok(message, ",");
        while (p[i] != NULL) {
            i++;
            p[i] = strtok(NULL,",");
        }
        p[i] = '\0';
        strcpy(type, p[0]);
        strcpy(oper, p[1]);
        strcpy(band, p[2]);
        strcpy(channel, p[3]);
        return true;
    }
    return false;
}

bool DevNetPacketCounter(unsigned long *send_bytes, unsigned long *recv_bytes, bool clean)
{
    char cmd[16] = "";

    strcpy(cmd, DEV_NET_PACKET_COUNTER);
    strcat(cmd, "?");
    if (sendAndSearch(cmd, RESPONSE_OK, 2)) {
        char *end_buf = searchStrBuffer(RESPONSE_CRLF_OK);
        *end_buf = '\0';
        char *med_buf = searchChrBuffer(',');
        *med_buf = '\0';
        char *sta_buf = searchStrBuffer(": ");
        *send_bytes = atol(sta_buf + 2);
        *recv_bytes = atol(med_buf + 1);
        if (clean == true) {
            memset(cmd, '\0', 16);
            strcpy(cmd, DEV_NET_PACKET_COUNTER);
            strcat(cmd, "=0");
            if (sendAndSearch(cmd, RESPONSE_OK, 2)) {
                return true;
            } else {
                return false;
            }
        }
        return true;
    }
    return false;
}

bool DevPowerDown()
{
    char cmd[16] = "";

    strcpy(cmd, DEV_POWER_DOWN);
    strcat(cmd, "=1");
    if (sendAndSearch(cmd, RESPONSE_POWER_DOWN, 2)) {
        return true;
    }
    return false;
}

bool DevClock(char *d_clock, Cmd_Status_t status)
{
    char cmd[32] = "";

    strcpy(cmd, DEV_CLOCK);
    if (status == READ_MODE) {
        strcat(cmd, "?");
        if (sendAndSearch(cmd, RESPONSE_OK, 2)) {
            char *end_buf = searchStrBuffer(RESPONSE_CRLF_OK);
            *end_buf = '\0';
            char *sta_buf = searchStrBuffer(": ");
            strcpy(d_clock, sta_buf + 2);
            return true;
        }
    } else if (status == WRITE_MODE) {
        char buf[32] = "";
        sprintf(buf, "=\"%s\"", d_clock);
        strcat(cmd, buf);
        if (sendAndSearch_also(cmd, RESPONSE_OK, RESPONSE_ERROR, 2)) {
            return true;
        }
    }
    return false;
}

/////////////////////////////////// BG96 TCPIP ///////////////////////////////////
bool SetDevAPNParameters(unsigned int pdp_index, Protocol_Type_t type, char *apn, char *usr, char *pwd, Authentication_Methods_t met)
{
    char cmd[64] = "";
    char buf[64] = "";

    strcpy(cmd, APN_PARAMETERS);
    sprintf(buf, "=%d,%d,\"%s\",\"%s\",\"%s\",%d", pdp_index, type, apn, usr, pwd, met);
    strcat(cmd, buf);
    if (sendAndSearch(cmd, RESPONSE_OK, 2)) {
        return true;
    }
    return false;
}

Cmd_Response_t ActivateDevAPN(unsigned int pdp_index)
{
    char cmd[16] = "";
    char buf[8] = "";

    strcpy(cmd, ACTIVATE_APN);
    sprintf(buf, "=%d", pdp_index);
    strcat(cmd, buf);
    return sendAndSearch_also(cmd, RESPONSE_OK, RESPONSE_ERROR, 150);
}

bool DeactivateDevAPN(unsigned int pdp_index)
{
    char cmd[16] = "";
    char buf[8] = "";

    strcpy(cmd, DEACTIVATE_APN);
    sprintf(buf, "=%d", pdp_index);
    strcat(cmd, buf);
    if (sendAndSearch_also(cmd, RESPONSE_OK, RESPONSE_ERROR, 40) > 0) {
        return true;
    }
    return false;
}

bool GetDevAPNIPAddress(unsigned int pdp_index, char *ip)
{
    char cmd[16] = "";
    char buf[8] = "";

    strcpy(cmd, GET_APN_IP_ADDRESS);
    sprintf(buf, "=%d", pdp_index);
    strcat(cmd, buf);
    if (sendAndSearch(cmd, RESPONSE_OK, 2)) {
        char *end_buf = searchStrBuffer(RESPONSE_CRLF_OK);
        *end_buf = '\0';
        char *sta_buf = searchChrBuffer(',');
        if (strcmp(sta_buf + 1, "0.0.0.0") <= 0) {
            return false;
        }
        strcpy(ip, sta_buf + 1);
        return true;
    }
    return false;
}

bool InitAPN(unsigned int pdp_index, char *apn, char *usr, char *pwd, char *err_code)
{
    Net_Status_t i_status;
    Cmd_Response_t init_status;
    const char *e_str;
    unsigned long start_time = millis();

    while (!DevSimPIN("", READ_MODE)) {
        if (millis() - start_time >= 10*1000UL) {
            e_str = "\r\nAPN ERROR: No SIM card detected!\r\n";
            strcpy(err_code, e_str);
            return false;
        }
    }
    start_time = millis();
    while (i_status != REGISTERED && i_status != REGISTERED_ROAMING) {
        i_status = DevNetRegistrationStatus();
        if (millis() - start_time >= 120*1000UL) {
            e_str = "\r\nAPN ERROR: Can't registered to the Operator network!\r\n";
            strcpy(err_code, e_str);
            return false;
        }
    }
    start_time = millis();
    while (millis() - start_time <= 300*1000UL) {
        if (SetDevAPNParameters(pdp_index, IPV4, apn, usr, pwd, PAP_OR_CHAP))
        {
            char i_ip[16] = "";
            if (GetDevAPNIPAddress(pdp_index, i_ip)) {
                sprintf(err_code, "\r\nAPN OK: The IP address is %s\r\n", i_ip);
                return true;
            } else {
                init_status = ActivateDevAPN(pdp_index);
                if (init_status == TIMEOUT_RESPONSE) {
                    e_str = "\r\nAPN ERROR: Please reset your device!\r\n";
                    strcpy(err_code, e_str);
                    return false;
                }
            }
        }
        e_str = "\r\nAPN ERROR: Activate APN file!\r\n";
        strcpy(err_code, e_str);
    }
    return false;
}

bool OpenSocketService(unsigned int pdp_index, unsigned int socket_index, Socket_Type_t socket, char *ip, unsigned int port, unsigned int local_port, Access_Mode_t mode)
{
    char cmd[128] = "";
    char buf[128] = "";

    strcpy(cmd, OPEN_SOCKET);
    switch (socket)
    {
        case TCP_CLIENT:
            sprintf(buf, "=%d,%d,\"TCP\",\"%s\",%d,%d,%d", pdp_index, socket_index, ip, port, local_port, mode);
            break;
        case TCP_SEVER:
            sprintf(buf, "=%d,%d,\"TCP LISTENER\",\"%s\",%d,%d,%d", pdp_index, socket_index, ip, port, local_port, mode);
            break;
        case UDP_CLIENT:
            sprintf(buf, "=%d,%d,\"UDP\",\"%s\",%d,%d,%d", pdp_index, socket_index, ip, port, local_port, mode);
            break;
        case UDP_SEVER:
            sprintf(buf, "=%d,%d,\"UDP SERVICE\",\"%s\",%d,%d,%d", pdp_index, socket_index, ip, port, local_port, mode);
            break;
        default:
            return false;
    }
    strcat(cmd, buf);
    switch (mode)
    {
        case BUFFER_MODE:
        case DIRECT_PUSH_MODE:
            if (sendAndSearch_also(cmd, OPEN_SOCKET, RESPONSE_ERROR, 150) > 0) {
                unsigned long start_time = millis();
                while (millis() - start_time < 200UL) {
                    if (serialAvailable()) {
                        readResponseByteToBuffer();
                    }
                }
                errorCode = -1;
                char *sta_buf = searchChrBuffer(',');
                if (0 == atoi(sta_buf + 1)) {
                    return true;
                } else {
                    errorCode = atoi(sta_buf + 1);
                }
            }
            break;
        case TRANSPARENT_MODE:
            if (sendAndSearch_also(cmd, RESPONSE_CONNECT, RESPONSE_ERROR, 150) > 0) {
                return true;
            }
            break;
        default:
            break;
    }
    return false;
}

bool CloseSocketService(unsigned int socket_index)
{
    char cmd[16] = "";
    char buf[8] = "";

    strcpy(cmd, CLOSE_SOCKET);
    sprintf(buf, "=%d", socket_index);
    if (sendAndSearch_also(cmd, RESPONSE_OK, RESPONSE_ERROR, 150)) {
        return true;
    }
    return false;
}

bool SocketSendData(unsigned int socket_index, Socket_Type_t socket, char *data_buf, char *ip, unsigned int port)
{
    char cmd[64] = "";
    char buf[64] = "";

    strcpy(cmd, SOCKET_SEND_DATA);
    switch (socket)
    {
        case TCP_CLIENT:
        case TCP_SEVER:
        case UDP_CLIENT:
            sprintf(buf, "=%d,%d", socket_index, strlen(data_buf));
            break;
        case UDP_SEVER:
            sprintf(buf, "=%d,%d,\"%s\",%d", socket_index, strlen(data_buf), ip, port);
            break;
        default:
            return false;
    }
    strcat(cmd, buf);
    if (sendAndSearchChr(cmd, '>', 2)) {
        if (sendDataAndCheck(data_buf, RESPONSE_SEND_OK, RESPONSE_SEND_FAIL, 10)) {
            return true;
        }
    }
    return false;
}

bool SocketRecvData(unsigned int socket_index, unsigned int recv_len, Socket_Type_t socket, char *recv_buf)
{
    char cmd[16] = "";
    char buf[16] = "";

    strcpy(cmd, SOCKET_READ_DATA);
    switch (socket)
    {
        case TCP_CLIENT:
        case TCP_SEVER:
        case UDP_CLIENT:
            sprintf(buf, "=%d,%d", socket_index, recv_len);
            break;
        case UDP_SEVER:
            sprintf(buf, "=%d", socket_index);
            break;
        default:
            return false;
    }
    strcat(cmd, buf);
    if (sendAndSearch_also(cmd, RESPONSE_CRLF_OK, RESPONSE_ERROR, 10)) {
        char *sta_buf = searchStrBuffer(": "); 
        char *end_buf = searchStrBuffer(RESPONSE_CRLF_OK);
        *end_buf = '\0';
        strcpy(recv_buf, sta_buf + 2);
        return true;
    }
    return false;
}

bool SocketSendHEXData(unsigned int socket_index, char *hex_buf)
{
    char cmd[256] = "";
    char buf[256] = "";

    strcpy(cmd, SOCKET_SEND_HEX_DATA);
    sprintf(buf, "=%d,\"%s\"", socket_index, hex_buf);
    strcat(cmd, buf);
    if (sendAndSearch_also(cmd, RESPONSE_SEND_OK, RESPONSE_SEND_FAIL, 2)) {
        return true;
    }
    return false;
}

bool SwitchAccessModes(unsigned int socket_index, Access_Mode_t mode)
{
    char cmd[16] = "";
    char buf[16] = "";

    strcpy(cmd, DATA_ACCESS_MODES);
    sprintf(buf, "=%d,%d", socket_index, mode);
    strcat(cmd, buf);
    if (mode == TRANSPARENT_MODE) {
        if (sendAndSearch_also(cmd, RESPONSE_CONNECT, RESPONSE_ERROR, 2)) {
            return true;
        }
    } else {
        if (sendAndSearch_also(cmd, RESPONSE_OK, RESPONSE_ERROR, 2)) {
            return true;
        }
    }
    return false;
}

bool DevPingFunction(unsigned int socket_index, char *host)
{
    int j = 0;
    char cmd[128] = "";
    char buf[64] = "";

    strcpy(cmd, PING_FUNCTION);
    sprintf(buf, "=%d,\"%s\"", socket_index, host);
    strcat(cmd, buf);
    if (sendAndSearch_also(cmd, RESPONSE_OK, RESPONSE_ERROR, 2)) {
        if (readResponseToBuffer(20)) {
            char *sta_buf = searchStrBuffer(PING_FUNCTION);
            char ping_data[256] = "";
            strcpy(ping_data, sta_buf);
            char *p[6] = {NULL};
            int i = 0;
            p[0] = strtok(ping_data, "\r\n\r\n");
            while (p[i] != NULL) {
                i++;
                p[i] = strtok(NULL, "\r\n\r\n");
            }
            p[i] = '\0';
            for (j = 0; j <= 4; j++) {
                char *cs_buf = strstr(p[j], ": ");
                char *ce_buf = strchr(p[j], ',');
                if (ce_buf == NULL) {
                    errorCode = atoi(cs_buf + 2);
                } else {
                    *ce_buf = '\0';
                    int code = atoi(cs_buf + 2);
                    if (code == 0) {
                        return true;
                    }
                }
            }
        }
    }
    return false;
}

bool DevNTPFunction(unsigned int socket_index, char *ntp_ip, unsigned int port, char *time)
{
    char cmd[64] = "";
    char buf[64] = "";

    strcpy(cmd, NTP_FUNCTION);
    sprintf(buf, "=%d,\"%s\",%d", socket_index, ntp_ip, port);
    strcat(cmd, buf);
    if (sendAndSearch_also(cmd, NTP_FUNCTION, RESPONSE_ERROR, 125)) {
        unsigned long start_time = millis();
        while (millis() - start_time < 200UL) {
            if (serialAvailable()) {
                readResponseByteToBuffer();
            }
        }
        char *sta_buf = searchStrBuffer(": ");
        strcpy(time, sta_buf + 2);
        return true;
    }
    return false;
}

bool ConfigDNSServer(unsigned int socket_index, char *primary_addr, char *secondary_addr)
{
    char cmd[128] = "";
    char buf[128] = "";

    strcpy(cmd, CONFIGURE_DNS_SERVER);
    sprintf(buf, "=%d,\"%s\",\"%s\"", socket_index, primary_addr, secondary_addr);
    strcat(cmd, buf);
    if (sendAndSearch_also(cmd, RESPONSE_OK, RESPONSE_ERROR, 2)) {
        return true;
    }
    return false;
}

bool DevDNSFunction(unsigned int socket_index, char *domain_name, char *ip)
{
    char cmd[128] = "";
    char buf[128] = "";

    strcpy(cmd, DNS_FUNCTION);
    sprintf(buf, "=%d,\"%s\"", socket_index, domain_name);
    strcat(cmd, buf);
    if (sendAndSearch_also(cmd, "+QIURC", RESPONSE_ERROR, 60)) {
        unsigned long start_time = millis();
        while (millis() - start_time < 200UL) {
            if (serialAvailable()) {
                readResponseByteToBuffer();
            }
        }
        char *sta_buf = searchStrBuffer(": ");
        strcpy(ip, sta_buf + 2);
        return true;
    }
    return false;
}

bool QueryLastErrorCode(char *err_code)
{
    char cmd[16] = "";

    strcpy(cmd, QUERY_ERROR_CODE);
    if (sendAndSearch(cmd, RESPONSE_OK, 2)) {
        char *sta_buf = searchStrBuffer(": ");
        strcpy(err_code, sta_buf + 2);
        return true;
    }
    return false;
}

Socket_Event_t WaitCheckSocketEvent(char *event, unsigned int timeout)
{
    if (readResponseAndSearch(RECV_SOCKET_EVENT, timeout)) {
        unsigned long start_time = millis();
        while (millis() - start_time < 200UL) {
            if (serialAvailable()) {
                readResponseByteToBuffer();
            }
        }
        char *sta_buf = searchChrBuffer(',');
        strcpy(event, sta_buf + 1);
        if (searchStrBuffer("closed")) {
            return SOCKET_CLOSE_EVENT;
        } else if (searchStrBuffer("recv")) {
            return SOCKET_RECV_DATA_EVENT;
        } else if (searchStrBuffer("incoming full")) {
            return SOCKET_CONNECTION_FULL_EVENT;
        } else if (searchStrBuffer("incoming")) {
            return SOCKET_INCOMING_CONNECTION_EVENT;
        } else if (searchStrBuffer("pdpdeact")) {
            return SOCKET_PDP_DEACTIVATION_EVENT;
        }
    }
    return 0;
}

/////////////////////////////////// BG96 Serial ///////////////////////////////////
void InitSerial()
{
    cleanBuffer();
}

bool sendDataAndCheck(const char *data_buf, const char *ok_str, const char *err_str, unsigned int timeout)
{
    delay_ms(100);

    // to read recv fifo till empty
    while (ReadFromBG96Serial() >= 0);

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
    while (ReadFromBG96Serial() >= 0);

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
    } else {
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
    int i = 0;

    for (i = 0; i < 3; i++) {
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
    int i = 0;

    for (i = 0; i < 3; i++) {
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
    int i = 0;
    Cmd_Response_t resp_status = UNKNOWN_RESPONSE;

    for (i = 0; i < 3; i++) {
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

/////////////////////////////////// BG96 HTTP ///////////////////////////////////
bool SetHTTPConfigParameters(unsigned int pdp_index, bool request_header, bool response_header, HTTP_Body_Data_Type_t type)
{
    char cmd[64], buf[32];
    strcpy(cmd, HTTP_CONFIG_PARAMETER);
    sprintf(buf, "=\"contextid\",%d", pdp_index);
    strcat(cmd, buf);
    if(!sendAndSearch_also(cmd, RESPONSE_OK, RESPONSE_ERROR, 2)){
        return false;
    }
    memset(cmd, '\0', 64);
    strcpy(cmd, HTTP_CONFIG_PARAMETER);
    if (request_header){
        strcat(cmd, "=\"requestheader\",1");
    }else {
        strcat(cmd, "=\"requestheader\",0");
    }
    if(!sendAndSearch_also(cmd, RESPONSE_OK, RESPONSE_ERROR, 2)){
        return false;
    }
    memset(cmd, '\0', 64);
    strcpy(cmd, HTTP_CONFIG_PARAMETER);
    if (response_header){
        strcat(cmd, "=\"responseheader\",1");
    }else {
        strcat(cmd, "=\"responseheader\",0");
    }
    if(!sendAndSearch_also(cmd, RESPONSE_OK, RESPONSE_ERROR, 2)){
        return false;
    }
    memset(cmd, '\0', 64);
    memset(buf, '\0', 32);
    strcpy(cmd, HTTP_CONFIG_PARAMETER);
    sprintf(buf, "=\"contenttype\",%d", type);
    strcat(cmd, buf);
    if(!sendAndSearch_also(cmd, RESPONSE_OK, RESPONSE_ERROR, 2)){
        return false;
    }
    return true;
}

bool SetHTTPEnableSSL(unsigned int ssl_index)
{
    char cmd[64], buf[32];
    strcpy(cmd, HTTP_CONFIG_PARAMETER);
    sprintf(buf, "=\"sslctxid\",%d", ssl_index);
    strcat(cmd, buf);
    if(sendAndSearch_also(cmd, RESPONSE_OK, RESPONSE_ERROR, 2)){
        return true;
    }
    return false;
}

bool HTTPURL(char *url, Cmd_Status_t status)
{
    char cmd[32],buf[16];
    strcpy(cmd, HTTP_SET_URL);
    if(status == READ_MODE){
        strcat(cmd, "?");
        if(sendAndSearch_also(cmd, RESPONSE_OK, RESPONSE_ERROR, 2)){
            char *sta_buf = searchStrBuffer(": ");
            if(sta_buf == NULL){
                url = "";
            }else {
                char *end_buf = searchStrBuffer("\r\nOK");
                *end_buf = '\0';
                strcpy(url, sta_buf + 2);
            }
            return true;
        }
    }else if (status == WRITE_MODE){
        sprintf(buf, "=%d", strlen(url));
        strcat(cmd, buf);
        if(sendAndSearch_also(cmd, RESPONSE_CONNECT, RESPONSE_ERROR, 10)){
            if(sendDataAndCheck(url, RESPONSE_OK, RESPONSE_ERROR, 60)){
                return true;
            }
        }
    }
    return false;
}

bool HTTPGET(unsigned int timeout)
{
    char cmd[32], buf[16];
    strcpy(cmd, HTTP_GET_REQUEST);
    sprintf(buf, "=%d", timeout);
    strcat(cmd, buf);
    if(sendAndSearch_also(cmd, HTTP_GET_REQUEST, RESPONSE_ERROR, timeout)){
        unsigned long start_time = millis();
        errorCode = -1;
        while(millis() - start_time < 200UL){
            if(serialAvailable()){
                readResponseByteToBuffer();
            }
        }
        char *sta_buf = searchStrBuffer(": ");
        char *end_buf = searchChrBuffer(',');
        *end_buf = '\0';
        if (atoi(sta_buf + 2) == 0) {
            return true;
        }else {
            errorCode = atoi(sta_buf + 2);
        }
    }
    return false;
}

bool HTTPPOST(char *post_data, unsigned int timeout)
{
    char cmd[32], buf[16];
    strcpy(cmd, HTTP_POST_REQUEST);
    sprintf(buf, "=%d,%d,%d", strlen(post_data), timeout, timeout);
    strcat(cmd, buf);
    if(sendAndSearch_also(cmd, RESPONSE_CONNECT, RESPONSE_ERROR, 10)){
        if(sendDataAndCheck(post_data, HTTP_POST_REQUEST, RESPONSE_ERROR, timeout)){
            unsigned long start_time = millis();
            errorCode = -1;
            while(millis() - start_time < 200UL){
                if(serialAvailable()){
                    readResponseByteToBuffer();
                }
            }
            char *sta_buf = searchStrBuffer(": ");
            char *end_buf = searchChrBuffer(',');
            *end_buf = '\0';
            if (atoi(sta_buf + 2) == 0) {
                return true;
            }else {
                errorCode = atoi(sta_buf + 2);
            }
        }
    }
    return false;
}

bool HTTTPPOSTFile(char *filename, unsigned int timeout)
{
    char cmd[64], buf[32];
    strcpy(cmd, HTTP_FILE_POST_REQUEST);
    sprintf(buf, "=\"%s\",%d", filename, timeout);
    strcat(cmd, buf);
    if(sendAndSearch_also(cmd, HTTP_FILE_POST_REQUEST, RESPONSE_ERROR, timeout)){
        unsigned long start_time = millis();
        errorCode = -1;
        while(millis() - start_time < 200UL){
            if(serialAvailable()){
                readResponseByteToBuffer();
            }
        }
        char *sta_buf = searchStrBuffer(": ");
        char *end_buf = searchChrBuffer(',');
        *end_buf = '\0';
        if (atoi(sta_buf + 2) == 0) {
            return true;
        }else {
            errorCode = atoi(sta_buf + 2);
        }
    }
    return false;
}

bool HTTPRead(char *read_data, unsigned int timeout)
{
    char cmd[32], buf[16];
    strcpy(cmd, HTTP_READ_RESPONSE);
    sprintf(buf, "=%d", timeout);
    strcat(cmd, buf);
    if(sendAndSearch_also(cmd, HTTP_READ_RESPONSE, RESPONSE_ERROR, timeout)){
        unsigned long start_time = millis();
        errorCode = -1;
        while(millis() - start_time < 200UL){
            if(serialAvailable()){
                readResponseByteToBuffer();
            }
        }
        char *sta_buf = searchStrBuffer(RESPONSE_CONNECT);
        char *end_buf = searchStrBuffer("OK\r\n\r\n");
        *end_buf = '\0';
        strcpy(read_data, sta_buf + strlen(RESPONSE_CONNECT) + 2);
        return true;
    }
    return false;
}

bool HTTPReadToFile(char *filename, unsigned int timeout)
{
    char cmd[64], buf[32];
    strcpy(cmd, HTTP_FILE_READ_RESPONSE);
    sprintf(buf, "=\"%s\",%d", filename, timeout);
    strcat(cmd, buf);
    if(sendAndSearch_also(cmd, HTTP_FILE_READ_RESPONSE, RESPONSE_ERROR, timeout)){
        unsigned long start_time = millis();
        errorCode = -1;
        while(millis() - start_time < 200UL){
            if(serialAvailable()){
                readResponseByteToBuffer();
            }
        }
        char *sta_buf = searchStrBuffer(": ");
        if(atoi(sta_buf + 2) == 0){
            return true;
        }else {
            errorCode = atoi(sta_buf + 2);
        }
    }
    return false;
}

/////////////////////////////////// BG96 GNSS ///////////////////////////////////
bool SetGNSSConstellation(GNSS_Constellation_t constellation)
{
    char cmd[32], buf[32];
    strcpy(cmd, GNSS_CONFIGURATION);
    sprintf(buf, "=\"gnssconfig\",%d", constellation);
    strcat(cmd, buf);
    if(sendAndSearch_also(cmd, RESPONSE_OK, RESPONSE_ERROR, 2)){
        return true;
    }
    return false;
}

bool SetGNSSAutoRun(bool auto_run)
{
    char cmd[32];
    strcpy(cmd, GNSS_CONFIGURATION);
    if(auto_run){
        strcat(cmd, "\"autogps\",1");
        if(sendAndSearch_also(cmd, RESPONSE_OK, RESPONSE_ERROR, 2)){
            return true;
        }
    }else {
        strcat(cmd, "\"autogps\",0");
        if(sendAndSearch_also(cmd, RESPONSE_OK, RESPONSE_ERROR, 2)){
            return true;
        }
    }
    return false;
}

bool SetGNSSEnableNMEASentences(bool enable)
{
    char cmd[32];
    strcpy(cmd, GNSS_CONFIGURATION);
    if (enable){
        strcat(cmd, "=\"nmeasrc\",1");
    }else {
        strcat(cmd, "=\"nmeasrc\",0");
    }
    if(sendAndSearch_also(cmd, RESPONSE_OK, RESPONSE_ERROR, 2)){
        return true;
    }
    return false;
}

bool TurnOnGNSS(GNSS_Work_Mode_t mode, Cmd_Status_t status)
{
    char cmd[16],buf[8];
    strcpy(cmd, GNSS_TURN_ON);
    if (status == READ_MODE){
        strcat(cmd, "?");
        if(sendAndSearch_also(cmd, RESPONSE_OK, RESPONSE_ERROR, 2)){
            char *sta_buf = searchStrBuffer(": ");
            char *end_buf = searchStrBuffer(RESPONSE_CRLF_OK);
            *end_buf = '\0';
            if(atoi(sta_buf + 2) == 1){
                return true;
            }
        }
    }else if (status == WRITE_MODE){
        sprintf(buf, "=%d", mode);
        strcat(cmd, buf);
        if(sendAndSearch_also(cmd, RESPONSE_OK, RESPONSE_ERROR, 10)){
            return true;
        }
    }
    return false;
}

bool TurnOffGNSS()
{
    char cmd[16];
    strcpy(cmd, GNSS_TURN_OFF);
    if (sendAndSearch_also(cmd, RESPONSE_OK, RESPONSE_ERROR, 10)){
        return true;
    }
    return false;
}

bool GetGNSSPositionInformation(char *position)
{
    char cmd[16];
    strcpy(cmd, GNSS_GET_POSITION);
    strcat(cmd, "=2");
    if(sendAndSearch_also(cmd, RESPONSE_OK, RESPONSE_ERROR, 10)){
        char *end_buf = searchStrBuffer(RESPONSE_CRLF_OK);
        *end_buf = '\0';
        char *sta_buf = searchStrBuffer(": ");
        strcpy(position, sta_buf + 2);
        return true;
    }
    return false;
}

bool GetGNSSNMEASentences(NMEA_Type_t type, char *sentences)
{
    char cmd[32];
    strcpy(cmd, GNSS_ACQUIRE_NMEA);
    switch(type)
    {
        case GPGGA:
            strcat(cmd, "=\"GCA\"");
            break;
        case GPRMC:
            strcat(cmd, "=\"RMC\"");
            break;
        case GPGSV:
            strcat(cmd, "=\"GSV\"");
            break;
        case GPGSA:
            strcat(cmd, "=\"GSA\"");
            break;
        case GPVTG:
            strcat(cmd, "=\"VTG\"");
            break;
        default:
            return false;
    }
    if(sendAndSearch_also(cmd, RESPONSE_OK, RESPONSE_ERROR, 10)){
        char *sta_buf = searchStrBuffer(": ");
        char *end_buf = searchStrBuffer(RESPONSE_CRLF_OK);
        *end_buf = '\0';
        strcpy(sentences, sta_buf + 2);
        return true;
    }
    return false;
}

/////////////////////////////////// BG96 FILE ///////////////////////////////////
bool GetFliesSpace(unsigned long *free_bytes, unsigned long *total_bytes)
{
    char cmd[16]; 
    strcpy(cmd,FILE_SPACE_INFORMATION);
    strcat(cmd,"=\"UFS\"");

    if(sendAndSearch_also(cmd, RESPONSE_OK, RESPONSE_ERROR, 2)){
        char *sta_buf = searchStrBuffer(": ");
        char *med_buf = searchChrBuffer(',');
        char *end_buf = searchStrBuffer(RESPONSE_CRLF_OK);
        *med_buf = '\0';
        *end_buf = '\0';
        *free_bytes = atol(sta_buf + 2);
        *total_bytes = atol(med_buf + 1);
        return true;
    }
    return false;
}

bool GetFilesList(char *list)
{
    char cmd[16];
    strcpy(cmd, FILE_LIST_FILES);
    if(sendAndSearch_also(cmd, RESPONSE_OK, RESPONSE_ERROR, 2)){
        char *sta_buf = searchStrBuffer(FILE_LIST_FILES);
        char *end_buf = searchStrBuffer(RESPONSE_CRLF_OK);
        *end_buf = '\0';
        strcpy(list, sta_buf);
        return true;
    }
    return false;
}

bool DeleteFiles(char *filename)
{
    char cmd[32],buf[32];
    strcpy(cmd, FILE_DELETE_FILES);
    sprintf(buf, "=\"%s\"", filename);
    strcat(cmd, buf);
    if(sendAndSearch_also(cmd, RESPONSE_OK, RESPONSE_ERROR, 2)){
        return true;
    }
    return false;
}

bool UploadFiles(char *filename, char *u_file)
{
    char cmd[32],buf[32];
    strcpy(cmd, FILE_UPLOAD_FILES);
    sprintf(buf, "=\"%s\",%d", filename, strlen(u_file));
    strcat(cmd, buf);
    if(sendAndSearch_also(cmd, RESPONSE_CONNECT, RESPONSE_ERROR, 5)){
        if(sendDataAndCheck(u_file, RESPONSE_OK, RESPONSE_ERROR, 10)){
            return true;
        }
    }
    return false;
}

bool DownloadFiles(char *filename, char *d_file)
{
    char cmd[32],buf[32];
    strcpy(cmd, FILE_DOWNLOAD_FILE);
    sprintf(buf, "=\"%s\"", filename);
    strcat(cmd, buf);
    if(sendAndSearch_also(cmd, RESPONSE_OK, RESPONSE_ERROR, 30)){
        char *end_buf = searchStrBuffer(FILE_DOWNLOAD_FILE);
        *end_buf = '\0';
        char *sta_buf = searchStrBuffer(RESPONSE_CONNECT);
        strcpy(d_file, sta_buf + strlen(RESPONSE_CONNECT) + 2);
        return true;
    }
    return false;
}

bool OpenFile(char *filename, Open_File_Mode_t mode, unsigned int *file_index)
{
    char cmd[32],buf[32];
    strcpy(cmd, FILE_OPEN_FILE);
    sprintf(buf, "=\"%s\",%d", filename, mode);
    strcat(cmd, buf);
    if(sendAndSearch_also(cmd, RESPONSE_OK, RESPONSE_ERROR, 2)){
        char *sta_buf = searchStrBuffer(": ");
        char *end_buf = searchStrBuffer(RESPONSE_CRLF_OK);
        *end_buf = '\0';
        *file_index = atoi(sta_buf + 2);
        return true;
    }
    return false;
}

bool ReadFile(unsigned int file_index, char *read_data)
{
    char cmd[16],buf[8];
    strcpy(cmd, FILE_READ_FILE);
    sprintf(buf, "=%d", file_index);
    strcat(cmd, buf);
    if(sendAndSearch_also(cmd, RESPONSE_OK, RESPONSE_ERROR, 10)){
        char *sta_buf = searchStrBuffer("\r\n");
        char *end_buf = searchStrBuffer("\r\nOK");
        *end_buf = '\0';
        strcpy(read_data, sta_buf + 2);
        return true;
    }
    return false;
}

bool WriteFile(unsigned int file_index, char *write_data)
{
    char cmd[32],buf[16];
    strcpy(cmd, FILE_WRITE_FILE);
    sprintf(buf, "=%d,%d", file_index, strlen(write_data));
    strcat(cmd, buf);
    if(sendAndSearch_also(cmd, RESPONSE_CONNECT, RESPONSE_ERROR, 5)){
        if(sendDataAndCheck(write_data, RESPONSE_OK, RESPONSE_ERROR, 10)){
            return true;
        }
    }
    return false;
}

bool CloseFlie(unsigned int file_index)
{
    char cmd[16],buf[8];
    strcpy(cmd, FILE_CLOSE_FILE);
    sprintf(buf, "=%d", file_index);
    strcat(cmd, buf);
    if(sendAndSearch_also(cmd, RESPONSE_OK, RESPONSE_ERROR, 5)){
        return true;
    }
    return false;
}

bool SetFilePointer(unsigned int file_index, unsigned int offset, Pointer_Mode_t p_mode)
{
    char cmd[32],buf[16];
    strcpy(cmd, FILE_SET_POSITION_FILE);
    sprintf(buf, "=%d,%d,%d", file_index, offset, p_mode);
    strcat(cmd, buf);
    if(sendAndSearch_also(cmd, RESPONSE_OK, RESPONSE_ERROR, 5)){
        return true;
    }
    return false;
}

bool GetFilePointer(unsigned int file_index, unsigned int *offset)
{
    char cmd[32],buf[16];
    strcpy(cmd, FILE_GET_POSITION_FILE);
    sprintf(buf, "=%d", file_index);
    strcat(cmd, buf);
    if(sendAndSearch_also(cmd, RESPONSE_OK, RESPONSE_ERROR, 5)){
        char *sta_buf = searchStrBuffer(": ");
        char *end_buf = searchStrBuffer(RESPONSE_CRLF_OK);
        *end_buf = '\0';
        *offset = atoi(sta_buf + 2);
        return true;
    }
    return false;
}

bool TruncateFile(unsigned int file_index)
{
    char cmd[16],buf[8];
    strcpy(cmd, FILE_TRUNCATE_FILE);
    sprintf(buf, "=%d", file_index);
    strcat(cmd, buf);
    if(sendAndSearch_also(cmd, RESPONSE_OK, RESPONSE_ERROR, 5)){
        return true;
    }
    return false;
}

/////////////////////////////////// BG96 FILE ///////////////////////////////////
bool SetSSLParameters(unsigned int ssl_index, SSL_Version_t s_version, SSL_Cipher_Suites_t s_ciper, unsigned int negotiation_time)
{
    char cmd[64],buf[32];
    strcpy(cmd, SSL_CONFIG_PARAMETER);
    sprintf(buf, "=\"sslversion\",%d,%d", ssl_index, s_version);
    strcat(cmd, buf);
    if(sendAndSearch_also(cmd, RESPONSE_OK, RESPONSE_ERROR, 5)){
        memset(cmd, '\0', 64);
        memset(buf, '\0', 32);
        strcpy(cmd, SSL_CONFIG_PARAMETER);
        switch (s_ciper)
        {
            case TLS_RSA_WITH_AES_256_CBC_SHA:
                sprintf(buf, "=\"ciphersuite\",%d,0X0035", ssl_index);
                break;
            case TLS_RSA_WITH_AES_128_CBC_SHA:
                sprintf(buf, "=\"ciphersuite\",%d,0X002F", ssl_index);
                break;
            case TLS_RSA_WITH_RC4_128_SHA:
                sprintf(buf, "=\"ciphersuite\",%d,0X0005", ssl_index);
                break;
            case TLS_RSA_WITH_RC4_128_MD5:
                sprintf(buf, "=\"ciphersuite\",%d,0X0004", ssl_index);
                break;
            case TLS_RSA_WITH_3DES_EDE_CBC_SHA:
                sprintf(buf, "=\"ciphersuite\",%d,0X000A", ssl_index);
                break;
            case TLS_RSA_WITH_AES_256_CBC_SHA256:
                sprintf(buf, "=\"ciphersuite\",%d,0X003D", ssl_index);
                break;
            case TLS_ECDHE_RSA_WITH_RC4_128_SHA:
                sprintf(buf, "=\"ciphersuite\",%d,0XC011", ssl_index);
                break;
            case TLS_ECDHE_RSA_WITH_3DES_EDE_CBC_SHA:
                sprintf(buf, "=\"ciphersuite\",%d,0XC012", ssl_index);
                break;
            case TLS_ECDHE_RSA_WITH_AES_128_CBC_SHA:
                sprintf(buf, "=\"ciphersuite\",%d,0XC013", ssl_index);
                break;
            case TLS_ECDHE_RSA_WITH_AES_256_CBC_SHA:
                sprintf(buf, "=\"ciphersuite\",%d,0XC014", ssl_index);
                break;
            case TLS_ECDHE_RSA_WITH_AES_128_CBC_SHA256:
                sprintf(buf, "=\"ciphersuite\",%d,0C027", ssl_index);
                break;
            case TLS_ECDHE_RSA_WITH_AES_256_CBC_SHA384:
                sprintf(buf, "=\"ciphersuite\",%d,0XC028", ssl_index);
                break;
            case TLS_ECDHE_RSA_WITH_AES_128_GCM_SHA256:
                sprintf(buf, "=\"ciphersuite\",%d,0XC02F", ssl_index);
                break;
            default:
                sprintf(buf, "=\"ciphersuite\",%d,0XFFFF", ssl_index);
                break;
        }
        strcat(cmd, buf);
        if(sendAndSearch_also(cmd, RESPONSE_OK, RESPONSE_ERROR, 5)){
            memset(cmd, '\0', 64);
            memset(buf, '\0', 32);
            strcpy(cmd, SSL_CONFIG_PARAMETER);
            sprintf(buf, "=\"negotiatetime\",%d,%d", ssl_index, negotiation_time);
            strcat(cmd, buf);
            if(sendAndSearch_also(cmd, RESPONSE_OK, RESPONSE_ERROR, 5)){
                return true;
            }
        }
    }
    return false;
}

bool SetSSLCertificate(unsigned int ssl_index, char *ca_cert_path, char *client_cert_path, char *client_key_path, bool validity_check)
{
    char cmd[64],buf[32];
    strcpy(cmd, SSL_CONFIG_PARAMETER);
    if(ca_cert_path == "" && client_cert_path == "" && client_key_path == ""){
        sprintf(buf, "=\"seclevel\",%d,0", ssl_index);
        strcat(cmd, buf);
        if(sendAndSearch_also(cmd, RESPONSE_OK, RESPONSE_ERROR, 5)){
            return true;
        }
    }else if(ca_cert_path != "" && client_cert_path == "" && client_key_path == ""){
        sprintf(buf, "=\"seclevel\",%d,1", ssl_index);
        strcat(cmd, buf);
        if(!sendAndSearch_also(cmd, RESPONSE_OK, RESPONSE_ERROR, 5)){
            return false;
        }
        memset(cmd, '\0', 64);
        memset(buf, '\0', 32);
        strcpy(cmd, SSL_CONFIG_PARAMETER);
        sprintf(buf, "=\"cacert\",%d,\"%s\"", ssl_index, ca_cert_path);
        strcat(cmd, buf);
        if(!sendAndSearch_also(cmd, RESPONSE_OK, RESPONSE_ERROR, 5)){
            return false;
        }
        memset(cmd, '\0', 64);
        memset(buf, '\0', 32);
        strcpy(cmd, SSL_CONFIG_PARAMETER);
        if(validity_check){
            sprintf(buf, "=\"ignorelocaltime\",%d,1", ssl_index);
            strcat(cmd, buf);
            if(sendAndSearch_also(cmd, RESPONSE_OK, RESPONSE_ERROR, 5)){
                return true;
            }
        }else{
            sprintf(buf, "=\"ignorelocaltime\",%d,0", ssl_index);
            strcat(cmd, buf);
            if(sendAndSearch_also(cmd, RESPONSE_OK, RESPONSE_ERROR, 5)){
                return true;
            }
        }
            
    }else if(ca_cert_path != "" && client_cert_path != "" && client_key_path != ""){
        sprintf(buf, "=\"seclevel\",%d,2", ssl_index);
        strcat(cmd, buf);
        if(!sendAndSearch_also(cmd, RESPONSE_OK, RESPONSE_ERROR, 5)){
            return false;
        }
        memset(cmd, '\0', 64);
        memset(buf, '\0', 32);
        strcpy(cmd, SSL_CONFIG_PARAMETER);
        sprintf(buf, "=\"cacert\",%d,\"%s\"", ssl_index, ca_cert_path);
        strcat(cmd, buf);
        if(!sendAndSearch_also(cmd, RESPONSE_OK, RESPONSE_ERROR, 5)){
            return false;
        }
        memset(cmd, '\0', 64);
        memset(buf, '\0', 32);
        strcpy(cmd, SSL_CONFIG_PARAMETER);
        sprintf(buf, "=\"clientcert\",%d,\"%s\"", ssl_index, client_cert_path);
        strcat(cmd, buf);
        if(!sendAndSearch_also(cmd, RESPONSE_OK, RESPONSE_ERROR, 5)){
            return false;
        }
        memset(cmd, '\0', 64);
        memset(buf, '\0', 32);
        strcpy(cmd, SSL_CONFIG_PARAMETER);
        sprintf(buf, "=\"clientkey\",%d,\"%s\"", ssl_index, client_key_path);
        strcat(cmd, buf);
        if(!sendAndSearch_also(cmd, RESPONSE_OK, RESPONSE_ERROR, 5)){
            return false;
        }
        memset(cmd, '\0', 64);
        memset(buf, '\0', 32);
        strcpy(cmd, SSL_CONFIG_PARAMETER);
        if(validity_check){
            sprintf(buf, "=\"ignorelocaltime\",%d,1", ssl_index);
            strcat(cmd, buf);
            if(sendAndSearch_also(cmd, RESPONSE_OK, RESPONSE_ERROR, 5)){
                return true;
            }
        }else{
            sprintf(buf, "=\"ignorelocaltime\",%d,0", ssl_index);
            strcat(cmd, buf);
            if(sendAndSearch_also(cmd, RESPONSE_OK, RESPONSE_ERROR, 5)){
                return true;
            }
        }
    }
    return false;
}

bool InitSSL(unsigned int ssl_index, char *ca_cert, char *client_cert, char *client_key, char *err_code)
{
    unsigned long start_time = millis();
    const char *e_str;
    int f_err_code;
    if (!SetSSLParameters(ssl_index, ALL_VERSION, SUPPORT_ALL_ABOVE, 300)){
        e_str = "\r\nSSL ERROR: An error occurred while setting the ssl parameter.\r\n";
        strcpy(err_code, e_str);
        return false;
    }

    if (ca_cert == "" && client_cert == "" && client_key == ""){
        if (SetSSLCertificate(ssl_index, "", "", "", false)){
            e_str = "\r\nSSL OK: The ssl were successfully initialized.\r\n";
            strcpy(err_code, e_str);
            return true;
        }
        e_str = "\r\nSSL ERROR: An error occurred while setting the ssl certificate.\r\n";
        strcpy(err_code, e_str);
    }else if (ca_cert != "" && client_cert == "" && client_key == ""){
        while (!UploadFiles(ssl_ca_cert_name, ca_cert)){
            if(returnErrorCode(f_err_code)){
                if (f_err_code == 407){
                    start_time = millis();
                    while (!DeleteFiles(ssl_ca_cert_name)){
                        if(millis() - start_time >= 10*1000UL){
                            e_str = "\r\nSSL ERROR: The ssl ca cert file exists. An error occurred while deleting the original file during the re-upload process.\r\n";
                            strcpy(err_code, e_str);
                            return false;
                        }
                    }
                }
            }else if(millis() - start_time >= 30*1000UL){
                sprintf(e_str, "\r\nSSL ERROR: Error uploading file, error code: %d ,Please check the corresponding documentation for details.\r\n", f_err_code);
                strcpy(err_code, e_str);
                return false;
            }
        }
        start_time = millis();
        while (!SetSSLCertificate(ssl_index, ssl_ca_cert_name, "", "", false)){
            if (millis() - start_time >= 30 * 1000UL){
                e_str = "\r\nSSL ERROR: An error occurred while setting the ssl certificate.\r\n";
                strcpy(err_code, e_str);
                return false;
            }
        }
        e_str = "\r\nSSL OK: The ssl were successfully initialized.\r\n";
        strcpy(err_code, e_str);
        return true;
    }else if (ca_cert != "" && client_cert != "" && client_key != ""){
        while (!UploadFiles(ssl_ca_cert_name, ca_cert)){
            if(returnErrorCode(f_err_code)){
                if (f_err_code == 407){
                    start_time = millis();
                    while (!DeleteFiles(ssl_ca_cert_name)){
                        if(millis() - start_time >= 10*1000UL){
                            e_str = "\r\nSSL ERROR: The ssl ca cert file exists. An error occurred while deleting the original file during the re-upload process.\r\n";
                            strcpy(err_code, e_str);
                            return false;
                        }
                    }
                }
            }else if(millis() - start_time >= 30*1000UL){
                sprintf(e_str, "\r\nSSL ERROR: Error uploading file, error code: %d ,Please check the corresponding documentation for details.\r\n", f_err_code);
                strcpy(err_code, e_str);
                return false;
            }
        }
        start_time = millis();
        while (!UploadFiles(ssl_client_cert_name, client_cert)){
            if(returnErrorCode(f_err_code)){
                if (f_err_code == 407){
                    start_time = millis();
                    while (!DeleteFiles(ssl_client_cert_name)){
                        if(millis() - start_time >= 10*1000UL){
                            e_str = "\r\nSSL ERROR: The ssl ca cert file exists. An error occurred while deleting the original file during the re-upload process.\r\n";
                            strcpy(err_code, e_str);
                            return false;
                        }
                    }
                }
            }else if(millis() - start_time >= 30*1000UL){
                sprintf(e_str, "\r\nSSL ERROR: Error uploading file, error code: %d ,Please check the corresponding documentation for details.\r\n", f_err_code);
                strcpy(err_code, e_str);
                return false;
            }
        }
        start_time = millis();
        while (!UploadFiles(ssl_client_key_name, client_key)){
            if(returnErrorCode(f_err_code)){
                if (f_err_code == 407){
                    start_time = millis();
                    while (!DeleteFiles(ssl_client_key_name)){
                        if(millis() - start_time >= 10*1000UL){
                            e_str = "\r\nSSL ERROR: The ssl ca cert file exists. An error occurred while deleting the original file during the re-upload process.\r\n";
                            strcpy(err_code, e_str);
                            return false;
                        }
                    }
                }
            }else if(millis() - start_time >= 30*1000UL){
                sprintf(e_str, "\r\nSSL ERROR: Error uploading file, error code: %d ,Please check the corresponding documentation for details.\r\n", f_err_code);
                strcpy(err_code, e_str);
                return false;
            }
        }
        start_time = millis();
        while (!SetSSLCertificate(ssl_index, ssl_ca_cert_name, ssl_client_cert_name, ssl_client_key_name, false)){
            if (millis() - start_time >= 30 * 1000UL){
                e_str = "\r\nSSL ERROR: An error occurred while setting the ssl certificate.\r\n";
                strcpy(err_code, e_str);
                return false;
            }
        }
        e_str = "\r\nSSL OK: The ssl were successfully initialized.\r\n";
        strcpy(err_code, e_str);
        return true;
    }
    return false;
}

bool OpenSSLSocket(unsigned int pdp_index, unsigned int ssl_index, unsigned int socket_index, char *ip, unsigned int port, Access_Mode_t mode)
{
    char cmd[64],buf[64];
    strcpy(cmd, SSL_OPEN_SOCKET);
    sprintf(buf, "=%d,%d,%d,\"%s\",%d,%d", pdp_index, ssl_index, socket_index, ip, port, mode);
    strcat(cmd, buf);
    switch(mode)
    {
        case BUFFER_MODE:
        case DIRECT_PUSH_MODE:
            if (sendAndSearch_also(cmd, SSL_OPEN_SOCKET, RESPONSE_ERROR, 150) > 0){
                unsigned long start_time = millis();
                while(millis() - start_time < 200UL){
                    if (serialAvailable()){
                        readResponseByteToBuffer();
                    }
                }
                char *sta_buf = searchChrBuffer(',');
                errorCode = -1;
                if(atoi(sta_buf + 1) == 0){
                    return true;
                }else{
                    errorCode = atoi(sta_buf + 1);
                }
            }
            break;
        case TRANSPARENT_MODE:
            if (sendAndSearch_also(cmd, RESPONSE_CONNECT, RESPONSE_ERROR, 150) > 0){
                return true;
            }
            break;
        default:
            break;
    }
    return false;
}

bool SSLSocketSendData(unsigned int socket_index, char *ssl_send_data)
{
    char cmd[16], buf[8];
    strcpy(cmd, SSL_SEND_DATA);
    sprintf(buf, "=%d,%d", socket_index, strlen(ssl_send_data));
    strcat(cmd, buf);
    if(sendAndSearchChr(cmd, '>', 2)){
        if(sendDataAndCheck(ssl_send_data, RESPONSE_SEND_OK, RESPONSE_SEND_FAIL, 30)){
            return true;
        }
    }
    return false;
}

bool SSLSocketRecvData(unsigned int socket_index, unsigned int ssl_recv_len, char *ssl_recv_data)
{
    char cmd[16],buf[16];
    strcpy(cmd, SSL_READ_DATA);
    sprintf(buf, "=%d,%d", socket_index, ssl_recv_len);
    strcat(cmd, buf);
    if(sendAndSearch_also(cmd, RESPONSE_CRLF_OK, RESPONSE_ERROR, 30)){
        char *end_buf = searchStrBuffer(RESPONSE_CRLF_OK);
        *end_buf = '\0';
        char *sta_buf = searchStrBuffer(": "); 
        strcpy(ssl_recv_data, sta_buf + 2);
        return true;
    }
    return false;
}

bool CloseSSLSocket(unsigned int socket_index)
{
    char cmd[16],buf[16];
    strcpy(cmd, SSL_CLOSE_SOCKET);
    sprintf(buf, "=%d", socket_index);
    strcat(cmd, buf);
    if(sendAndSearch_also(cmd, RESPONSE_OK, RESPONSE_ERROR, 150)){
        return true;
    }
    return false;
}

bool QuerySSLSocketStatus(unsigned int socket_index, char *ssl_status)
{
    char cmd[16],buf[16];
    strcpy(cmd, SSL_QUERY_STATUS);
    sprintf(buf, "=%d", socket_index);
    strcat(cmd, buf);
    if(sendAndSearch(cmd, RESPONSE_OK, 10)){
        char *sta_buf = searchStrBuffer(": ");
        char *end_buf = searchStrBuffer(RESPONSE_CRLF_OK);
        *end_buf = '\0';
        strcpy(ssl_status, sta_buf + 2);
        return true;
    }
    return false;
}

SSL_Socket_Event_t WaitCheckSSLSocketEvent(char *event, unsigned int timeout)
{
    if(readResponseAndSearch(SSL_SOCKET_EVENT, timeout)){
        unsigned long start_time = millis();
        while(millis() - start_time < 200UL){
            if (serialAvailable()){
                readResponseByteToBuffer();
            }
        }
        char *sta_buf = searchChrBuffer(',');
        strcpy(event, sta_buf + 1);
        if(searchStrBuffer("closed")){
            return SSL_SOCKET_CLOSE_EVENT;
        }else if (searchStrBuffer("recv")){
            return SSL_SOCKET_RECV_EVENT;
        }
    }
    return 0;
}
