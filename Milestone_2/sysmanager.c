//Pedro Lima 2017276603
//Carlos Soares 2020230124

#include "sysmanager.h"

#define NUM_CONFIG_VARS 6
#define NUM_CHILDREN 2
pid_t childs[NUM_CHILDREN];

sig_atomic_t end_flag = 0;
pid_t system_pid;

int MOBILE_USERS, QUEUE_POS, AUTH_SERVERS, AUTH_PROC_TIME, MAX_VIDEO_WAIT, MAX_OTHERS_WAIT;
int shmid;
int mqid;

sync_struct *sync_pointer;
info_struct *info_pointer;
mobile_struct *mobiles_pointer;
auth_struct *auths_pointer;

FILE *log_file;
pthread_mutexattr_t attrmutex;
pthread_condattr_t attrcondv;

int main(int argc, char const *argv[]) { //auth manager
    system_pid = getpid();
    
    if(argc != 2){
        printf("Invalid number of arguments given to %s. Exiting...\n", argv[0]);
        exit(1);
    }

    // Open logs
    log_file = fopen("log.txt","a");
    if (log_file == NULL) {
        write_log_error("[SYS MANAGER] ERROR OPENING LOG FILE. EXITING...", 0, errno);
        exit(1);
    }

    //reads config file (already using logs)
    read_config(argv[1]);

    signal(SIGINT, sys_handler);
    
    sys_config(); //all system configuration/allocation/creation

    write_log("[SYS MANAGER] BOOTED.",1);
    assess_memory();
    
    // creates authorization manager
    childs[0] = fork();
    if(childs[0] == 0){
        signal(SIGINT, SIG_IGN);
        auth_manager();
        exit(0);
    } else if (childs[0] == -1){
        write_log_error("[SYS MANAGER] FAILED TO CREATE AUTH MANAGER PROCESS",1, errno);
        exit(1);
    }

    // creates monitor engine
    childs[1] = fork();
    if(childs[1] == 0){
        signal(SIGINT, SIG_IGN);
        monitor_engine();
        exit(0);
    } else if (childs[1] == -1){
        write_log_error("[SYS MANAGER] FAILED TO CREATE MONITOR ENGINE PROCESS",1, errno);
        exit(1);
    }

    if (DEBUG) write_log("[SYS MANAGER] CHILDREN CREATED",1);

    while(!end_flag){
        //waits for signal (SIGINT)
        pause();
    }
    
    //finish program
    sys_finish();
    sys_cleanup();
    
    return 0;
}


void sys_handler(int sig){ //sets end_flag to 1
    if(sig == SIGINT)
    {
        write_log("[SYS MANAGER] RECEIVED SIGINT. INITALIZING SHUTDOWN...",1);
        // kill(0, SIGUSR1)
        end_flag = 1;
    }
}

