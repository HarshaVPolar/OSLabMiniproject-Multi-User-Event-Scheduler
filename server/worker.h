#ifndef WORKER_H
#define WORKER_H
#include "../common/events.h"

void execute_task(Event e);
void log_event(const char* msg);

#endif