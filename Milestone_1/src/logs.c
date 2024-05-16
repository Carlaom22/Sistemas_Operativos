//Pedro Lima 2017276603
//Carlos Soares 2020230124

#include "main.h"
#include "logs.h"
#include "sharedm.h"

void write_log(char *str){
    char timestamp[MBUF];

    //syncronize the log file with all programs that are using it
    sem_wait(mutex_log);

    //create log string with timestamp
    time_t t = time(NULL);
    struct tm tm = *localtime(&t);
    
    sprintf(timestamp, "[%d-%02d-%02d %02d:%02d:%02d]", tm.tm_year + 1900, tm.tm_mon + 1, tm.tm_mday, tm.tm_hour, tm.tm_min, tm.tm_sec);
    
    FILE *file = fopen("../log.txt","a");
    fprintf(file, "%s %s\n",timestamp,str);
    fclose(file);
    
    printf("%s %s\n",timestamp,str);
    
    //fechar a sincronização
    sem_post(mutex_log);
}