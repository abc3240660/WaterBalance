#include<netinet/in.h>
#include<sys/types.h>
#include<sys/socket.h>
#include<stdio.h>
#include<stdlib.h>
#include<string.h>

#define SERVER_PORT_PL    6666
#define SERVER_PORT_PS    8888
#define LENGTH_OF_LISTEN_QUEUE     20
#define BUFFER_SIZE                1024
#define FILE_NAME_MAX_SIZE         512

int auto_send = 0;
int port_pl = 6666;
int port_ps = 8888;

pthread_t thread;
pthread_t service_thread1,service_thread2;

void *PL_switch(void)
{
    struct sockaddr_in   server_addr;
    bzero(&server_addr, sizeof(server_addr));
    server_addr.sin_family = AF_INET;
    //server_addr.sin_addr.s_addr = htons(INADDR_ANY);
    server_addr.sin_addr.s_addr = inet_addr("127.0.0.1");
    server_addr.sin_port = htons(port_pl);

    int server_socket = socket(PF_INET, SOCK_STREAM, 0);
    if (server_socket < 0)
    {
        printf("Create Socket Failed!\n");
        exit(1);
    }

	// int flags = fcntl(server_socket, F_GETFL, 0);
	// fcntl(server_socket, F_SETFL, flags|O_NONBLOCK);
    if (bind(server_socket, (struct sockaddr*)&server_addr, sizeof(server_addr)))
    {
        printf("Server Bind Port: %d Failed!\n", SERVER_PORT_PL);
        exit(1);
    }

    if (listen(server_socket, LENGTH_OF_LISTEN_QUEUE))
    {
        printf("Server Listen Failed!\n");
        exit(1);
    }

    while(1)
    {
        struct sockaddr_in client_addr;
        socklen_t          length = sizeof(client_addr);

        int new_server_socket = accept(server_socket, (struct sockaddr*)&client_addr, &length);
        if (new_server_socket < 0)
        {
            printf("Server Accept Failed!\n");
            break;
        }

        printf("client connected!!!\n");

		fd_set rset, wset;
		struct timeval tval;
		tval.tv_sec = 0;
		tval.tv_usec = 300000;
        char buffer[BUFFER_SIZE];
		while (1) {
			FD_ZERO(&rset);
			FD_SET(new_server_socket, &rset);
			int ret = select(new_server_socket+1, &rset, NULL, NULL, &tval);
			if (-1 == ret) {
		        printf("select error\n");
				break;
			} else if (0 == ret) {
				usleep(10000);
				continue;
			} else {
		   		memset(buffer, 0, BUFFER_SIZE);  
   	    		length = recv(new_server_socket, buffer, BUFFER_SIZE, 0);

        		if (0 == length) {
					printf("client closed!!!\n", ret);
					break;
				}

        		if (length) {
	        		if (length < 0)
   			    	{
        			    printf("Server Recieve Data Failed!\n");
   		        		break;
		        	}
					printf("Recved Data(%dB): %s\n", length, buffer);
				}

	            length = send(new_server_socket, buffer, length, 0);
				printf("Send Data(%dB): %s\n", length, buffer);
        	    if (length < 0)
				{
        	    	printf("Server Send Data Failed!\n");
	            	break;
				}
			}

			if (auto_send) {
	            length = send(new_server_socket, buffer, length, 0);
				printf("send ret = %d\n", length);
        	    if (length < 0)
				{
        	    	printf("Server Send Data Failed!\n");
	            	break;
				}
			}

			usleep(1000000);
		}

        close(new_server_socket);
	}

    close(server_socket);
}

void *PS_switch(void)
{
    struct sockaddr_in   server_addr;
    bzero(&server_addr, sizeof(server_addr));
    server_addr.sin_family = AF_INET;
    //server_addr.sin_addr.s_addr = htons(INADDR_ANY);
    server_addr.sin_addr.s_addr = inet_addr("127.0.0.1");
    server_addr.sin_port = htons(port_ps);

    int server_socket = socket(PF_INET, SOCK_STREAM, 0);
    if (server_socket < 0)
    {
        printf("Create Socket Failed!\n");
        exit(1);
    }

	// int flags = fcntl(server_socket, F_GETFL, 0);
	// fcntl(server_socket, F_SETFL, flags|O_NONBLOCK);
    if (bind(server_socket, (struct sockaddr*)&server_addr, sizeof(server_addr)))
    {
        printf("Server Bind Port: %d Failed!\n", SERVER_PORT_PL);
        exit(1);
    }

    if (listen(server_socket, LENGTH_OF_LISTEN_QUEUE))
    {
        printf("Server Listen Failed!\n");
        exit(1);
    }

    while(1)
    {
        struct sockaddr_in client_addr;
        socklen_t          length = sizeof(client_addr);

        int new_server_socket = accept(server_socket, (struct sockaddr*)&client_addr, &length);
        if (new_server_socket < 0)
        {
            printf("Server Accept Failed!\n");
            break;
        }

        printf("client connected!!!\n");

		fd_set rset, wset;
		struct timeval tval;
		tval.tv_sec = 0;
		tval.tv_usec = 300000;
        char buffer[BUFFER_SIZE];
		while (1) {
			FD_ZERO(&rset);
			FD_SET(new_server_socket, &rset);
			int ret = select(new_server_socket+1, &rset, NULL, NULL, &tval);
			if (-1 == ret) {
		        printf("select error\n");
				break;
			} else if (0 == ret) {
				usleep(10000);
				continue;
			} else {
		   		memset(buffer, 0, BUFFER_SIZE);  
   	    		length = recv(new_server_socket, buffer, BUFFER_SIZE, 0);

        		if (0 == length) {
					printf("client closed!!!\n", ret);
					break;
				}

        		if (length) {
	        		if (length < 0)
   			    	{
        			    printf("Server Recieve Data Failed!\n");
   		        		break;
		        	}
					printf("Recved Data(%dB): %s\n", length, buffer);
				}

	            length = send(new_server_socket, buffer, length, 0);
				printf("Send Data(%dB): %s\n", length, buffer);
        	    if (length < 0)
				{
        	    	printf("Server Send Data Failed!\n");
	            	break;
				}
			}

			usleep(1000000);
		}

        close(new_server_socket);
	}

    close(server_socket);
}

int main(int argc, char **argv)
{
    int result = 0;

	if (argc >= 2) {
		port_pl = atoi(argv[1]);
	}
	if (argc >= 3) {
		port_ps = atoi(argv[2]);
	}

	result = pthread_create(&service_thread1, NULL, PL_switch, NULL);
	if(result != 0)
    {
	    fprintf(stderr, "Could not create thread!\n");
	    exit(1);
    }
    result = pthread_create(&service_thread2, NULL, PS_switch, NULL);
    if(result != 0)
    {
	    fprintf(stderr, "Could not create thread!\n");
	    exit(1);
    }
    /*----------------------------------*/
    /*  wait till the threads are done  */
    /*----------------------------------*/

    pthread_join(service_thread1, NULL);
    pthread_join(service_thread2, NULL);

    return 0;
}
