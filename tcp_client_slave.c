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

	int mode = 0;
    int length = 0;  
    char buffer[BUFFER_SIZE];  
	while (1) {
		mode++;
#if 1
   		memset(buffer, 0, BUFFER_SIZE);  
		sprintf(buffer, "SET=%d", mode);
	    length = send(client_socket, buffer, strlen(buffer), 0);  

	    length = 0;  
   		memset(buffer, 0, BUFFER_SIZE);  
    	if (length = recv(client_socket, buffer, BUFFER_SIZE, 0))  
	    {  
    	    if (length < 0)  
        	{  
            	printf("Recieve Data From Server %s Failed!\n", argv[1]);  
	            break;  
    	    } else {
				printf("Recved Data(%dB): %s\n", length, buffer);
			}

    		memset(buffer, 0, BUFFER_SIZE);  
	    }  
#endif
		usleep(3000);
	}

    close(client_socket);  

    return 0;  
}
