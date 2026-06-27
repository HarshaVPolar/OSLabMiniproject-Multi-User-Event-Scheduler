#ifndef EVENTS_H
#define EVENTS_H

typedef struct {
    int id;
    char filename[100];
    int interval;
    long created_at;
    long last_run;
    int priority;
    char created_by[50];
} Event;

// function comes AFTER struct
int parse_event(char* line, Event* e);

#endif