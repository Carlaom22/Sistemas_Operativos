//Pedro Lima 2017276603
//Carlos Soares 2020230124

//i think the write_log bug is because it can't be used inside a pthread mutex routine
#include "authmanager.h"

#include "sysmanager.h" // maybe shouldn't
#include "sharedm.h"
#include "signals.h" //requires END_FLAG
#include "logs.h"

sem_t **writers_mem;

void signal_handler_auth(int signum);
void signal_handler_thread(int signum);
void signal_handler_engine(int signum);

// pthread_mutex_t video_queue_mutex = PTHREAD_MUTEX_INITIALIZER; // Video Streaming Queue Mutex
// pthread_mutex_t other_queue_mutex = PTHREAD_MUTEX_INITIALIZER; // Other Services Queue Mutex
pthread_mutex_t mutex_engine = PTHREAD_MUTEX_INITIALIZER; // Create/Destroy Engine Mutex
pthread_mutex_t mutex_queue = PTHREAD_MUTEX_INITIALIZER; // Queue Mutex

pthread_cond_t mutex_queue_cond = PTHREAD_COND_INITIALIZER; // Cond
pthread_cond_t mutex_engine_cond = PTHREAD_COND_INITIALIZER; // Cond
// pthread_cond_t videomutex_queue_cond = PTHREAD_COND_INITIALIZER; // Cond
// pthread_cond_t othermutex_queue_cond = PTHREAD_COND_INITIALIZER; // Cond

pthread_t receiver_thread, sender_thread; // Receiver and Sender threads
pid_t *auth_engines;
int auth_engine_count;

int videoQueueSize;
int otherQueueSize;
int *videoQueue; 
int *otherQueue; 
int CREATE_FROM_VIDEO_FLAG;
int CREATE_FROM_OTHERS_FLAG;
int WHICH_QUEUE; // 0 for video, 1 for others

void auth_engine_writer(pid_t request_id, int request_size, int request_type, int index, int valid, char* message);

void check_queue_size(int *CREATE_FLAG, int *QUEUE_SIZE);


void auth_manager(){
    signal(SIGUSR1, signal_handler_auth);
    WHICH_QUEUE = 0;
    CREATE_FROM_OTHERS_FLAG = 0;
    CREATE_FROM_VIDEO_FLAG = 0;
    videoQueueSize = 0;
    otherQueueSize = 0;
    auth_engine_count = 0;
    videoQueue = (int *)malloc(sizeof(int) * QUEUE_POS);
    otherQueue = (int *)malloc(sizeof(int) * QUEUE_POS);   

    // Create Receiver and Sender threads
    pthread_create(&receiver_thread, NULL, auth_receiver, NULL);
    pthread_create(&sender_thread, NULL, auth_sender, NULL);
    
    auth_engines = (pid_t *)malloc(sizeof(pid_t) * AUTH_SERVERS+1);
    if(auth_engines == NULL){
        perror("Error allocating memory for Authorization Engines. Ending auth_manager process...");
        write_log("Error allocating memory for Authorization Engines. Ending auth_manager process...");
        finish();
        exit(1);
    }

    // missing auth_engine pipe initialization

    // Allocate writer semaphores for shared memory
    writers_mem = (sem_t **)malloc(sizeof(sem_t *) * MOBILE_USERS);
    if(writers_mem == NULL){
        perror("Error allocating memory for writer semaphores. Ending auth_manager process...");
        write_log("Error allocating memory for writer semaphores. Ending auth_manager process...");
        finish();
        exit(1);
    }

    // Initialize writer semaphores for shared memory
    char sem_name[SBUF];
    for(int i = 0; i < MOBILE_USERS; i++){
        snprintf(sem_name, sizeof(sem_name), "%s%d", WRITERS_MEM, i);
        sem_unlink(sem_name);
        writers_mem[i]=sem_open(sem_name,O_CREAT|O_EXCL,0700,1); // create semaphore
        if(writers_mem[i]==SEM_FAILED){
            perror("Failure creating a writer semaphore. Ending auth_manager process...");
            write_log("Failure creating a writer semaphore. Ending auth_manager process...");
            finish();
            exit(1);
        }
    }

    // Create Authorization Engines
    for(int i = 0; i < AUTH_SERVERS; i++){
        // How does the sender/receiver know which engine to send requests to in the pipe? mutex?
        char message[MBUF];
        sprintf(message,"Creating Authorization Engine %d.", i);
        write_log(message);

        auth_engine_count++;

        auth_engines[i] = fork();
        if(auth_engines[i] == -1){
            perror("Error creating Authorization Engine. Ending auth_manager process...");
            write_log("Error creating Authorization Engine. Ending auth_manager process...");
            finish();
            exit(1);
        }
        else if(auth_engines[i] == 0){
            sprintf(message,"Authorization Engine %d created.", i);
            write_log(message);
            //create unnamed pipe
            auth_engine();
            exit(0);
        }
    }

    // Creates the extra Authorization Engine
    while(END_FLAG == 0){
        int *CREATE_FLAG;
        char message[MBUF];

        pthread_mutex_lock(&mutex_engine);
        printf("AUTH: Waiting for signal to create/destroy Authorization\n");
        pthread_cond_wait(&mutex_engine_cond, &mutex_engine);   //releases mutex and waits for signal

        if (auth_engine_count > AUTH_SERVERS){
            printf("Error: Maximum number of Authorization Engines reached.\n");
            pthread_mutex_unlock(&mutex_engine);
            continue;
        }
        // Check which queue was signalled and set the CREATE_FLAG accordingly
        if(WHICH_QUEUE == 0) CREATE_FLAG = &CREATE_FROM_VIDEO_FLAG; 
        else CREATE_FLAG = &CREATE_FROM_OTHERS_FLAG; 

        if ((*CREATE_FLAG) == 1){
            // Create Authorization Engine
            printf("Creating Extra Engine\n");
            sprintf(message,"Creating Authorization Engine %d.", auth_engine_count);
            // write_log(message);

            auth_engines[auth_engine_count] = fork();
            if(auth_engines[auth_engine_count] == -1){
                perror("Error creating Authorization Engine. Ending auth_manager process...");
                //write_log("Error creating Authorization Engine. Ending auth_manager process...");
                finish();
                exit(1);
            }
            else if(auth_engines[auth_engine_count] == 0){
                sprintf(message,"Authorization Engine %d created.", auth_engine_count);
                // write_log(message);
                //create unnamed pipe
                auth_engine();
                exit(0);
            }
            auth_engine_count++;
        }
        else{
            // Destroy Authorization Engine
            char message[MBUF];
            sprintf(message,"Destroying Authorization Engine %d.", auth_engine_count-1);
            printf("Destroying Extra Engine\n");
            // write_log(message);

            kill(auth_engines[auth_engine_count-1], SIGUSR1);
            auth_engine_count--;
        }

        pthread_mutex_unlock(&mutex_engine);
    }

    
    wait(&auth_engines[0]); // just to be in passive wait
    // Sends signal to kill itself
    kill(getpid(), SIGUSR1);
}

