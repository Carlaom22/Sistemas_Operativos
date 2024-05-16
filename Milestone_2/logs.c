//Pedro Lima 2017276603
//Carlos Soares 2020230124

#include "logs.h"

//write log to file, with signal blocking options and mandatory syncronization
void write_log(char *str, int block_type){

    //block signals
    if (block_type == 1){
        block_signals();
    }
    else if (block_type == 2){
        block_signals_thread();
    }
    //syncronize the log file with all programs that are using it
    pthread_mutex_lock(&sync_pointer->log_mutex);

    //write log
    write_log_nosync(str);
    fprintf(log_file, "\n"); //adds closing line to logfile

    //close syncronization
    pthread_mutex_unlock(&sync_pointer->log_mutex);
    //unblock signals
    if (block_type == 1){
        unblock_signals();
    }
    else if (block_type == 2){
        unblock_signals_thread();
    }   
}

//write log to file without any syncronization
void write_log_nosync(char *str){
    char msg[MBUF];
    int processpid = getpid();
    time_t t = time(NULL);
    struct tm tm = *localtime(&t);
    
    sprintf(msg, "[%d-%02d-%02d %02d:%02d:%02d] [%d] %s", tm.tm_year + 1900, tm.tm_mon + 1, tm.tm_mday, tm.tm_hour, tm.tm_min, tm.tm_sec, processpid, str);
    fprintf(log_file, "%s",msg);
    printf("%s\n",msg);
    
}

void write_log_error(char *str, int block_type, int error_code){
    //block signals
    if (block_type == 1){
        block_signals();
    }
    else if (block_type == 2){
        block_signals_thread();
    }
    //syncronize the log file with all programs that are using it
    pthread_mutex_lock(&sync_pointer->log_mutex);

    write_log_error_nosync(str, error_code);

    //close syncronization
    pthread_mutex_unlock(&sync_pointer->log_mutex);
    //unblock signals
    if (block_type == 1){
        unblock_signals();
    }
    else if (block_type == 2){
        unblock_signals_thread();
    }  

}

void write_log_error_nosync(char *str, int error_code){
    write_log_nosync(str);
    char *message = strerror(error_code);
    fprintf(log_file, "%s\n", message); //adds error msg + closing line to logfile



}