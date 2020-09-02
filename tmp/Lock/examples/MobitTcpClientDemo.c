#include "MobitLTEBG96TCPIP.h"

const char APN[] = "hologram";
const char tcp_ip[] = "mbed.org";
const int tcp_port = 80;
const char send_data[] = "GET /media/uploads/mbed_official/hello.txt HTTP/1.0\r\n\r\n";
unsigned int comm_pdp_index = 2;  // The range is 1 ~ 16
unsigned int comm_socket_index = 2;  // The range is 0 ~ 11
Socket_Type_t socket = TCP_CLIENT;

void setup(){
    printf("This is the Mobit Debug Serial!\n");
    delay_ms(1000);
    while(!InitModule());

    SetDevCommandEcho(false);

    char inf[64];
    if(GetDevInformation(inf)){
        printf("Dev Info: %s!\n", inf);
    }

    char apn_error[64];
    while (!InitAPN(comm_pdp_index, APN, "", "", apn_error)){
        printf("apn_error :\n", apn_error);
    }
    printf("apn_error :\n", apn_error);

    while (!OpenSocketService(comm_pdp_index, comm_socket_index, socket, tcp_ip, tcp_port, 0, BUFFER_MODE)){
        printf("Open Socket Service Fail!\n");
    }
    printf("Open Socket Service Success!\n");

    if(SocketSendData(comm_socket_index, socket, send_data, "", "")){
        printf("Socket Send Data Success!\n");
    }
}

void loop(){
    char m_event[16];
    unsigned int index;
    char recv_data[128];
    Socket_Event_t ret = WaitCheckSocketEvent(m_event, 2);
    switch(ret)
    {
        case SOCKET_CLOSE_EVENT:
            index = atoi(m_event);
            if(CloseSocketService(index)){
                printf("Close Socket Success!\n");
            }
            break;
        case SOCKET_RECV_DATA_EVENT:
            index = atoi(m_event);
            if (SocketRecvData(index, 128, socket, recv_data)){
                printf("Socket Recv Data Success!\n");
                printf("recv_data: %s\n", recv_data);
            }
            break;
        case SOCKET_PDP_DEACTIVATION_EVENT:
            index = atoi(m_event);
            if(DeactivateDevAPN(index)){
                printf("Please reconfigure APN!\n");
            }
            break;
        default:
            break;
    }
}

int main(void)
{
    return 0;
}
