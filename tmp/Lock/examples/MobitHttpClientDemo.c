#include "MobitLTEBG96HTTP.h"

const char APN[] = "hologram";
const char http_url[] = "http://www.baidu.com";   //"http://app.tongxinmao.com:89/app/api/ip";
unsigned int comm_pdp_index = 1;    // The range is 1 ~ 16
HTTP_Body_Data_Type_t  http_type = APPLICATION_X_WWW_FORM_URLENCODED;

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

    while(!SetHTTPConfigParameters(comm_pdp_index, false, false, http_type)){
        printf("Config the HTTP Parameter Fail!\n");
        int e_code;
        if (returnErrorCode(e_code)){
            printf("ERROR CODE: %s\n", e_code);
            printf("Please check the documentation for error details.\n");
            while(1);
        }
    }
    printf("Config the HTTP Parameter Success!\n");

    while(!HTTPURL(http_url, WRITE_MODE)){
        printf("Set the HTTP URL Fail!\n");
        int e_code;
        if (returnErrorCode(e_code)){
            printf("ERROR CODE: %s\n", e_code);
            printf("Please check the documentation for error details.\n");
            while(1);
        }
    }
    printf("Set the HTTP URL Success!\n");
}

void loop(){
    const char recv_file[] = "http_read.txt";
    // char recv_data[128];
    if(!HTTPGET(80)){
        printf("HTTP GET Success!\n");
        int e_code;
        if (returnErrorCode(e_code)){
            printf("ERROR CODE: %s\n", e_code);
            printf("Please check the documentation for error details.\n");
            while(1);
        }
    }
    printf("HTTP GET Success!\n");

    if(HTTPReadToFile(recv_file, 80)){
        printf("HTTP Read to File Success!\n");
    }
    // if(HTTPRead(recv_data, 80)){
    //     printf("HTTP Read Success!\n");
    //     printf("recv_data = %s\n", recv_data);
    // }
    while(1);
}
