//Pedro Lima 2017276603
//Carlos Soares 2020230124

#include "authengine.h"


void auth_engine(int auth_index){
    unblock_signals_thread();
    block_signals();

    fd_set read_set;
    char buffer[MBUF], pipebuffer[MBUF];
    int bytes_read;
    int *fd_pipe = auths_pointer[auth_index].fd_pipe;

    sprintf(buffer, "[ENGINE %d]: STARTED...", auth_index);
    if (DEBUG) write_log(buffer,1);
    if (DEBUG && 0){
        if (isatty(auths_pointer[auth_index].fd_pipe[0])) {
            if (DEBUG) printf("fd_pipe[0] is connected to a terminal\n");
        } else {
            if (DEBUG) printf("fd_pipe[0] is not connected to a terminal\n");
        }

        if (isatty(auths_pointer[auth_index].fd_pipe[1])) {
            if (DEBUG) printf("fd_pipe[1] is connected to a terminal\n");
        } else {
        if (DEBUG) printf("fd_pipe[1] is not connected to a terminal\n");
    }
    }

    while(!end_flag){
        
        sprintf(buffer, "[ENGINE %d] WAITING...", auth_index); //just to check before locking
        if (DEBUG) write_log(buffer,0);

        //change state to available

        pthread_mutex_lock(&sync_pointer->auths_mutex); //test
        if(auths_pointer[auth_index].state == -1){  
            pthread_mutex_unlock(&sync_pointer->auths_mutex);
            break;
        }
        auths_pointer[auth_index].state = 1; //state is set to 0 by sender
        sprintf(buffer, "[ENGINE %d] IS AVAILABLE", auth_index);
        write_log(buffer,0); //mandatory log 
        pthread_cond_signal(&sync_pointer->mem_cond1); //signals sender an engine is available
        pthread_mutex_unlock(&sync_pointer->auths_mutex);
        
        // Selects pipe, used for more control over what engine does
        FD_ZERO(&read_set);
		FD_SET(fd_pipe[0], &read_set);
        if (select(fd_pipe[0]+1, &read_set, NULL, NULL, NULL) > 0){
            if (FD_ISSET(fd_pipe[0], &read_set)){
                bytes_read = read(fd_pipe[0], pipebuffer, sizeof(pipebuffer));                
                sprintf(buffer,"[ENGINE %d] IS NOW UNAVAILABLE", auth_index);
                write_log(buffer,0);
                
                usleep(AUTH_PROC_TIME * 1000);  //sleep for PROC_TIME as per config file

                if (bytes_read == -1) {
                    sprintf(buffer, "[ENGINE %d] ERROR READING FROM PIPE", auth_index);
                    if (DEBUG) write_log_error(buffer,0,errno);
                    break;
                }
                pipebuffer[bytes_read] = '\0';
                if (strcmp(pipebuffer, "terminate") == 0) {
                    sprintf(buffer,"[ENGINE %d] RECEIVED ORDER TO TERMINATE", auth_index);
                    if (DEBUG) write_log(buffer,0);
                    break;
                }
                else process_request(pipebuffer, auth_index); //actual request processing
                }

                // //send response to sender thread signaling finished processing
                // sprintf(buffer, "[ENGINE %d] Sending response through pipe", auth_index);
                // if (DEBUG) write_log(buffer,0);
                // write(fd_pipe[1], "fin", strlen("fin")+1);
            }
        }

    sprintf(buffer, "[ENGINE %d] ...EXITING", auth_index);
    if (1) write_log(buffer,0); //mandatory log 
    exit(0);
}

