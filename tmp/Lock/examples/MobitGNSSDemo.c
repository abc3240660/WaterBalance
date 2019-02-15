#include "MobitLTEBG96GNSS.h"

GNSS_Work_Mode_t mode = STAND_ALONE;

void setup(){
    printf("This is the Mobit Debug Serial!\n");
    delay_ms(1000);
    while(!InitModule());

    SetDevCommandEcho(false);

    const char inf[64];
    if(GetDevInformation(inf)){
        printf("Dev Info: %s!\n", inf);
    }

    while (!TurnOnGNSS(mode, WRITE_MODE)){
        printf("Open the GNSS Function Fali!\n");
        if(TurnOnGNSS(mode, READ_MODE)){
            printf("The GNSS Function is Opened!\n");
            TurnOffGNSS();
        }
    }
    printf("Open the GNSS Function Success!\n");
}

void loop(){
    const char gnss_posi[128];
    while (!GetGNSSPositionInformation(gnss_posi)){
        printf("Get the GNSS Position Fail!\n");
        int e_code;
        if (returnErrorCode(e_code)){
            printf("ERROR CODE: %d\n", e_code);
            printf("Please check the documentation for error details.\n");
        }
        delay_ms(5000);
    }
    printf("Get the GNSS Position Success!\n");
    printf("gnss_posi = %s\n", gnss_posi);
}
