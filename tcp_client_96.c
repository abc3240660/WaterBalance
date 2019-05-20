#include<netinet/in.h>                         // for sockaddr_in  
#include<sys/types.h>                          // for socket  
#include<sys/socket.h>                         // for socket  
#include<stdio.h>                              // for printf  
#include<stdlib.h>                             // for exit  
#include<string.h>                             // for bzero  

#define HELLO_WORLD_SERVER_PORT       6666  
#define BUFFER_SIZE                   1024  
#define FILE_NAME_MAX_SIZE            512  

int port_pl = 6666;

int main(int argc, char **argv)
{
    if (argc != 3)
    {
        printf("Usage: ./%s ServerIPAddress\n", argv[0]);
        exit(1);
    }
 
    port_pl = atoi(argv[2]);
    struct sockaddr_in client_addr;
    bzero(&client_addr, sizeof(client_addr));
    client_addr.sin_family = AF_INET;
    client_addr.sin_addr.s_addr = htons(INADDR_ANY);
    client_addr.sin_port = htons(0);

    int client_socket = socket(AF_INET, SOCK_STREAM, 0);
    if (client_socket < 0)
    {
        printf("Create Socket Failed!\n");
        exit(1);
    }

    if (bind(client_socket, (struct sockaddr*)&client_addr, sizeof(client_addr)))
    {
        printf("Client Bind Port Failed!\n");
        exit(1);
    }

    struct sockaddr_in  server_addr;
    bzero(&server_addr, sizeof(server_addr));
    server_addr.sin_family = AF_INET;

    if (inet_aton(argv[1], &server_addr.sin_addr) == 0)
    {
        printf("Server IP Address Error!\n");
        exit(1);
    }

    server_addr.sin_port = htons(port_pl);
    socklen_t server_addr_length = sizeof(server_addr);

    if (connect(client_socket, (struct sockaddr*)&server_addr, server_addr_length) < 0)
    {
        printf("Can Not Connect To %s!\n", argv[1]);
        exit(1);
    }

    int length = 0;
    char temp = 0;
    int rx_len = 0;
    int msg_len = 0;
    char buffer[BUFFER_SIZE] = "";
    char *p = NULL;
    int rd_flag = 0;

    while (1) {
#if 0
        memset(buffer, 0, BUFFER_SIZE);
        strcpy(buffer, "Hello Damon\n");
        length = send(client_socket, buffer, strlen(buffer), 0);
#endif

        length = 0;
        if (length = recv(client_socket, &temp, 1, 0))
        {
            if (length < 0)
            {
                printf("Recieve Data From Server %s Failed!\n", argv[1]);
                break;
            } else {
//                printf("Recved Data(%dB): %.2X\n", length, temp);
#if 0
                for (int i=0; i<length; i++) {
                    printf("%.2X ", buffer[i]);
                }
                printf("\n");
#endif

                // when MSG1 lost tail
                // need next msg(MSG2 head) to complete MSG1
                if ('+' == temp) {
                    if (('\r'==buffer[rx_len-2]) && ('\n'==buffer[rx_len-1])) {// "\r\n+QQQ"
                        printf("recv msg = %s\n", buffer);
                        rd_flag = 0;
                        rx_len = 0;
                        memset(buffer, 0, BUFFER_SIZE);
                    }
                }

                buffer[rx_len++] = temp;
                if (rx_len > 4) {// skip "\r\n" & "\r\n\r\n"
                    if (!rd_flag) {
                        // TODO: will disturb HTTP, because thee same string in HEX.BIN
                        if ((p=strstr(buffer, "+QIRD: "))) {
                            rd_flag = 1;
                            continue;
                        }

                        if (('\r'==buffer[rx_len-2]) && ('\n'==buffer[rx_len-1])) {
                            printf("recv ack = %s\n", buffer);
                            rx_len = 0;
                            memset(buffer, 0, BUFFER_SIZE);
                        }
                    } else {
                        if (!msg_len) {
                            if (('\r'==buffer[rx_len-2]) && ('\n'==buffer[rx_len-1])) {
                                msg_len = atoi(p+7);
                                printf("msg len= %d\n", msg_len);
                            }
                        }

                        // MSG1 complete self
                        if (('O'==buffer[rx_len-2]) && ('K'==buffer[rx_len-1])) {
                            printf("recv msg = %s\n", buffer);
                            rd_flag = 0;
                            rx_len = 0;
                            memset(buffer, 0, BUFFER_SIZE);
                        }
                    }
                }
            }

//            memset(buffer, 0, BUFFER_SIZE);
        }

        usleep(1000);
    }

    close(client_socket);

    return 0;
}