void check_queue_size(int *CREATE_FLAG, int *QUEUE_SIZE){
    printf("Checking queue size in Sender...\n");
    pthread_mutex_lock(&mutex_engine);
    if (*QUEUE_SIZE >= QUEUE_POS && *CREATE_FLAG == 0) {
        printf("Queue size is at 100%% capacity, creating Auth Engine...\n");
        *CREATE_FLAG = 1;
        WHICH_QUEUE = 0;
        pthread_cond_signal(&mutex_engine_cond);
    }
    else if (*QUEUE_SIZE < 0.5*QUEUE_POS && *CREATE_FLAG == 1) {
        printf("Queue size dipped below 100%% capacity, freeing Auth Engine...\n");
        *CREATE_FLAG = 0;
        WHICH_QUEUE = 0;
        pthread_cond_signal(&mutex_engine_cond);
    }
    pthread_mutex_unlock(&mutex_engine);
}


// Thread: Receiver
void *auth_receiver(void *arg){
    signal(SIGUSR2, signal_handler_thread);

    int request_type = 0; // 0 for video, 1 for other    
    write_log("Receiver thread created.");
    while (END_FLAG == 0) {
        pthread_mutex_lock(&mutex_queue);
        while(videoQueueSize == QUEUE_POS && otherQueueSize == QUEUE_POS){
            write_log("Waiting for queue to free up in Receiver...");
            pthread_cond_wait(&mutex_queue_cond, &mutex_queue);
        }
        
        if (!request_type) {
            printf("Received video %d request in Receiver...\n", videoQueueSize);
            videoQueue[videoQueueSize] = 50;
            videoQueueSize++;

        } else {
            printf("Received other %d request in Receiver...\n", videoQueueSize);
            otherQueueSize++;
            otherQueue[otherQueueSize] = 50;  
        }
        pthread_cond_signal(&mutex_queue_cond);
        pthread_mutex_unlock(&mutex_queue);
        sleep(1);
        // sleep(1); //placeholder for request time

    }
    pthread_exit(NULL);
}