void sys_config(){
    char buffer[MBUF];

    int mem_size = sizeof(sync_struct)+sizeof(info_struct)+sizeof(mobile_struct)*MOBILE_USERS+sizeof(auth_struct)*AUTH_SERVERS;
    sprintf(buffer,"[SYS MANAGER] MEMORY SIZE: %d\n",mem_size);
    write_log_nosync(buffer);

    // Create shared memory
    if((shmid = shmget(IPC_PRIVATE, mem_size, IPC_CREAT | 0766)) < 0){
        write_log_error("[SYS MANAGER] FAILED AT SHMGET - ", 0, errno);
        exit(1);
    }
    if((sync_pointer = (sync_struct *) shmat(shmid, NULL, 0)) == (sync_struct*)-1) {
		write_log_error("[SYS MANAGER] FAILED AT SHMAT - ", 0, errno);
		exit(1);
    }

    // Set pointers to shared memory
    info_pointer = (info_struct *) (sync_pointer + 1);
    mobiles_pointer = (mobile_struct *) (info_pointer + 1);
    auths_pointer = (auth_struct *) (mobiles_pointer + MOBILE_USERS);

    //creates mutexes
    pthread_mutexattr_init(&attrmutex);
    pthread_condattr_init(&attrcondv);
    pthread_mutexattr_setpshared(&attrmutex, PTHREAD_PROCESS_SHARED);
    pthread_condattr_setpshared(&attrcondv, PTHREAD_PROCESS_SHARED);

    pthread_mutex_init(&sync_pointer->mem_mutex, &attrmutex);
    pthread_mutex_init(&sync_pointer->log_mutex, &attrmutex);
    pthread_mutex_init(&sync_pointer->auths_mutex, &attrmutex);
    pthread_cond_init(&sync_pointer->mem_cond1, &attrcondv);
    pthread_cond_init(&sync_pointer->mem_cond2, &attrcondv);
    pthread_cond_init(&sync_pointer->mem_cond4, &attrcondv);
    

    // Initialize statistics in memory
    info_pointer->video_stats.auth_requests = 0;
    info_pointer->video_stats.total_data = 0;
    info_pointer->music_stats.auth_requests = 0;
    info_pointer->music_stats.total_data = 0;
    info_pointer->social_stats.auth_requests = 0;
    info_pointer->social_stats.total_data = 0;

    // Initialize mobiles in memory
    for(int i = 0; i < MOBILE_USERS; i++){
        mobiles_pointer[i].client_id = -1;
        mobiles_pointer[i].current_plafond = 0;
        mobiles_pointer[i].start_plafond = 0;
        mobiles_pointer[i].alert_level = 0;
    }

    for(int i = 0; i< AUTH_SERVERS; i++){
        auths_pointer[i].engine_id = -1;
        auths_pointer[i].state = -1;
        auths_pointer[i].fd_pipe[0] = -1;
        auths_pointer[i].fd_pipe[1] = -1;
    }
    
    // creates message queue
    mqid = msgget(ftok(MESSAGE_QUEUE_NAME,MESSAGE_QUEUE_ID), IPC_CREAT|0666);
    if(mqid < 0){
        write_log_error("[SYS MANAGER] ERROR AT MSGGET - \n", 0, errno);
        exit(1);
    }
}

void sys_cleanup(){
    write_log_nosync("[SYS MANAGER] CLEANING UP...");
    
    if (DEBUG) write_log_nosync("[SYS MANAGER] CLEANING PTHREAD RESOURCES...");

    // cleans up pthread resources
    pthread_cond_destroy(&sync_pointer->mem_cond1);
    pthread_cond_destroy(&sync_pointer->mem_cond2);
    pthread_cond_destroy(&sync_pointer->mem_cond4);
    pthread_mutex_destroy(&sync_pointer->log_mutex);
    pthread_mutex_destroy(&sync_pointer->auths_mutex);
    pthread_mutex_destroy(&sync_pointer->mem_mutex);

    pthread_mutexattr_destroy(&attrmutex);
    pthread_condattr_destroy(&attrcondv);

    if (DEBUG) write_log_nosync("[SYS MANAGER] CLEANING MESSAGE QUEUE...");
    
    // cleans up message queue
    if (msgctl(mqid, IPC_RMID, 0) == -1) {
        write_log_error("[SYS MANAGER] ERROR AT MSGCTL - ", 0, errno);
    }

    if (DEBUG) write_log_nosync("[SYS MANAGER] CLEANING SHARED MEMORY...");
    
    // cleans up shared memory
    shmdt(sync_pointer);
    shmctl(shmid, IPC_RMID, 0);


    // closes log file and exits
    write_log_nosync("[SYS MANAGER] CLOSING LOGS AND EXITING...");

    fclose(log_file);
    exit(0);
}

void sys_finish(){
    char buffer[MBUF];
    write_log_nosync("[SYS MANAGER] WAITING FOR RESOURCES...");

    // sends end signal to other processes (to end them)
    for(int i = 0; i < NUM_CHILDREN; i++){
        kill(childs[i], SIGUSR1);
        sprintf(buffer,"[SYS MANAGER] SENT SIGUSR1 TO CHILD (ID = %d)", childs[i]);
        if (DEBUG) write_log(buffer,1);
    }

    // waits for other processes to end
    for(int i = 0; i < NUM_CHILDREN; i++){
        waitpid(childs[i], NULL, 0);
        sprintf(buffer,"[SYS MANAGER] CHILD PROCESS (ID = %d) JOINED",childs[i]);
        if (DEBUG) write_log(buffer,1);
    }

    write_log_nosync("[SYS MANAGER] WAITING FOR RESOURCES...");
}

