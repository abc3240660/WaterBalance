#include<netinet/in.h>
#include<sys/types.h>
#include<sys/socket.h>
#include<stdio.h>
#include<stdlib.h>
#include<string.h>
#include <signal.h>

#define SERVER_PORT_PL    88
#define SERVER_PORT_PS    89
#define LENGTH_OF_LISTEN_QUEUE     20
#define BUFFER_SIZE                1024
#define FILE_NAME_MAX_SIZE         512

int auto_send = 0;
int port_pl = 6666;
int port_ps = 8888;

pthread_t thread;
pthread_t service_thread1,service_thread2;

char debug_buffer[BUFFER_SIZE] = "";

void parse_msg(char* msg)
{
	char imei[] = "012345678901234";

	// Recved DEV Reply, SRV Just SKIP
	if (strstr(msg, ",Re,")) {// Dev Auto Register
   		memset(msg, 0, BUFFER_SIZE);  
	} else {
		// Recved DEV CMD, SRV Just Reply
		if (strstr(msg, ",R0,")) {// Dev Auto Register
	   		memset(msg, 0, BUFFER_SIZE);  
			sprintf(msg, "^MOBIT,ECAR,%s,Re,R0,20190115170655,15$\n", imei);
		} else if (strstr(msg, ",H0,")) {// Auto Heartbeat
	   		memset(msg, 0, BUFFER_SIZE);  
			sprintf(msg, "^MOBIT,ECAR,%s,Re,H0$\n", imei);
		} else if (strstr(msg, ",C1,")) {// Dev Auto Door Locked
	   		memset(msg, 0, BUFFER_SIZE);  
			sprintf(msg, "^MOBIT,ECAR,%s,Re,C1$\n", imei);
		} else if (strstr(msg, ",O1,")) {// Dev Auto Door Unlocked
	   		memset(msg, 0, BUFFER_SIZE);  
			sprintf(msg, "^MOBIT,ECAR,%s,Re,O1$\n", imei);
		} else if (strstr(msg, ",C3,")) {// Dev Auto Calypso Upload
	   		memset(msg, 0, BUFFER_SIZE);  
			sprintf(msg, "^MOBIT,ECAR,%s,Re,C3,0$\n", imei);// keep locked
			// sprintf(msg, "^MOBIT,ECAR,%s,Re,C3,1$\n", imei);// do unlock
		} else if (strstr(msg, ",W1,")) {// Dev Auto Invalid Moving
	   		memset(msg, 0, BUFFER_SIZE);  
			sprintf(msg, "^MOBIT,ECAR,%s,Re,W1$\n", imei);
		} else if (strstr(msg, ",L1,")) {// Dev Auto Report GPS
	   		memset(msg, 0, BUFFER_SIZE);  
			sprintf(msg, "^MOBIT,ECAR,%s,Re,L1$\n", imei);
		} else if (strstr(msg, ",U1,")) {// Dev Auto IAP Success
	   		memset(msg, 0, BUFFER_SIZE);  
			sprintf(msg, "^MOBIT,ECAR,%s,Re,U1$\n", imei);
		} else if (strstr(msg, ",B1,")) {// Dev Auto Charge Started
	   		memset(msg, 0, BUFFER_SIZE);  
			sprintf(msg, "^MOBIT,ECAR,%s,Re,B1$\n", imei);
		} else if (strstr(msg, ",B3,")) {// Dev Auto 
	   		memset(msg, 0, BUFFER_SIZE);  
			sprintf(msg, "^MOBIT,ECAR,%s,Re,B3$\n", imei);
		} else {
   			memset(msg, 0, BUFFER_SIZE);  
			printf("Unknown CMD Recved!!!\n");
		}
	}
}

void create_msg(char* msg)
{
	char imei[] = "012345678901234";

	printf("auto_send = %d\n", auto_send);
	if (1 == auto_send) {// Srv Auto Query Params
		sprintf(msg, "^MOBIT,ECAR,%s,C0$\n", imei);
	} else if (2 == auto_send) {// Srv Auto Shutdown
		sprintf(msg, "^MOBIT,ECAR,%s,S0$\n", imei);
	} else if (3 == auto_send) {// Srv Auto Ring Alarm
		sprintf(msg, "^MOBIT,ECAR,%s,R2,5$\n", imei);
	} else if (4 == auto_send) {// Srv Auto Unlock Door
		sprintf(msg, "^MOBIT,ECAR,%s,O0$\n", imei);
	} else if (5 == auto_send) {// Srv Auto Lamp Jump
		sprintf(msg, "^MOBIT,ECAR,%s,S2,5$\n", imei);
	} else if (6 == auto_send) {// Srv Auto OneKey Start
		sprintf(msg, "^MOBIT,ECAR,%s,E0$\n", imei);
	} else if (7 == auto_send) {// Srv Auto Query GPS
		sprintf(msg, "^MOBIT,ECAR,%s,L0$\n", imei);
	} else if (8 == auto_send) {// Srv Auto IAP Request
		sprintf(msg, "^MOBIT,ECAR,%s,U0,http://xxx/xxx.bin$\n", imei);
	} else if (9 == auto_send) {// Srv Auto MP3 Update Request
		sprintf(msg, "^MOBIT,ECAR,%s,U2,test11,http://gdlt.sc.chinaz.com/Files/DownLoad/sound1/201701/8224.wav,file-md5$\n", imei);
	} else if (10 == auto_send) {// Srv Auto MP3 Play
		sprintf(msg, "^MOBIT,ECAR,%s,P0,test11$\n", imei);
	} else if (11 == auto_send) {// Srv Auto Start GPS Trace
		sprintf(msg, "^MOBIT,ECAR,%s,T0,120$\n", imei);
	} else if (12 == auto_send) {// Srv Auto Stop GPS Trace
		sprintf(msg, "^MOBIT,ECAR,%s,T2$\n", imei);
	} else if (13 == auto_send) {// Srv Auto Query BMS Status
		sprintf(msg, "^MOBIT,ECAR,%s,B0$\n", imei);
	} else if (14 == auto_send) {// Srv Auto Query MP3
		sprintf(msg, "^MOBIT,ECAR,%s,P2$\n", imei);
	}
}

