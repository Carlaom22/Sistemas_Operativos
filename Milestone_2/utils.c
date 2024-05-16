//Pedro Lima 2017276603
//Carlos Soares 2020230124

#include "utils.h"

void block_signals_thread(){
    //block sigusr1 and sigint signals in thread
    sigset_t set;
    sigemptyset(&set);
    sigaddset(&set, SIGUSR1);
    sigaddset(&set, SIGINT);
    pthread_sigmask(SIG_BLOCK, &set, NULL);
}

void unblock_signals_thread(){
    //unblock sigusr1 and sigint signals in thread
    sigset_t set;
    sigemptyset(&set);
    sigaddset(&set, SIGUSR1);
    sigaddset(&set, SIGINT);
    pthread_sigmask(SIG_UNBLOCK, &set, NULL);
}

void block_signals(){
    //block sigusr1 and sigint signals in process
    sigset_t set;
    sigemptyset(&set);
    sigaddset(&set, SIGUSR1);
    sigaddset(&set, SIGINT);
    sigprocmask(SIG_BLOCK, &set, NULL);
}

void unblock_signals(){
    //unblock sigusr1 and sigint signals in process
    sigset_t set;
    sigemptyset(&set);
    sigaddset(&set, SIGUSR1);
    sigaddset(&set, SIGINT);
    sigprocmask(SIG_UNBLOCK, &set, NULL);
}