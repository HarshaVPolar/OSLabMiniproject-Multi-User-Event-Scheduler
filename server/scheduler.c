#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <fcntl.h>
#include <time.h>
#include <pthread.h>
#include "scheduler.h"
#include "worker.h"
#include "util.h"

#define TASK_FILE "data/tasks.txt"
#define MAX_TASKS 100



/* ---------------- PARSE EVENT ---------------- */
int parse_event(char* line, Event* e) {
    return sscanf(line, "%d,%[^,],%d,%ld,%ld,%d,%[^,\n]",
                  &e->id,
                  e->filename,
                  &e->interval,
                  &e->created_at,
                  &e->last_run,
                  &e->priority,
                  e->created_by);
}

/* ---------------- priority-BASED SORT ---------------- */
void sort_by_priority(Event events[], int n) {
    for (int i = 0; i < n - 1; i++) {
        for (int j = 0; j < n - i - 1; j++) {
            if (events[j].priority < events[j + 1].priority) {
                Event temp = events[j];
                events[j] = events[j + 1];
                events[j + 1] = temp;
            }
        }
    }
}

/* ---------------- SCHEDULER LOOP ---------------- */
void* scheduler_loop(void* arg) {
    while (1) {
        FILE* file = fopen(TASK_FILE, "r");
        if (!file) {
            sleep(1);
            continue;
        }

        Event events[MAX_TASKS];
        int count = 0;

        char line[256];
        while (fgets(line, sizeof(line), file) && count < MAX_TASKS) {
            if (parse_event(line, &events[count]) == 7) {
                count++;
            }
        }
        fclose(file);

        time_t now = time(NULL);

        // collect ready tasks
        Event ready[MAX_TASKS];
        int rcount = 0;

        for (int i = 0; i < count; i++) {
            if (now - events[i].last_run >= events[i].interval) {
                ready[rcount++] = events[i];
            }
        }
        
        //aging to prevent starvation
        for (int i = 0; i < rcount; i++) {
            int waiting_time = now - ready[i].last_run;
            int aging = waiting_time / 5;

            ready[i].priority += aging;
        }

        // sort by priority (SJF-like)
        sort_by_priority(ready, rcount);

        // simulate 2-core CPU
        int cores = 2;
        int executed = 0;

        for (int i = 0; i < rcount && executed < cores; i++) {
            execute_task(ready[i]);

            // update original event
            for (int j = 0; j < count; j++) {
                if (events[j].id == ready[i].id) {
                    events[j].last_run = now;
                    break;
                }
            }

            executed++;
        }

        // rewrite file safely
        int fd = open(TASK_FILE, O_WRONLY | O_TRUNC);
        if (fd >= 0) {
            lock_file(fd);

            for (int i = 0; i < count; i++) {
                char buffer[256];
                snprintf(buffer, sizeof(buffer),
                         "%d,%s,%d,%ld,%ld,%d,%s\n",
                         events[i].id,
                         events[i].filename,
                         events[i].interval,
                         events[i].created_at,
                         events[i].last_run,
                         events[i].priority,
                         events[i].created_by);

                write(fd, buffer, strlen(buffer));
            }

            unlock_file(fd);
            close(fd);
        }

        sleep(1);
    }

    return NULL;
}