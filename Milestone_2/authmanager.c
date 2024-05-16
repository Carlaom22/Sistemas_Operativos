//Pedro Lima 2017276603
//Carlos Soares 2020230124

#include "authmanager.h"

#define THRESHOLD_LOW 0.5
#define THRESHOLD_HIGH 0.9

pthread_mutex_t queue_mutex = PTHREAD_MUTEX_INITIALIZER;
pthread_cond_t sender_cond = PTHREAD_COND_INITIALIZER;
pthread_cond_t manager_cond = PTHREAD_COND_INITIALIZER;
pthread_t receiver_thread, sender_thread;

int fd_pipe_user, fd_pipe_admin;

char** queue_v, **queue_o;
int queue_v_start = 0;
int queue_v_rear = 0;
int queue_v_count = 0;
int queue_o_start = 0;
int queue_o_rear = 0;
int queue_o_count = 0;

int manager_flag = 0;
int engine_created_flag = 0;
int which_queue_created_flag = 0;


void auth_manager() { //auth manager
    char buffer[MBUF];
    pid_t engine_pid;
    signal(SIGUSR1, auth_handler);

    if (DEBUG) write_log("[AUTH MANAGER] STARTING...", 0);
    block_signals_thread(); //configuration sequence
    pthread_mutex_lock(&sync_pointer->auths_mutex);

    // Initialize engines
    for(int i = 0; i < AUTH_SERVERS; i++){
        auths_pointer[i].engine_id = -1;
        auths_pointer[i].state = 0;
        if((pipe(auths_pointer[i].fd_pipe)) == -1){
            if (DEBUG) write_log_error("[AUTH MANAGER] ERROR CREATING UNNAMED PIPE.",0,errno);
            //kill(system_pid, SIGINT);
        }
    }

    // Creates engines
    for(int i = 0; i < AUTH_SERVERS-1; i++){
        // How does the sender/receiver know which engine to send requests to in the pipe? mutex?
        sprintf(buffer,"[AUTH MANAGER] Creating Engine #%d.", i);
        write_log(buffer,1);

        engine_pid = fork();
        if(engine_pid == -1){
            if (DEBUG) write_log_error("[AUTH MANAGER] ERROR CREATING ENGINE.",0,errno);
            kill(system_pid, SIGINT);
        }
        else if(engine_pid == 0){
            auth_engine(i);
            exit(0);
        }
        else{
            auths_pointer[i].engine_id = engine_pid;
        }
    }

    pthread_mutex_unlock(&sync_pointer->auths_mutex);

    auth_setup();  // auth manager config
    unblock_signals_thread(); //configuration sequence end

    sleep(1); //let monitor grab mutex

    // thread creation
    pthread_create(&sender_thread, NULL, sender, NULL);
    pthread_create(&receiver_thread, NULL, receiver, NULL);
    
    engine_manager();   // engine creation/deletion block

    auth_cleanup();
}

void auth_setup(){ 
    char buffer[MBUF];
    // Allocate memory for each queue
    queue_v = (char **)malloc(sizeof(char*)*QUEUE_POS);
    queue_o = (char **)malloc(sizeof(char*)*QUEUE_POS);
    if(queue_v == NULL || queue_o == NULL){
        write_log_error("[AUTH MANAGER] FAILED TO ALOCATE MEM FOR QUEUE POINTERS",0,errno);
        end_flag = 1;
    }
    
    for (int i = 0; i < QUEUE_POS; i++) {
        queue_v[i] = malloc(MBUF * sizeof(char)); 
        if (queue_v[i] == NULL) {
            sprintf(buffer, "[AUTH MANAGER] FAILED TO ALOCATE MEM FOR QUEUE_VIDEO[%d]", i);
            write_log_error(buffer,0,errno);
            end_flag = 1;
        }
        queue_o[i] = malloc(MBUF * sizeof(char)); 
        if (queue_o[i] == NULL) {
            sprintf(buffer, "[AUTH MANAGER] FAILED TO ALOCATE MEM FOR QUEUE_OTHERS[%d]", i);
            write_log_error(buffer,0,errno);
            end_flag = 1;
        }
    }

    // create named pipes (write to send end signals to receiver)
	unlink(PIPE_USER);
    unlink(PIPE_ADMIN);
	if((mkfifo(PIPE_USER, O_CREAT | O_EXCL | 0666)==-1) || (mkfifo(PIPE_ADMIN, O_CREAT | O_EXCL | 0666)==-1)){
        write_log_error("[AUTH MANAGER] ERROR CREATING NAMED PIPES",0,errno);
		end_flag = 1;
	}
    fd_pipe_user = open(PIPE_USER, O_RDWR|O_NONBLOCK);
    fd_pipe_admin = open(PIPE_ADMIN, O_RDWR|O_NONBLOCK);
    if(fd_pipe_user == -1 || fd_pipe_admin == -1){
        write_log_error("[AUTH MANAGER] ERROR OPENING NAMED PIPES",0,errno);
        end_flag = 1;
    }
}

