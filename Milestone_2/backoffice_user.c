//Pedro Lima 2017276603
//Carlos Soares 2020230124

#include "structs.h"
#include "utils.h"

int mqid;
int pipe_fd;
pthread_t mq_thread;

void signal_handler(int signum);
void backoffice_cleanup();
void* msq_thread();
void send_message(int messagetype);
void display_help(int options);

int main(int argc, char const *argv[]){
    signal(SIGINT, signal_handler);
    if(argc != 1){
        printf("Invalid number of arguments given to %s. Exiting...\n", argv[0]);
        printf(" Usage:\n $ backoffice_user\n");
        backoffice_cleanup();
    }

    //open message queue
    mqid = msgget(ftok(MESSAGE_QUEUE_NAME,MESSAGE_QUEUE_ID), 0666);
    if(mqid < 0){
        perror("[BACKOFFICE] ERROR OPENING QUEUE\n");
        backoffice_cleanup();
    }

    //open named pipe
    if((pipe_fd = open(PIPE_USER, O_WRONLY)) == -1){
        perror("[BACKOFFICE] ERROR OPENING PIPE\n");
		backoffice_cleanup();
	}

    printf("[%d] [BACKOFFICE] BACKOFFICE CREATED.\n", getpid());

    pthread_create(&mq_thread, NULL, msq_thread, NULL);

    char line[KBUF];
    char command[KBUF];

    while(1){
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
        //instructions for command
        else if (strcmp(command, "help") == 0) {
            printf("Available commands:\n");
            display_help(0);
            display_help(1);
        } 
        else {
            printf("Invalid command. Type help for commands\n");
        }
    }
}

void backoffice_cleanup(){
    printf("[%d] [BACKOFFICE] CLEANING AND EXITING...\n", getpid());
    close(pipe_fd);

    pthread_cancel(mq_thread);
    pthread_join(mq_thread, NULL);

    exit(0);
}

void signal_handler(int signum) {
    if(signum == SIGINT) {
        printf("[%d] [BACKOFFICE] RECEIVED SIGINT...\n", getpid());
        backoffice_cleanup();
    }
}

void send_message(int messagetype){
    char message[MBUF];
    switch(messagetype){
        case 0:
            printf("[BACKOFFICE] SENDING REQUEST FOR RESET...\n");
            sprintf(message, "1#reset");
            break;
        case 1:
            printf("[BACKOFFICE] SENDING REQUEST FOR DATA STATS...\n");
            sprintf(message, "1#data_stats");
            break;
    }
    if(write(pipe_fd, message, strlen(message)) == -1){
        perror("[BACKOFFICE] ERROR WRITING TO PIPE - ");
        backoffice_cleanup();
    }

}

void* msq_thread(){
    backoffice_msg received_msg;
    block_signals_thread();
    while(1){
        if(msgrcv(mqid, &received_msg, sizeof(backoffice_msg)-sizeof(long), 1, 0) == -1){
            perror("[BACKOFFICE] ERROR AT RECEIVING MESSAGE");
            kill(getpid(), SIGINT);
        }
        printf("\nService\tTotal Data\tAuth Reqs\n");
        printf("VIDEO\t%d\t\t%d\n", received_msg.stats.video_stats.total_data, received_msg.stats.video_stats.auth_requests);
        printf("MUSIC\t%d\t\t%d\n", received_msg.stats.music_stats.total_data, received_msg.stats.music_stats.auth_requests);
        printf("SOCIAL\t%d\t\t%d\n", received_msg.stats.social_stats.total_data, received_msg.stats.social_stats.auth_requests);
    }
}

void display_help(int options){
    switch(options){
        case 0:
            printf(" reset: Requests zeroing of system statistics\n");
            break;
        case 1:
            printf(" data_stats: Requests system statistics\n");
            break;
    }
}
