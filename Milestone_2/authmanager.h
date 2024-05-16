//Pedro Lima 2017276603
//Carlos Soares 2020230124

#ifndef AUTHMANAGER_H
#define AUTHMANAGER_H

#include "authengine.h"
#include "structs.h"
#include "utils.h"
#include "logs.h"

//auth manager
void auth_manager();
void auth_handler(int sig);
void auth_setup();
void auth_cleanup();

//extra engine thread (main thread)
void engine_manager();
void manage_extra_auth(int create_flag); 

//auth threads
void* receiver();
void* sender();

// queue utils
void queue_add(int queue_flag, char* message);
char* queue_pop(int queue_flag);

#endif