void auth_cleanup(){
    char buffer[MBUF];

    //join threads
    if (DEBUG) write_log("[AUTH MANAGER] ORDERING THREADS TO TERMINATE...", 2);

    //send termination signals to threads
    write(fd_pipe_admin, "terminate", strlen("terminate")+1);
    pthread_cond_signal(&sender_cond);
    pthread_cond_signal(&sync_pointer->mem_cond1); //sender waiting for engine to be available

    pthread_join(sender_thread, NULL);
    pthread_join(receiver_thread, NULL);
    if (DEBUG) write_log("[AUTH MANAGER] THREADS FINISHED...", 1);

    if (DEBUG) write_log("[AUTH MANAGER] ORDERING ENGINES TO TERMINATE...", 1); //this is done here because if the sender holds the mutex, the engine is locked in the wrong place (can be salvaged with a broadcast signal to all engines, but this is cleaner)
    //change state of all engines to -1
    pthread_mutex_lock(&sync_pointer->auths_mutex);
    for(int i = 0; i < AUTH_SERVERS; i++){
        auths_pointer[i].state = -1;
    }
    pthread_mutex_unlock(&sync_pointer->auths_mutex);
    //send termination messages to all engines
    for(int i = 0; i < AUTH_SERVERS; i++){
        write(auths_pointer[i].fd_pipe[1], "terminate", strlen("terminate")+1);
    }

    if (DEBUG) write_log("[AUTH MANAGER] WAITING FOR ENGINES TO FINISH...", 1);
    // wait for all engines to finish
    for(int i = 0; i < AUTH_SERVERS; i++){
        waitpid(auths_pointer[i].engine_id, NULL, 0);
    }
    if (DEBUG) write_log("[AUTH MANAGER] ENGINES FINISHED...", 1);

    if (DEBUG) write_log("[AUTH MANAGER] CLOSING UNNAMED PIPES...", 1);
    // close and unlink unnamed pipes
    for(int i = 0; i < AUTH_SERVERS; i++){
        close(auths_pointer[i].fd_pipe[0]);
        close(auths_pointer[i].fd_pipe[1]);
    }

    //destroy mutexes and conditions
    if (DEBUG) write_log("[AUTH MANAGER] DESTROYING MUTEX/COND...", 1);
    pthread_cond_destroy(&sender_cond);
    pthread_cond_destroy(&manager_cond);
    pthread_mutex_destroy(&queue_mutex);

    sprintf(buffer,"[AUTH MANAGER] LEAVING WITH %d VIDEO REQUESTS AND %d OTHER REQUESTS TO PROCESS...", queue_v_count, queue_o_count);
    write_log(buffer, 1);

    //free memory
    if (DEBUG) write_log("[AUTH MANAGER] FREEING MEMORY...", 1);
    for (int i = 0; i < QUEUE_POS; i++) {
        free(queue_v[i]);
        free(queue_o[i]);
    }
    free(queue_v);
    free(queue_o);

    //close and unlink named pipes
    if (DEBUG) write_log("[AUTH MANAGER] CLOSING NAMED PIPES...", 1);
    close(fd_pipe_user);
    close(fd_pipe_admin);
    unlink(PIPE_USER);
    unlink(PIPE_ADMIN);
}

