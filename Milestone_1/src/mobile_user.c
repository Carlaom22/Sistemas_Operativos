//Pedro Lima 2017276603
//Carlos Soares 2020230124

#include "mobile_user.h"

#include "signals.h" //requires END_FLAG

volatile sig_atomic_t END_FLAG;

void signal_handler(int signum);

int main(int argc, char const *argv[]){
    signal(SIGINT, signal_handler);
    int args[MOBILE_ARGS];

    if(argc != MOBILE_ARGS+1){
        printf("Invalid number of arguments given to %s. Exiting...\n", argv[0]);
        printf("Usage:\n $ mobile_user / {starting plafond} / {max number of auth requests} / {request interval VIDEO} / {request interval MUSIC} / {request interval SOCIAL} / {data to reserve}\n");
        exit(1);
    }
    //checks if all arguments are positive integers
    for(int i = 1; i < argc; i++){
        if(atoi(argv[i]) < 0){
            printf("Error parsing arguments given to mobile_user. All arguments must be positive integers. Exiting...\n");
            exit(1);
        }
        args[i-1] = atoi(argv[i]);
    }

    printf("Mobile user created with plafond %s; %s max auth requests; repeat intervals of %s (video) , %s (music), %s (social); %s data to reserve\n", argv[1], argv[2], argv[3], argv[4], argv[5], argv[6]);

    while(!END_FLAG){
        //needs to implement SIGINT to exit, currently only exiting after some time
        sleep(50);
        // needs to implement exit on max number of requests
        // needs to implement exit on 100% alert
        printf("Finished sleeping, exiting...\n");
        exit(1);
    }
}

void signal_handler(int signum) {
    if(signum == SIGINT) {
        printf("Received SIGINT. Exiting...\n");
        END_FLAG = 1;
    }
}