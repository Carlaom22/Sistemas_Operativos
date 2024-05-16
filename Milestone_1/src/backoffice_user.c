//Pedro Lima 2017276603
//Carlos Soares 2020230124

#include "backoffice_user.h"

#include "signals.h" //requires END_FLAG

volatile sig_atomic_t END_FLAG;

void signal_handler(int signum);

void display_help(int options){
    switch(options){
        case 0:
            printf(" reset: resets system\n");
            break;
        case 1:
            printf(" data_stats: prints stats\n");
            break;
    }
}

void send_message(int messagetype){
    char message[MBUF];
    switch(messagetype){
        case 0:
            printf("Sending reset...\n");
            sprintf(message, "1#reset");
            // send data stat message to named pipe
            break;
        case 1:
            printf("Sending request for data_stats...\n");
            sprintf(message, "1#data_stats");
            // send data stat message to named pipe
            break;
    }

}

int main(int argc, char const *argv[]){
    signal(SIGINT, signal_handler);
    if(argc != 1){
        printf("Invalid number of arguments given to %s. Exiting...\n", argv[0]);
        printf(" Usage:\n $ backoffice_user\n");
        exit(1);
    }

    // como é que garantimos que só um backoffice é criado? shared mem?
    printf("Backoffice user created. Currently only exiting with ctrl+c\n");

    char line[SBUF];
    char command[SBUF];

    while(!END_FLAG){
        //needs to implement SIGINT to exit, currently only exiting with ctrl+c

        printf("Enter command: ");
        fgets(line, sizeof(line), stdin);

        sscanf(line, "%s", command);

        //instructions for mobileuser command
        if (strcmp(command, "reset") == 0) {
            send_message(0);
        } 
        //instructions for backoffice command
        else if (strcmp(command, "data_stats") == 0) {
            send_message(1);
        } 
        //instructions for sysmanager command
        else if (strcmp(command, "help") == 0) {
            printf("Available commands:\n");
            display_help(0);
            display_help(1);
        } 
        else {
            printf("Invalid command. Type help for list of available commands\n");
        }
    }
}

void signal_handler(int signum) {
    if(signum == SIGINT) {
        printf("Received SIGINT. Exiting...\n");
        END_FLAG = 1;
    }
}