void auth_handler(int sig){
    if(sig == SIGUSR1)
    {
        write_log("[AUTH MANAGER] SIGUSR1 RECEIVED",2);
        end_flag = 1;

        if (DEBUG) write_log("[AUTH MANAGER] SENDING SIGNALS TO THREADS...",2);
        // send signals to free threads
        //pthread_cond_signal(&sync_pointer->mem_cond1); //sender waiting for engine to be available
        //pthread_cond_signal(&sender_cond);             //sender waiting for receiver signal
        pthread_cond_signal(&manager_cond);              //signal to unblock itself
        //pthread_cond_signal(&sync_pointer->mem_cond4); //manager waiting for engine to be available
    }
}

void* receiver(){ //receiver
    block_signals_thread();
    char request [MBUF];
    char buffer[MBUF];
    int queue_flag, bytes_read, max_fd, select_result;
    fd_set readfds;

    while (!end_flag) {
        queue_flag = -1;

        if (DEBUG) write_log("[RECEIVER] WAITING FOR A MESSAGE FROM NAMED PIPE.",0);
        FD_ZERO(&readfds);
        FD_SET(fd_pipe_admin, &readfds);
        FD_SET(fd_pipe_user, &readfds);
        max_fd = (fd_pipe_admin > fd_pipe_user) ? fd_pipe_admin : fd_pipe_user;
        select_result = select(max_fd + 1, &readfds, NULL, NULL, NULL);
        if (select_result > 0){
            if (FD_ISSET(fd_pipe_admin, &readfds)) {
                // Read from backoffice pipe
                bytes_read = read(fd_pipe_admin, request, sizeof(request));
                if (end_flag) break;
                if (bytes_read == -1) {
                    write_log_error("[RECEIVER] ERROR READING FROM BACKOFFICE PIPE.",0, errno);
                }
                request[bytes_read] = '\0';
                queue_flag = 0;

                sprintf(buffer,"[RECEIVER] RECEIVED MESSAGE FROM BACKOFFICE PIPE: %s", request);
                if (DEBUG) write_log(buffer,0);
                //check if it's a terminate receiver signal (sent by sigusr1 handler)
                if (strcmp(request, "terminate") == 0) {
                    break;
                }
            }
            else if(FD_ISSET(fd_pipe_user, &readfds)){
                // Read from user pipe
                bytes_read = read(fd_pipe_user, request, sizeof(request));
                if (end_flag) break;
                if (bytes_read == -1) {
                    write_log_error("[RECEIVER] ERROR READING FROM USER PIPE.",0, errno);
                }
                request[bytes_read] = '\0';
                queue_flag = 0;

                sprintf(buffer,"[RECEIVER] RECEIVED MESSAGE FROM USER PIPE: %s", request);
                if (DEBUG) write_log(buffer,0);
                //find if after the first #there's a "v" char
                char *hash_ptr = strchr(request, '#');
                if (hash_ptr != NULL && *(hash_ptr + 1) == 'V') {
                    queue_flag = 1;
                }
            }

            //processing request
            if (DEBUG) write_log("[RECEIVER] WAITING FOR QUEUE",0);
            // processing the request with lock
            pthread_mutex_lock(&queue_mutex);
            if(queue_flag == 1){
                //if video queue is full, discard request
                if(queue_v_count >= QUEUE_POS){
                    sprintf(buffer,"[RECEIVER] QUEUE FULL - DISCARDING REQUEST %s", request);
                    write_log(buffer,0);
                }
                //else add to queue_v
                else{
                    sprintf(buffer,"[RECEIVER] ADDING REQUEST \"%s\" TO VIDEO_QUEUE", request);
                    if (1) write_log(buffer,0);
                    queue_add(1, request);
                    //internal_queue_v_count++;
                }
            }
            else if (queue_flag == 0){      //not an else because queue_flag can be -1 if error in select           
                //if other queue is full, discard request
                if(queue_o_count >= QUEUE_POS){
                    sprintf(buffer,"[RECEIVER] QUEUE FULL - DISCARDING REQUEST %s", request);
                    write_log(buffer,0);
                }
                else{
                    sprintf(buffer,"[RECEIVER] ADDING REQUEST \"%s\" TO OTHERS_QUEUE", request);
                    if (1) write_log(buffer,0);

                    queue_add(0, request);
                    //internal_queue_o_count++;
                }
            }
            pthread_cond_signal(&sender_cond);
            pthread_mutex_unlock(&queue_mutex);

            if (DEBUG) write_log("[RECEIVER] SIGNALING SENDER",0);
        }
    }
    pthread_mutex_lock(&queue_mutex);
    pthread_cond_signal(&sender_cond);
    pthread_mutex_unlock(&queue_mutex);
    return NULL;
}

