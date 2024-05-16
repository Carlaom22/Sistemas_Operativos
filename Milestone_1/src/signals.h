//Pedro Lima 2017276603
//Carlos Soares 2020230124

#ifndef SIGNALS_H
#define SIGNALS_H

#include "main.h"

#include <signal.h>

extern volatile sig_atomic_t END_FLAG;

void finish();

#endif