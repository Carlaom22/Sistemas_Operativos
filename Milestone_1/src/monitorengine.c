//Pedro Lima 2017276603
//Carlos Soares 2020230124

#include "monitorengine.h"

#include "sysmanager.h" 
#include "sharedm.h"
#include "signals.h" //requires END_FLAG
#include "logs.h"


void signal_handler_monitor(int signum);

void monitor_engine(){
    signal(SIGUSR1, signal_handler_monitor);
    write_log("Monitor Engine started");
    while(1){
        //will be substituted by the SIGINT signal that will terminate the monitor engine
        sleep(40);
        printf("Monitor Engine running...\n");
    }
}

void signal_handler_monitor(int signum){
    if(signum == SIGUSR1){
        write_log("Received SIGUSR1 in monitor engine. Exiting...");
        END_FLAG = 1;
        write_log("... All resources cleaned. Exiting monitor engine.");
        exit(0);
    }
}