void* sender(){ //sender
    block_signals_thread();

    char* request;
    char buffer[MBUF];
    int queue_flag, engine_index;
    int request_id;
    struct timespec timeout;
    int ret;

    while (!end_flag) {
        pthread_mutex_lock(&queue_mutex);
        // condition to wait for receiver, with a flag to end the program
        while (queue_v_count <= 0 && queue_o_count <= 0 && !end_flag) {
            if (DEBUG) write_log("[SENDER] WAITING FOR RECEIVER SIGNAL",0);
            pthread_cond_wait(&sender_cond, &queue_mutex);
        }
        if(end_flag){
            pthread_mutex_unlock(&queue_mutex);
            break;
        }

        // pop last request from queues
        if(queue_v_count > 0){
            request = queue_pop(1);
            queue_flag = 1;
        }else{
            request = queue_pop(0);
            queue_flag = 0;
        }

        sprintf(buffer," -> [SENDER] POP REQUEST %s FROM QUEUE %d", request, queue_flag);
        if (DEBUG) write_log(buffer,0);

        // send signal to manager, when signal is outside of mutex it deadlocks for some reason
        if (queue_v_count <= QUEUE_POS * THRESHOLD_LOW || queue_v_count >= QUEUE_POS * THRESHOLD_HIGH || queue_o_count <= QUEUE_POS * THRESHOLD_LOW || queue_o_count >= QUEUE_POS * THRESHOLD_HIGH){
            manager_flag = 1;
            if (DEBUG) write_log("[SENDER] SENDING CREATE/DESTROY ENGINE SIGNAL",0);
            pthread_cond_signal(&manager_cond);
        }
        pthread_mutex_unlock(&queue_mutex);

        //find available auth engine
        engine_index = -1; //resets index
        if (DEBUG) write_log("[SENDER] SEARCHING FOR AVAILABLE ENGINE",0);

        pthread_mutex_lock(&sync_pointer->auths_mutex);                 //locks the auths
        while(engine_index == -1 && !end_flag){
            for(int i = 0; i < AUTH_SERVERS; i++){
                if(auths_pointer[i].state == 1){
                    sprintf(buffer,"[SENDER] FOUND AVAILABLE ENGINE %d", auths_pointer[i].engine_id);
                    if (DEBUG) write_log(buffer,0);
                    auths_pointer[i].state = 0;
                    engine_index = i;
                    break;
                }
            }
            if (engine_index == -1 && !end_flag){
                clock_gettime(CLOCK_REALTIME, &timeout);
                timeout.tv_nsec += (queue_flag == 1) ? MAX_VIDEO_WAIT*1000000 : MAX_OTHERS_WAIT*1000000;
                timeout.tv_sec += timeout.tv_nsec / 1000000000;
                timeout.tv_nsec %= 1000000000;

                // waits for an available engine with timeout
                if (DEBUG) write_log("[SENDER] NO AVAILABLE ENGINE. WAITING FOR SIGNAL...",0);
                ret = pthread_cond_timedwait(&sync_pointer->mem_cond1, &sync_pointer->auths_mutex, &timeout); //waits for a signal that an engine is available
                
                if(ret == ETIMEDOUT){
                    write_log("[SENDER] TIMEOUT REACHED. DISCARDING REQUEST",0);
                    break;
                }if (DEBUG) write_log("[SENDER] ENGINE AVAILABILITY SIGNAL RECEIVED...",0);
            }
        }
        pthread_mutex_unlock(&sync_pointer->auths_mutex);           //unlocks the auths

        if(end_flag) break; //if end flag is set, breaks the loop

        // Process the request
        request_id = get_request_id(request);
        sprintf(buffer,"[SENDER] SENDING REQUEST \"%s\" (ID = %d) TO ENGINE [%d]", request, request_id, engine_index);
        write_log(buffer,0);

        //send message through unnamed pipe to auth engine             
        if(write(auths_pointer[engine_index].fd_pipe[1], request, strlen(request)+1) == -1){
            sprintf(buffer,"[SENDER] ERROR WRITING IN ENGINE %d PIPE", auths_pointer[engine_index].engine_id);
            write_log_error(buffer,0,errno);
        }
    }
    return NULL;
}