// Thread: Sender
void *auth_sender(void *arg){
    signal(SIGUSR2, signal_handler_thread);
    
    char message[MBUF];
    write_log("Sender thread created.");
    while (END_FLAG == 0) {

        pthread_mutex_lock(&mutex_queue);
        while (videoQueueSize == 0 && otherQueueSize == 0) {
            printf("Waiting for mutex_queue_cond in Sender...\n");
            pthread_cond_wait(&mutex_queue_cond, &mutex_queue);
        }
        printf("Sending request in Sender...\n");
        
        if (videoQueueSize > 0) {
            check_queue_size(&CREATE_FROM_VIDEO_FLAG, &videoQueueSize);
            // Fetch a request from the video queue...
            videoQueueSize--;
            printf("Fetched %d from video queue\n",videoQueue[videoQueueSize]);
            //write_log(message);
            videoQueue[videoQueueSize] = 0;
        } else if (otherQueueSize > 0) {
            check_queue_size(&CREATE_FROM_OTHERS_FLAG, &otherQueueSize);
            // Fetch a request from the video queue...
            otherQueueSize--;
            printf("Fetched %d from other queue",otherQueue[otherQueueSize]);
            //write_log(message);
            otherQueue[otherQueueSize] = 0;
        }

        pthread_mutex_unlock(&mutex_queue);
        sleep(5);
        

        // // if (videoQueueSize > 0) {
        // //     check_queue_size(&CREATE_FROM_VIDEO_FLAG, &videoQueueSize);
        // //     // Fetch a request from the video queue...
        // //     videoQueueSize--;
        // //     sprintf(message,"Fetched %d from video queue",videoQueue[videoQueueSize]);
        // //     write_log(message);
        // //     videoQueue[videoQueueSize] = 0;
        // // } else if (otherQueueSize > 0) {
        // //     check_queue_size(&CREATE_FROM_OTHERS_FLAG, &otherQueueSize);
        // //     // Fetch a request from the video queue...
        // //     otherQueueSize--;
        // //     sprintf(message,"Fetched %d from other queue",otherQueue[otherQueueSize]);
        // //     write_log(message);
        // //     otherQueue[otherQueueSize] = 0;
        // // }
        // pthread_mutex_unlock(&mutex_queue);
        // // Send the request...
        // write_log("Sending message to auth engine...");
        // sleep(1); //placeholder for processing time
    }
    pthread_exit(NULL);
}


void auth_engine(){
    signal(SIGUSR1, signal_handler_engine);
    int FOUND_FLAG; // -1 for not found, i for found at index i
    int VALID_FLAG; // 0 for invalid request, 1 for valid request (enough data)
    mobiles = (mobile_struct*) (stats + 1);

    char request_buf[SBUF];
    char message_buf[SBUF];
    pid_t request_id;
    int request_type; // -1 for register, 0 for video, 1 for music, 2 for social
    int request_size; // plafond for register, data for video/music/social

    while(!END_FLAG){
        write_log("ENG: Waiting for requests...");
        sleep(4); // placeholder
        // waits for responses
        strcpy(request_buf, "request"); // placeholder
        write_log("ENG: Received request in auth engine...");
        
        //parse request
        request_id = 1234; // placeholder
        request_type = 0; // placeholder
        request_size = 2000; // placeholder

        // processes request (how is this being timed? certainly not here right?)
        auth_engine_reader(request_id, request_size, request_type, &FOUND_FLAG, &VALID_FLAG);
        auth_engine_writer(request_id, request_size, request_type, FOUND_FLAG, VALID_FLAG, message_buf);

        //send message to queue and log
        write_log("ENG: Seding response to queue...");
        write_log(message_buf);
        //queue_send(message_buf);
    }
    //close pipes maybe? idk
}

//needs better error handling and confirmation that the syncronization is working properly
void auth_engine_reader(pid_t request_id, int request_size, int request_type, int* index, int* valid){
    int readers;
    *index = -1;
    *valid = 0;
    mobile_struct* temp_pointer = mobiles;

    write_log("ENG: Waiting to read...");
    // locks writers
    sem_wait(mutex_mem);
    readers = ++stats->readers;
    if (readers == 1) {
        for (int i = 0; i < MOBILE_USERS; i++) {
            sem_wait(writers_mem[i]);
        }
    }
    sem_post(mutex_mem);

    write_log("ENG: Starting to read...");
    
    // reads from shared memory
    for (int i = 0; i < MOBILE_USERS; i++) {
        if(request_type >= 0 && temp_pointer->client_id == request_id){
            if(temp_pointer->plafond - request_size >= 0) *valid = 1;
            *index = i;
            break;
        }
        else if(request_type < 0 && temp_pointer->client_id){
            *index = i;
            break;
            }
        temp_pointer++;
    }

    write_log("ENG: End read...");

    // unlocks writers
    sem_wait(mutex_mem);
    readers = --stats->readers;
    if (readers == 0) {
        for (int i = 0; i < MOBILE_USERS; i++) {
            sem_post(writers_mem[i]);
            
        }
        write_log("ENG: Unlocked writers...");
    }
    sem_post(mutex_mem);
}

