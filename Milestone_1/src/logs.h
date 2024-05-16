//Pedro Lima 2017276603
//Carlos Soares 2020230124

#ifndef LOGS_H
#define LOGS_H

#include "main.h"

#include <time.h>
#include <semaphore.h>

extern sem_t *mutex_log;
#define MUTEX_LOG "/mutex_log"

void write_log(char *str);

#endif