void queue_add(int queue_flag, char* message){ //1 for video, 0 for others
    if(queue_flag == 1){
        //queue_v put
        strcpy(queue_v[queue_v_rear], message);
        queue_v_rear = (queue_v_rear + 1) % QUEUE_POS;
        queue_v_count++;
    }else{
        //queue_o put
        strcpy(queue_o[queue_o_rear], message); 
        queue_o_rear = (queue_o_rear + 1) % QUEUE_POS;
        queue_o_count++;
    }
}

char* queue_pop(int queue_flag){ //1 for video, 0 for others
    if(queue_flag == 1){
        //queue_v get
        char* request = queue_v[queue_v_start];
        queue_v_start = (queue_v_start + 1) % QUEUE_POS;
        queue_v_count--;
        return request;
    }else{
        //queue_o get
        char* request = queue_o[queue_o_start];
        queue_o_start = (queue_o_start + 1) % QUEUE_POS;
        queue_o_count--;
        return request;
    }
}

void engine_manager(){ //auth engine manager
    char buffer[MBUF];
    pthread_mutex_lock(&queue_mutex);
    while(!end_flag){
        // condition to wait for sender signaling the queues should be checked
        while(!manager_flag && !end_flag){
            if (DEBUG) write_log("[AUTH MANAGER] WAITING FOR SIGNALS TO MANAGE EXTRA ENGINE..", 1);
            pthread_cond_wait(&manager_cond, &queue_mutex);
        }
        if(end_flag){
            if (DEBUG) write_log("[AUTH MANAGER] LEAVING MONITORING STAGE", 0);
            break;
        }

        block_signals_thread();
        sprintf(buffer,"[AUTH MANAGER] CHECKING QUEUES... VIDEO QUEUE:%d/%d || OTHER QUEUE%d/%d",queue_v_count, QUEUE_POS, queue_o_count, QUEUE_POS);
        if (1) write_log(buffer, 0);

        sprintf(buffer,"[AUTH MANAGER] CREATE_FLAG %d | WHICH_CREATE_FLAG %d", engine_created_flag, which_queue_created_flag);
        if (DEBUG) write_log(buffer, 0);
        manager_flag = 0;        
        
        if(engine_created_flag == 1){
            if(which_queue_created_flag == 1){
                if(queue_v_count <= QUEUE_POS * THRESHOLD_LOW){
                    engine_created_flag = 0;
                    manage_extra_auth(0); 
                }
            }else{
                if(queue_o_count <= QUEUE_POS * THRESHOLD_LOW){
                    engine_created_flag = 0;  
                    manage_extra_auth(0);       
                }
            }
        }else{
            if(queue_v_count >= QUEUE_POS * THRESHOLD_HIGH){
                engine_created_flag = 1;        
                which_queue_created_flag = 1;
                manage_extra_auth(1);
            }else if(queue_o_count >= QUEUE_POS * THRESHOLD_HIGH){
                engine_created_flag = 1;        
                which_queue_created_flag = 0;
                manage_extra_auth(1);
            }
        }
        unblock_signals_thread();
    }
    pthread_mutex_unlock(&queue_mutex);
}