void process_request(char* pipebuffer, int auth_index){
    backoffice_msg msg;
    char buffer[MBUF];
    sprintf(buffer, "[ENGINE %d] PROCESSING REQUEST...", auth_index);
    if (DEBUG) write_log(buffer,0);

    int request_id = get_request_id(pipebuffer);
    int request_size = get_request_last(pipebuffer);
    int request_type = get_request_type(pipebuffer);
    int request_response;
    if(request_id == -1 || request_size == -1 || request_type == -1){
        sprintf(buffer, "[ENGINE %d] INVALID REQUEST, PASSING... %d %d %d", auth_index, request_id, request_size, request_type);
        write_log(buffer, 0);
    }
    // mobile user requests
    pthread_mutex_lock(&sync_pointer->mem_mutex);
    switch(request_type){
        case 0:
            request_response = register_user(request_id, request_size);
            if (request_response == -1) sprintf(buffer, "[ENGINE %d] REGISTER REQUEST (ID = %d) PROCESSING COMPLETE (FAILED)", auth_index, request_id);
            else sprintf(buffer, "[ENGINE %d] REGISTER REQUEST (ID = %d) PROCESSING COMPLETE (SUCCESS)", auth_index, request_id);
            break;
        case 1:
            request_response = authorize_data_request(request_id, &info_pointer->video_stats, request_size);
            if (request_response == -1) sprintf(buffer, "[ENGINE %d] VIDEO AUTH REQUEST (ID = %d) PROCESSING COMPLETE (FAILED)", auth_index, request_id);
            else if (request_response == 0) sprintf(buffer, "[ENGINE %d] VIDEO AUTH REQUEST (ID = %d) PROCESSING COMPLETE (NO DATA LEFT)", auth_index, request_id);
            else sprintf(buffer, "[ENGINE %d] VIDEO AUTH (ID = %d) PROCESSING COMPLETE (SUCCESS)", auth_index, request_id);
            break;
        case 2:
            request_response = authorize_data_request(request_id, &info_pointer->music_stats, request_size);
            if (request_response == -1) sprintf(buffer, "[ENGINE %d] MUSIC AUTH REQUEST (ID = %d) PROCESSING COMPLETE (FAILED)", auth_index, request_id);
            else if (request_response == 0) sprintf(buffer, "[ENGINE %d] MUSIC AUTH REQUEST (ID = %d) PROCESSING COMPLETE (NO DATA LEFT)", auth_index, request_id);
            else sprintf(buffer, "[ENGINE %d] MUSIC AUTH (ID = %d) PROCESSING COMPLETE (SUCCESS)", auth_index, request_id);
            break;
        case 3:
            request_response = authorize_data_request(request_id, &info_pointer->social_stats, request_size);
            if (request_response == -1) sprintf(buffer, "[ENGINE %d] SOCIAL AUTH REQUEST (ID = %d) PROCESSING COMPLETE (FAILED)", auth_index, request_id);
            else if (request_response == 0) sprintf(buffer, "[ENGINE %d] SOCIAL AUTH REQUEST (ID = %d) PROCESSING COMPLETE (NO DATA LEFT)", auth_index, request_id);
            else sprintf(buffer, "[ENGINE %d] SOCIAL AUTH (ID = %d) PROCESSING COMPLETE (SUCCESS)", auth_index, request_id);
            break;
        case 4:
            request_response = delete_user(request_id);
            if (request_response == -1) sprintf(buffer, "[ENGINE %d] DELETE REQUEST (ID = %d) PROCESSING COMPLETE (FAILED)", auth_index, request_id);
            else sprintf(buffer, "[ENGINE %d] DELETE REQUEST (ID = %d) PROCESSING COMPLETE (SUCCESS)", auth_index, request_id);
            break;
        case 5:
            //reset command
            info_pointer->video_stats.auth_requests = 0;
            info_pointer->video_stats.total_data = 0;
            info_pointer->music_stats.auth_requests = 0;
            info_pointer->music_stats.total_data = 0;
            info_pointer->social_stats.auth_requests = 0;
            info_pointer->social_stats.total_data = 0;
            sprintf(buffer, "[ENGINE %d] RESET REQUEST (ID = %d) PROCESSING COMPLETE (SUCCESS)", auth_index, request_id);
            break;
        case 6:
            //send data command
            msg.pid = 1;
            msg.stats = *info_pointer;

            if (msgsnd(mqid, &msg, sizeof(backoffice_msg) - sizeof(long), IPC_NOWAIT) == -1) {
                sprintf(buffer, "[ENGINE %d] STATISTICS REQUEST (ID = %d) PROCESSING COMPLETE (FAILED)", auth_index, request_id);
            }
            sprintf(buffer, "[ENGINE %d] STATISTICS REQUEST (ID = %d) PROCESSING COMPLETE (SUCCESS)", auth_index, request_id);
            break;

        default:
            sprintf(buffer, "[ENGINE %d] BAD REQUEST", auth_index);
            break;
    }
    pthread_mutex_unlock(&sync_pointer->mem_mutex);


    write_log(buffer, 0);
}

int register_user(int user_id, int request_size){
    for(int i = 0; i < MOBILE_USERS; i++){
        if(mobiles_pointer[i].client_id == -1){
            mobiles_pointer[i].client_id = user_id;
            mobiles_pointer[i].current_plafond = request_size;
            mobiles_pointer[i].start_plafond = request_size;
            return 0;
        }
    }
    return -1;
}

int delete_user(int user_id){
    for(int i = 0; i < MOBILE_USERS; i++){
        if(mobiles_pointer[i].client_id == user_id){
            mobiles_pointer[i].client_id = -1;
            mobiles_pointer[i].current_plafond = 0;
            mobiles_pointer[i].start_plafond = 0;
            mobiles_pointer[i].alert_level = 0;
            return 0;
        }
    }
    return -1;
}

int authorize_data_request(int user_id, stats_struct* stat, int request_size){
    for(int i = 0; i < MOBILE_USERS; i++){
        if(mobiles_pointer[i].client_id == user_id){
                stat->auth_requests++;
                stat->total_data += request_size;
                if(mobiles_pointer[i].current_plafond < request_size){
                    mobiles_pointer[i].current_plafond = 0; 
                    pthread_cond_signal(&sync_pointer->mem_cond2); 
                    return 0;
                }
                else{
                    mobiles_pointer[i].current_plafond -= request_size;
                    pthread_cond_signal(&sync_pointer->mem_cond2); 
                    return 1;
                }
        }
    }
    return -1;
}

int get_request_id(char* request){
    char *token;
    char *request_copy = strdup(request);
    int id = -1;
    token = strtok(request_copy, "#");
    if (token != NULL) id = atoi(token);
    free(request_copy);
    return id;
}

int get_request_type(char* request){
    char *hash_ptr = strchr(request, '#');
    if (hash_ptr != NULL){
        if (*(hash_ptr + 1) == 'V') return 1; //video stats
        else if (*(hash_ptr + 1) == 'M') return 2; //music stats
        else if (*(hash_ptr + 1) == 'S') return 3; //social stats
        else if (*(hash_ptr + 1) == '0') return 4; //delete user
        else if (*(hash_ptr + 1) == 'r') return 5; //reset stats
        else if (*(hash_ptr + 1) == 'd') return 6; //send data stats
        else return 0; //register user
    }
    return -1;
}

int get_request_last(char* request){

    char *token;
    char *request_copy = strdup(request);
    int type = get_request_type(request);
    int last = -1;

    if(type > 0 && type < 4){
        token = strtok(request_copy, "#");
        token = strtok(NULL, "#");
        token = strtok(NULL, "#");
        if (token != NULL) last = atoi(token);
    }
    else if (type == 0 || type >= 4){
        token = strtok(request_copy, "#");
        token = strtok(NULL, "#");
        if (token != NULL) last = atoi(token);
    }
    free(request_copy);
    return last;
}