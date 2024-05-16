//Pedro Lima 2017276603
//Carlos Soares 2020230124

#ifndef SHAREDM_H
#define SHAREDM_H

#include "main.h"

#include <semaphore.h>

extern sem_t *mutex_mem;
extern int shmid;
extern int MOBILE_USERS, QUEUE_POS, AUTH_SERVERS, AUTH_PROC_TIME, MAX_VIDEO_WAIT, MAX_OTHERS_WAIT;

extern info_struct *stats;
extern mobile_struct *mobiles;

#define MUTEX_MEM "/mutex_mem"
#define MUTEX_READERS "/mutex_readers"
#define SHSTRING "/tmp"

#endif