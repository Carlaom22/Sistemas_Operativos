//Pedro Lima 2017276603
//Carlos Soares 2020230124

#ifndef STRUCTS_H
#define STRUCTS_H

#define DEBUG 0

#include <time.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <errno.h>
#include <pthread.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <sys/ipc.h>
#include <sys/msg.h>
#include <sys/shm.h>
#include <sys/stat.h>
#include <signal.h>
#include <fcntl.h>
// #include <bits/sigaction.h>
// #include <bits/types/sigset_t.h>
#include <sys/select.h>

#define PIPE_USER "pipe_user"
#define PIPE_ADMIN "pipe_admin"

#define MESSAGE_QUEUE_NAME "msg_queue"
#define MESSAGE_QUEUE_ID 1234

#define GBUF 512
#define MBUF 256
#define KBUF 64

typedef struct {
	int total_data;
	int auth_requests;
} stats_struct;

typedef struct {
    pthread_mutex_t log_mutex;
	pthread_mutex_t mem_mutex;
	pthread_mutex_t auths_mutex;
	pthread_cond_t mem_cond1;
	pthread_cond_t mem_cond2;
	pthread_cond_t mem_cond4;
	//conditions for monitor engine
} sync_struct;

typedef struct {
	stats_struct video_stats;
	stats_struct music_stats;
	stats_struct social_stats;
} info_struct;

typedef struct {
	pid_t client_id; //pid client process
    int start_plafond; 
	int current_plafond;
	int alert_level;
} mobile_struct;

typedef struct{
    pid_t engine_id;
    int state; //-1 if not created, 0 if created and busy, 1 if created and free
    int fd_pipe[2];
}auth_struct;

typedef struct {
	long pid;
	info_struct stats;
} backoffice_msg;

typedef struct {
    long pid; //always needs a long
    int alert_val;
} mobile_msg;

extern pid_t system_pid;
extern int mqid;
extern int shmid;
extern int MOBILE_USERS, QUEUE_POS, AUTH_SERVERS, AUTH_PROC_TIME, MAX_VIDEO_WAIT, MAX_OTHERS_WAIT;
extern sig_atomic_t end_flag;

extern sync_struct *sync_pointer;
extern info_struct *info_pointer;
extern mobile_struct *mobiles_pointer;
extern auth_struct *auths_pointer;


#endif