void manage_extra_auth(int create_flag){
    
    char buffer[MBUF];
    pid_t engine_pid;
    if(create_flag == 1){
        pthread_mutex_lock(&sync_pointer->auths_mutex);
        if(auths_pointer[AUTH_SERVERS-1].engine_id == -1){
            // Create new auth engine
            
            sprintf(buffer,"[AUTH MANAGER] -> CREATING ENGINE #%d.", AUTH_SERVERS-1);
            if (DEBUG) write_log(buffer,0);

            engine_pid = fork();
            if(engine_pid == -1){
                write_log_error("[AUTH MANAGER] ERROR CREATING EXTRA ENGINE.", 0, errno);
                // kill(system_pid, SIGINT);
            }
            else if(engine_pid == 0){
                auth_engine(AUTH_SERVERS-1);
                exit(0);
            }
            else{
                write_log("[AUTH MANAGER] -> ENGINE CREATED SUCCESSFULY.", 0);
                auths_pointer[AUTH_SERVERS-1].engine_id = engine_pid;
            }
        }else{
            write_log("[AUTH MANAGER] CAN'T CREATE EXTRA ENGINE. AT MAX CAPACITY",0);
        }
        pthread_mutex_unlock(&sync_pointer->auths_mutex);
    }else{
        if(auths_pointer[AUTH_SERVERS-1].engine_id != -1){
            // Kill last auth engine
            pthread_mutex_lock(&sync_pointer->auths_mutex);
            auths_pointer[AUTH_SERVERS-1].state = -1;
            pthread_mutex_unlock(&sync_pointer->auths_mutex);
            write(auths_pointer[AUTH_SERVERS-1].fd_pipe[1], "terminate", strlen("terminate")+1);
            // while(auths_pointer[AUTH_SERVERS-1].state != 1){ //waits for engine to be available for termination
            
            //     if (DEBUG) write_log("[AUTH MANAGER] WAITING FOR ENGINE TO BE AVAILABLE FOR TERMINATION...",0);
            //     pthread_cond_wait(&sync_pointer->mem_cond4, &sync_pointer->auths_mutex);
            
            //     sprintf(buffer,"[AUTH MANAGER] KILLING ENGINE #%d.", AUTH_SERVERS-1);
            //     write_log(buffer,0);
            // }
            // if(write(auths_pointer[AUTH_SERVERS-1].fd_pipe[1], "terminate", strlen("terminate")+1) == -1){ //sending end message to auth engine
            //     write_log_error("[AUTH MANAGER] ERROR SENDING ENGINE TERMINATION SIGNAL THROUGH PIPE.", 0, errno);
            //     //kill(system_pid, SIGINT); 
            // };
            //kill(auths_pointer[AUTH_SERVERS-1].engine_id, SIGUSR2); //trying to exit engine with signals crashes the OS
            // pthread_mutex_unlock(&sync_pointer->auths_mutex);
            waitpid(auths_pointer[AUTH_SERVERS-1].engine_id, NULL, 0);  

            write_log("[AUTH MANAGER] -> ENGINE KILLED SUCCESSFULY.", 0);
            auths_pointer[AUTH_SERVERS-1].engine_id = -1;
            auths_pointer[AUTH_SERVERS-1].state = -1;
            manager_flag = 0; //makes it go back to condition
        }else{
            write_log("[AUTH MANAGER] CAN'T CREATE EXTRA ENGINE. AT MIN CAPACITY",0);
        }
    }
    
    
}