void *PL_switch(void)
{
    struct sockaddr_in   server_addr;
    bzero(&server_addr, sizeof(server_addr));
    server_addr.sin_family = AF_INET;
    server_addr.sin_addr.s_addr = htonl(INADDR_ANY);
    server_addr.sin_port = htons(port_pl);

    int server_socket = socket(PF_INET, SOCK_STREAM, 0);
    if (server_socket < 0)
    {
        printf("Create Socket Failed!\n");
        exit(1);
    }

//    int set = 1;
//    setsockopt(server_socket, SOL_SOCKET, SO_NOSIGPIPE, (void*)&set, sizeof(int));


	// int flags = fcntl(server_socket, F_GETFL, 0);
	// fcntl(server_socket, F_SETFL, flags|O_NONBLOCK);
    if (bind(server_socket, (struct sockaddr*)&server_addr, sizeof(server_addr)))
    {
        printf("Server Bind Port: %d Failed!\n", port_pl);
        exit(1);
    }

    if (listen(server_socket, LENGTH_OF_LISTEN_QUEUE))
    {
        printf("Server Listen Failed!\n");
        exit(1);
    }

    printf("start pl listening...\n");
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

        printf("master client connected!!!\n");

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
			} else {
		   		memset(buffer, 0, BUFFER_SIZE);  
   	    		length = recv(new_server_socket, buffer, BUFFER_SIZE, 0);

        		if (0 == length) {
        			close(new_server_socket);
				printf("master client closed!!!\n", ret);
				break;
			}

        		if (length) {
	        		if (length < 0)
   			    	{
        			    printf("Master server Recieve Data Failed!\n");
   		        		break;
		        	}
					printf("Master recved Data(%dB): %s\n", length, buffer);
					parse_msg(buffer);

					if (strlen(buffer) > 0) {
						length = send(new_server_socket, buffer, strlen(buffer), 0);
						printf("Master send AckData(%dB): %s\n", length, buffer);
        					if (0 == length) {
        						close(new_server_socket);
							printf("Master client closed!!!\n", ret);
							break;
						}
						if (length < 0)
						{
		        	    			printf("Master server Send Data Failed!\n");
	        		    			break;
						}
					}
				}
			}

			if (auto_send > 0) {
				usleep(10000);

		   		memset(buffer, 0, BUFFER_SIZE);  
				if (88 == auto_send) {
					memcpy(buffer, debug_buffer, strlen(debug_buffer));
				} else {
					create_msg(buffer);
				}
				auto_send = 0;
				if (strlen(buffer) > 0) {
					length = send(new_server_socket, buffer, strlen(buffer), 0);
					printf("Master send AutData(%dB): %s\n", length, buffer);
       					if (0 == length) {
       						close(new_server_socket);
						printf("master client closed!!!\n", ret);
						break;
					}
					if (length < 0)
				    	{
		        	    		printf("Master server Send Data Failed!\n");
	        		    		break;
					}
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
    server_addr.sin_addr.s_addr = htonl(INADDR_ANY);
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
        printf("Server Bind Port: %d Failed!\n", port_ps);
        exit(1);
    }

    if (listen(server_socket, LENGTH_OF_LISTEN_QUEUE))
    {
        printf("Server Listen Failed!\n");
        exit(1);
    }

    printf("start ps listening...\n");
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

        printf("slave client connected!!!\n");

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
				close(new_server_socket);
				printf("slave client closed!!!\n", ret);
				break;
			}

        		if (length) {
	        		if (length < 0)
   			    	{
        			    printf("Slave server Recieve Data Failed!\n");
   		        		break;
		        	}
				printf("Slave recved Data(%dB): %s\n", length, buffer);
				if (strstr(buffer, "SET=")) {
					auto_send = atoi(buffer+4);
					printf("Slave auto_send mode = %d\n", auto_send);
				} else {
					auto_send = 88;
					memset(debug_buffer, 0, BUFFER_SIZE);
					memcpy(debug_buffer, buffer, length);
				}
			}

			length = send(new_server_socket, buffer, length, 0);
			printf("Slave send Data(%dB): %s\n", length, buffer);
			if (0 == length) {
				close(new_server_socket);
				printf("slave client closed!!!\n", ret);
				break;
			}
        	    if (length < 0)
				{
        	    	printf("Slave server Send Data Failed!\n");
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

	signal(SIGPIPE, SIG_IGN);

	sigset_t signal_mask;
	sigemptyset(&signal_mask);
	sigaddset(&signal_mask, SIGPIPE);
	int rc = pthread_sigmask(SIG_BLOCK, &signal_mask, NULL);
	if (rc != 0) {
		printf("block SIGPIPE error\n");
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