//needs better error handling and confirmation that the syncronization is working properly
void auth_engine_writer(pid_t request_id, int request_size, int request_type, int index, int valid, char* message){
    mobile_struct* temp_pointer = mobiles+index;

    if (!valid) sprintf(message, "Mobile user with id: %d does not have enough data.", (int) request_id);
    else if (index < 0) sprintf(message, "Mobile user with id: %d not found.", (int) request_id);
    else{

        write_log("ENG: Waiting to write...");

        sem_wait(mutex_mem); // Lock readers semaphore
        sem_wait(writers_mem[index]); // Lock writer semaphore for specific index

        write_log("ENG: Starting to write..."); 

        if(request_type < 0){
            temp_pointer->client_id = request_id;
            temp_pointer->plafond = request_size;
            sprintf(message, "Mobile user with id: %d registered with plafond %d.", (int) request_id, request_size);
        }
        else if(valid){
            temp_pointer->plafond -= request_size;
            sprintf(message, "Mobile user with id: %d found at index %d.", (int) request_id, index);
        }

        write_log("ENG: End write...");

        sem_post(writers_mem[index]); // Unlock writer semaphore for specific index
        sem_post(mutex_mem);   
    }
}


void signal_handler_auth(int signum) {
    if(signum == SIGUSR1) {
        char message[MBUF];
        sprintf(message, "Received SIGUSR1. Cleaning resources of auth manager process with pid = %d...", getpid());
        write_log(message);

        sem_close(mutex_log);
        sem_close(mutex_mem);
        for(int i = 0; i < MOBILE_USERS; i++){
            sem_close(writers_mem[i]);
        }
        free(writers_mem);

        write_log("Cleaned auth manager semaphores...");

        // Send signal to threads to finish
        END_FLAG = 1;
        pthread_kill(receiver_thread, SIGUSR2);
        pthread_kill(sender_thread, SIGUSR2);

        // Wait for threads to finish
        pthread_join(receiver_thread, NULL);
        pthread_join(sender_thread, NULL);  
        // pthread_mutex_destroy(&videomutex_queue);
        // pthread_mutex_destroy(&othermutex_queue);
        pthread_mutex_destroy(&mutex_queue);
        pthread_mutex_destroy(&mutex_engine);
        pthread_cond_destroy(&mutex_engine_cond);
        pthread_cond_destroy(&mutex_queue_cond);
        // pthread_cond_destroy(&videomutex_queue_cond);
        // pthread_cond_destroy(&othermutex_queue_cond);

        write_log("Cleaned auth manager thread conditions and mutexes...");

        // Destroy Authorization Engines
        for(int i = 0; i < auth_engine_count; i++){
            kill(auth_engines[i], SIGUSR1);
        }
        
        for(int i = 0; i < AUTH_SERVERS+1; i++){
            wait(NULL);
        }

        free(auth_engines);
        write_log("Cleaned auth engines... Exiting auth manager process.");
        exit(0);
        }
}

void signal_handler_engine(int signum) {
    if(signum == SIGUSR1) {
        char message[MBUF];
        sprintf(message, "Received SIGUSR1. Cleaning resources of auth engine process with pid = %d...", getpid());
        write_log(message);

        sem_close(mutex_log);
        sem_close(mutex_mem);
        for(int i = 0; i < MOBILE_USERS; i++){
            sem_close(writers_mem[i]);
        }

        write_log("Cleaned auth engine semaphores...");

        // Send signal to threads to finish
        END_FLAG = 1;
        // Destroy mutexes
        // pthread_mutex_destroy(&videomutex_queue);
        // pthread_mutex_destroy(&othermutex_queue);
        pthread_mutex_destroy(&mutex_queue);
        pthread_mutex_destroy(&mutex_engine);
        pthread_cond_destroy(&mutex_queue_cond);
        pthread_cond_destroy(&mutex_engine_cond);
        // pthread_cond_destroy(&videomutex_queue_cond);
        // pthread_cond_destroy(&othermutex_queue_cond);

        write_log("Cleaned auth engine conditions and mutexes... Exiting auth engine process.");
        exit(0);
        }
}

void signal_handler_thread(int signum) {
    if(signum == SIGUSR2) {
        char message[MBUF];
        sprintf(message, "Received SIGUSR2. Cleaning resources of thread with pid = %d...", getpid());
        write_log(message);
        // do something else
        write_log("...All resources cleaned. Exiting thread.");
        pthread_exit(0);
        }
        
}