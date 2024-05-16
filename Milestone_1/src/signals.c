//Pedro Lima 2017276603
//Carlos Soares 2020230124

#include "signals.h"

void finish(){
    END_FLAG = 1;
    //send kill signal to itself    
    kill(getpid(), SIGKILL);
}