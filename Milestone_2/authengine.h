//Pedro Lima 2017276603
//Carlos Soares 2020230124

#ifndef AUTHENGINE_H
#define AUTHENGINE_H

#include "structs.h"
#include "utils.h"
#include "logs.h"

//auth engine
void auth_engine(int auth_index);

//auth engine functions
void process_request(char* pipebuffer, int auth_index);
int register_user(int user_id, int request_size);
int delete_user(int user_id);
int authorize_data_request(int user_id, stats_struct* stat, int request_size);

//request utils
int get_request_id(char* request);
int get_request_type(char* request);
int get_request_last(char* request);


#endif