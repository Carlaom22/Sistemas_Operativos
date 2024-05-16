//Pedro Lima 2017276603
//Carlos Soares 2020230124

#include "structs.h"
#include "utils.h"

#define MOBILE_ARGS 6
#define NUM_THREADS 4

pid_t parent_pid;

pthread_t mobile_threads[NUM_THREADS];
pthread_mutex_t mobile_mutex = PTHREAD_MUTEX_INITIALIZER;
pthread_cond_t mobile_cond = PTHREAD_COND_INITIALIZER;
int cond_request_num;

int args[MOBILE_ARGS];

sig_atomic_t end_flag = 0;
pid_t parent_pid;
int mqid;
int fd_mobile_pipe;
int alert_flag = 0;

void signal_handler(int signum);
void* mobile_thread(void *i);
void cleanup();
void configure_thread_signals();

int main(int argc, char const *argv[]){
    int id[NUM_THREADS];
    char buffer[MBUF];
    parent_pid = getpid();

    cond_request_num = 0;

    // checks if cli arguments are valid
    if(argc != MOBILE_ARGS+1){
        printf("Invalid number of arguments given to %s. Exiting...\n", argv[0]);
        printf("$ mobile_user / {plafond} / {max number of auth requests} / {request interval VIDEO} / {request interval MUSIC} / {request interval SOCIAL} / {data to reserve}\n");
        exit(0);
    }

    //checks if cli arguments are positive integers
    for(int i = 1; i < argc; i++){
        if(atoi(argv[i]) == 0){
            printf("[USER %d] INVALID ARGUMENT GIVEN %s. EXITING...\n", parent_pid, argv[i]);
            exit(0);
        }
        args[i-1] = atoi(argv[i]);
    }

    signal(SIGINT, signal_handler);

    // logs thread creation
    printf("[USER %d] PLAFOND: %d | REQUESTS: %d | CADANCE: %d (V)  %d (M)  %d (S) | DATA PER REQUEST: %d \n",parent_pid, args[0], args[1], args[2], args[3], args[4], args[5]);

    // opens message queue
    mqid = msgget(ftok(MESSAGE_QUEUE_NAME,MESSAGE_QUEUE_ID), 0666);
    if(mqid < 0){
        perror("[USER] ERROR OPENING QUEUE\n");
        end_flag = 1;
    }

    // opens named mobile user pipe
    if((fd_mobile_pipe = open(PIPE_USER, O_WRONLY | O_NONBLOCK)) == -1){
		perror("[USER] ERROR OPENING PIPE\n");
		end_flag = 1;
	}

    // sends register command through user pipe
    sprintf(buffer,"%d#%d",parent_pid,args[0]);
    printf("[USER %d] REGISTERING... SENDING \"%s\" TO SERVER\n",parent_pid,buffer);
    if(write(fd_mobile_pipe,buffer,strlen(buffer)+1) == -1){
        perror("[USER] ERROR WRITING TO MOBILE PIPE -");
        end_flag = 1;
    }

    sleep(1);

    // creates threads
    if (DEBUG) printf("[USER %d] Creating threads\n",parent_pid);
    for(int i = 0; i < NUM_THREADS; i++){
        id[i] = i;
        if(pthread_create(&mobile_threads[i], NULL, mobile_thread, &id[i])){
            perror("[USER] ERROR CREATING THREAD -");
            end_flag = 1;
        }
    }

    // waits for the cond
    while(!end_flag){  
        pthread_mutex_lock(&mobile_mutex);
        if (DEBUG) printf("[USER %d] WAITING FOR SIGNALS...\n",parent_pid);
        while(cond_request_num < args[1] && !alert_flag && !end_flag){
            pthread_cond_wait(&mobile_cond, &mobile_mutex);
        }
        if(cond_request_num >= args[1]){
            printf("[USER %d] REQUEST LIMIT %d/%d REACHED...",parent_pid, cond_request_num, args[1]);
            break;
        } 
        if(alert_flag){
            printf("[USER %d] PLAFOND REACHED...",parent_pid);
            break;
        } 
        if(end_flag){
            printf("[USER %d] RECEIVED CTRL+C...",parent_pid);
            break;
        }
        pthread_mutex_unlock(&mobile_mutex);
    }
    pthread_mutex_unlock(&mobile_mutex);

    
    //exiting program sequence:
    printf(" beginning exit\n");

    //sends sigint to all threads
    for(int i = 0; i < NUM_THREADS; i++){
        pthread_cancel(mobile_threads[i]);
    }

    //waits for threads to join
    for(int i = 0; i < NUM_THREADS; i++){
        pthread_join(mobile_threads[i], NULL);
    }

    if (DEBUG) printf("[USER %d] THREADS JOINED. SENDING CLOSING MSG TO PIPE\n",parent_pid);
    
    //sends pipe command to free user's resources in the server
    sprintf(buffer,"%d#0",parent_pid);
    printf("[USER %d] Sending closing msg \"%s\" to server\n",parent_pid,buffer);
    if(write(fd_mobile_pipe,buffer,strlen(buffer)+1) == -1){
        perror("[USER] ERROR WRITING TO PIPE - ");
    }
    
    if (DEBUG) printf("[USER %d] CLEANING RESOURCES",parent_pid);
    //cleanup resources
    cleanup();
    printf("[USER %d] EXITING...\n",parent_pid);
    exit(0);
}

