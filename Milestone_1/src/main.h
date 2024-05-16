//Pedro Lima 2017276603
//Carlos Soares 2020230124

#ifndef MAIN_H
#define MAIN_H

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <errno.h>

typedef struct {
	int readers;
} info_struct;

typedef struct {
	pid_t client_id; //pid client process
    int plafond; 
} mobile_struct;

typedef struct {
} auth_struct;


#define MBUF 1024
#define SBUF 256
#define TBUF 64

#endif
