//Pedro Lima 2017276603
//Carlos Soares 2020230124

#ifndef AUTHMANAGER_H
#define AUTHMANAGER_H

#include "main.h"

void auth_manager();

void auth_engine();

void auth_engine_reader(pid_t request_id, int request_size, int request_type, int* index, int* valid);

void auth_engine_writer(pid_t request_id, int request_size, int request_type, int index, int valid, char* message);

void *auth_sender(void *arg);

void *auth_receiver(void *arg);

#define WRITERS_MEM "/writer_mem"

#endif