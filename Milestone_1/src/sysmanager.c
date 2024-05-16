//Pedro Lima 2017276603
//Carlos Soares 2020230124

#include "sysmanager.h"

#include "authmanager.h"
#include "monitorengine.h"
#include "sharedm.h"
#include "signals.h" //requires a finish() function
#include "logs.h"


pid_t childs[2];
info_struct *stats;
mobile_struct *mobiles;

//all these global variables are extern variables in sharedm.h or signals.h
volatile sig_atomic_t END_FLAG;
int shmid;
int MOBILE_USERS, QUEUE_POS, AUTH_SERVERS, AUTH_PROC_TIME, MAX_VIDEO_WAIT, MAX_OTHERS_WAIT;
sem_t *mutex_log, *mutex_mem;

void read_config(const char *config_file);
void show_shm();
void signal_handler(int signum);

int main(int argc, char const *argv[]){
    signal(SIGINT, signal_handler);
    if(argc != 2){
        write_log("Invalid number of arguments");
        exit(1);
    }

    //reads config file
    read_config(argv[1]);

    // creates semaphores of log for all of these
    sem_unlink(MUTEX_LOG);
	mutex_log=sem_open(MUTEX_LOG,O_CREAT|O_EXCL,0700,1); // create semaphore
	if(mutex_log==SEM_FAILED){
		perror("Failure creating the semaphore MUTEX_LOG");
        finish();
        exit(1);
	}

	sem_unlink(MUTEX_MEM);
	mutex_mem=sem_open(MUTEX_MEM,O_CREAT|O_EXCL,0700,1); // create semaphore
	if(mutex_mem==SEM_FAILED){
		perror("Failure creating the semaphore MUTEX_MEM");
        finish();
        exit(1);
	}

    write_log("Semaphores created successfully");

    shm_unlink(SHSTRING); //limpa shared memory
    //cria shared mem (info + NMAX_USERS * client_struct )
    shmid = shm_open(SHSTRING, O_CREAT|O_RDWR, 0700);
    if(shmid == -1){
        perror("Failed to create shared memory");
        finish();
        exit(1);
    }
    write_log("Shared memory created successfully");

    if(ftruncate(shmid, sizeof(info_struct) + sizeof(mobile_struct)*MOBILE_USERS) == -1){
        perror("Failed to truncate shared memory");
        finish();
        exit(1);
    }

    stats = mmap(NULL, sizeof(info_struct) + sizeof(mobile_struct)*MOBILE_USERS, PROT_READ|PROT_WRITE, MAP_SHARED, shmid, 0);
    if(stats == MAP_FAILED){
        perror("Failed to map shared memory");
        finish();
        exit(1);
    }

    // initializes shared memory
    stats->readers = 0;

    // creates authorization manager
    if((childs[0] = fork()) == 0){
        signal(SIGINT, SIG_IGN);
        auth_manager();
        exit(0);
    } else if (childs[0] == -1){
        perror("Failed to create authorization manager process");
        finish();
        exit(1);
    }

    // creates monitor engine
    if((childs[1] = fork()) == 0){
        signal(SIGINT, SIG_IGN);
        monitor_engine();
        exit(0);
    } else if (childs[1] == -1){
        perror("Failed to create monitor engine process");
        finish();
        exit(1);
    }

    // creates message queue

    // waits for other processes to end
    for(size_t i = 0; i < sizeof(childs)/sizeof(childs[0]); i++){
        waitpid(childs[i], NULL, 0);
    }

    kill(getpid(), SIGINT); //calls signal handler (to clean resources and end program
    return 0;
}

//reads config file
void read_config(const char *config_file){
    char message[MBUF];
    FILE *file = fopen(config_file,"r");
    if (file == NULL) {
        write_log("Error opening config file");
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
            write_log("Error reading config file: values must be non-negative integers");
            exit(1);
        }

        count++;
    }

    // Check if count < NUM_CONFIG_VARS (currently ignores extra lines)
    if(count != NUM_CONFIG_VARS){
        write_log("Error reading config file: incorrect number of lines");
        exit(1);
    }

    fclose(file);

    // Check if specific config variable conditions are met
    if(config_values[2] == 0 || config_values[3] == 0 || config_values[4] == 0 || config_values[5] == 0){
        write_log("Error reading config file: invalid values");
        exit(1);
    }

    MOBILE_USERS = config_values[0];
    QUEUE_POS = config_values[1];
    AUTH_SERVERS = config_values[2];
    AUTH_PROC_TIME = config_values[3];
    MAX_VIDEO_WAIT = config_values[4];
    MAX_OTHERS_WAIT = config_values[5];
    
    sprintf(message,"Config file read successfully. MOBILE_USERS: %d, QUEUE_POS: %d, AUTH_SERVERS: %d, AUTH_PROC_TIME: %d, MAX_VIDEO_WAIT: %d, MAX_OTHERS_WAIT: %d", MOBILE_USERS, QUEUE_POS, AUTH_SERVERS, AUTH_PROC_TIME, MAX_VIDEO_WAIT, MAX_OTHERS_WAIT);
    write_log(message);
}

//shows shared memory
void show_shm(){
    printf("Showing shm (integer by integer):\n");
    int *p = (int *) stats;
    printf("Stats: %d\n", stats->readers);
    *p += sizeof(info_struct);

    //assumes pid_t is an int
    for(size_t i = 0; i < ((sizeof(mobile_struct)*MOBILE_USERS)/sizeof(int)); i++){
        printf("%d  ", *(p+i));
        if(i%2==1) printf("\n");
    }
  printf("\n");
}

void signal_handler(int signum) {
    if(signum == SIGINT) {
        END_FLAG = 1;
        write_log("Received SIGINT in System Manager. Initiating shutdown...");

        // sends end signal to other processes (to end them)
        for(size_t i = 0; i < sizeof(childs)/sizeof(childs[0]); i++){
            kill(childs[i], SIGUSR1);
        }

        // waits for other processes to end
        for(size_t i = 0; i < sizeof(childs)/sizeof(childs[0]); i++){
            waitpid(childs[i], NULL, 0);
        }

        write_log("Successfully shut down all child processes. Cleaning resources...");
        sem_close(mutex_log);
        sem_close(mutex_mem);
        sem_unlink(MUTEX_LOG);
        sem_unlink(MUTEX_MEM);
        shmdt(stats);
        shmctl(shmid, IPC_RMID, NULL);

        // cleans message queue
        write_log("Resources cleaned. Closing program!");
        exit(0);
    }
}