void* mobile_thread(void *i){
    block_signals_thread();

    int my_id = *((int *)i);
    char buffer[MBUF];
    mobile_msg received_msg;

    while(!end_flag){
        switch (my_id)
            {
            case 0:
                // sends register command through user pipe
                sprintf(buffer,"%d#VIDEO#%d",parent_pid,args[5]);
                if (DEBUG) printf("[USER %d T%d] SENDING \"%s\" TO SERVER, SLEEPING FOR %d\n",parent_pid, my_id, buffer, args[2]);
                if(write(fd_mobile_pipe,buffer,strlen(buffer)+1) == -1){
                    perror("[USER] ERROR WRITING TO PIPE - ");
                }
                usleep(args[2]*1000);
                break;
            case 1:
                sprintf(buffer,"%d#MUSIC#%d",parent_pid,args[5]);
                if (DEBUG) printf("[USER %d T%d] SENDING \"%s\" TO SERVER, SLEEPING FOR %d\n",parent_pid, my_id, buffer, args[3]);
                if(write(fd_mobile_pipe,buffer,strlen(buffer)+1) == -1){
                    perror("[USER] ERROR WRITING TO PIPE - ");
                }
                usleep(args[3]*1000);
                break;
            case 2:
                sprintf(buffer,"%d#SOCIAL#%d",parent_pid,args[5]);
                if (DEBUG) printf("[USER %d T%d] SENDING \"%s\" TO SERVER, SLEEPING FOR %d\n",parent_pid, my_id, buffer, args[4]);
                if(write(fd_mobile_pipe,buffer,strlen(buffer)+1) == -1){
                    perror("[USER] ERROR WRITING TO PIPE - ");
                }
                usleep(args[4]*1000);
                break;
            case 3:
                //waits for alerts from message queue, sends sigif alert 100%
                if(msgrcv(mqid, &received_msg, sizeof(mobile_msg)-sizeof(long), (long) parent_pid, 0) == -1){
                    perror("[USER] ERROR READING FROM QUEUE - ");
                    end_flag = 1;
                }
                printf("[USER %d T%d] RECEIVED ALERT %d%% FROM SERVER\n",parent_pid, my_id, received_msg.alert_val);
                if(received_msg.alert_val == 100){
                    pthread_mutex_lock(&mobile_mutex);
                    alert_flag = 1;
                    pthread_cond_signal(&mobile_cond);
                    pthread_mutex_unlock(&mobile_mutex);
                    pthread_exit(0);
                }
                break;
            }
        // increment request count and signal main process to check if request reached max
        if (my_id != 3){
            if (DEBUG) printf("[USER %d T%d] LOGGING REQUEST TO COUNTER\n",parent_pid, my_id);
            pthread_mutex_lock(&mobile_mutex);
            cond_request_num++;
            pthread_cond_signal(&mobile_cond);
            pthread_mutex_unlock(&mobile_mutex);
        }
    }
    return 0;
}

void signal_handler(int signum) {
    if(signum == SIGINT) {
        if (DEBUG) printf("[USER %d] RECEIVED SIGINT. EXITING...\n", parent_pid);
        end_flag = 1;
        pthread_cond_signal(&mobile_cond);
    }
}

void cleanup(){
    //clean up resources
    pthread_mutex_destroy(&mobile_mutex);
    pthread_cond_destroy(&mobile_cond);

    close(fd_mobile_pipe);
    //msgqueue and pipe doesn't need further cleaning, as they are created by the server (confirm)
}