//reads config file
void read_config(const char *config_file){
    char buffer[MBUF];
    FILE *file = fopen(config_file,"r");
    if (file == NULL) {
        write_log_error("[SYS MANAGER] ERROR OPENING COONFIG FILE - ", 0, errno);
        exit(1);
    }

    int config_values[NUM_CONFIG_VARS];

    char line[MBUF];
    int count = 0;
    while(fgets(line, sizeof(line), file) != NULL && count < 6){
        // Remove newline character from line
        line[strcspn(line, "\n")] = '\0';

        // Convert line to integer
        char *end;
        errno = 0;
        config_values[count] = strtol(line, &end, 10);
        if (errno != 0 || *end != '\0' || config_values[count] < 0) {
            write_log_error("[SYS MANAGER] ERROR READING CONFIG - ", 0, errno);
            exit(1);
        }

        count++;
    }

    // Check if count < NUM_CONFIG_VARS (currently ignores extra lines)
    if(count != NUM_CONFIG_VARS){
        write_log_nosync("[SYS MANAGER] WRONG NUMBER OF CONFIG VARS PROCESSED -");
        exit(1);
    }

    fclose(file);

    // Check if specific config variable conditions are met
    if(config_values[2] == 0 || config_values[3] == 0 || config_values[4] == 0 || config_values[5] == 0){
        write_log_nosync("[SYS MANAGER] ERROR AT CONFIG - 0 VALS.");
        exit(1);
    }

    MOBILE_USERS = config_values[0];
    QUEUE_POS = config_values[1];
    AUTH_SERVERS = config_values[2];
    AUTH_PROC_TIME = config_values[3];
    MAX_VIDEO_WAIT = config_values[4];
    MAX_OTHERS_WAIT = config_values[5];
    
    sprintf(buffer,"[SYS MANAGER] CONFIG WAS SUCCESSFUL. USERS: %d, QUEUE_SIZE: %d, ENGINES: %d, PROC_TIME: %d, MAX_VIDEO_WAIT: %d, MAX_OTHERS_WAIT: %d\n", MOBILE_USERS, QUEUE_POS, AUTH_SERVERS, AUTH_PROC_TIME, MAX_VIDEO_WAIT, MAX_OTHERS_WAIT);
    write_log_nosync(buffer);
}

void assess_memory(){
    char buffer[MBUF];
    if (DEBUG){
        sprintf(buffer,"STATS (DATA): \t%d V %d M %d S",info_pointer->video_stats.total_data, info_pointer->music_stats.total_data, info_pointer->social_stats.total_data);
        write_log(buffer,1);
        sprintf(buffer,"STATS (REQUESTS): \t%d V %d M %d S",info_pointer->video_stats.auth_requests, info_pointer->music_stats.auth_requests, info_pointer->social_stats.auth_requests);
        write_log(buffer,1);

        for(int i = 0; i < MOBILE_USERS; i++){
            sprintf(buffer,"m[%d]id:\t%d",i,mobiles_pointer[i].client_id);
            write_log(buffer,1);
            sprintf(buffer,"m[%d]data:\t%d",i,mobiles_pointer[i].current_plafond);
            write_log(buffer,1);
        }
        for(int i = 0; i < AUTH_SERVERS; i++){
            sprintf(buffer,"e[%d]id:\t%d",i,auths_pointer[i].engine_id);
            write_log(buffer,1);
            sprintf(buffer,"e[%d]state:\t%d",i,auths_pointer[i].state);
            write_log(buffer,1);
            sprintf(buffer,"e[%d]pipe1:\t%d",i,auths_pointer[i].fd_pipe[1]);
            write_log(buffer,1);
        }
    }
    
}