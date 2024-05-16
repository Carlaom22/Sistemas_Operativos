//Pedro Lima 2017276603
//Carlos Soares 2020230124

#ifndef LOGS_H
#define LOGS_H

#include "utils.h"

extern FILE *log_file;

void write_log(char *str, int block_type);
void write_log_nosync(char *str);
void write_log_error(char *str, int block_type, int error_code);
void write_log_error_nosync(char *str, int error_code);

#endif