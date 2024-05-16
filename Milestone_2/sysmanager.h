//Pedro Lima 2017276603
//Carlos Soares 2020230124

#ifndef SYSMANAGER_H
#define SYSMANAGER_H

#include "monitorengine.h"
#include "authmanager.h"
#include "structs.h"
#include "utils.h"
#include "logs.h"

void sys_cleanup();
void sys_finish();
void sys_config();
void read_config(const char *config_file);
void sys_handler(int sig);

#endif