#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <sys/wait.h>
#include <time.h>
#include <fcntl.h>
#include <signal.h>

#include "worker.h"
#include "util.h"

#define TASK_DIR "data/tasks/"
#define LOG_FILE "data/log.txt"

/* ---------------- SIGNAL HANDLER ---------------- */
void handle_sigterm(int sig) {
    log_event("WORKER RECEIVED SHUTDOWN SIGNAL (SIGTERM)");
    exit(0);
}

/* ---------------- LOGGING ---------------- */
void log_event(const char* msg) {
    int fd = open(LOG_FILE, O_WRONLY | O_CREAT | O_APPEND, 0644);
    if (fd < 0) return;

    lock_file(fd);
    
    time_t now = time(NULL);
    char time_str[100];
    struct tm *t = localtime(&now);    
    strftime(time_str, sizeof(time_str), "%Y-%m-%d %H:%M:%S", t);

    char buffer[512];
    snprintf(buffer, sizeof(buffer), "[%ld] %s %s\n", now, msg, time_str);
    write(fd, buffer, strlen(buffer));

    unlock_file(fd);
    close(fd);
}

/* ---------------- EXECUTE TASK ---------------- */
void execute_task(Event e) {
    pid_t pid = fork();

    if (pid == 0) {


        signal(SIGTERM, handle_sigterm);  // handle shutdown signal

        char path[200];
        snprintf(path, sizeof(path), "%s%s", TASK_DIR, e.filename);

        execl(path, e.filename, NULL);

        perror("exec failed");
        exit(1);
    } 
    else if (pid > 0) {
        int status;
        waitpid(pid, &status, 0);

        char logmsg[200];
        if (WIFEXITED(status)) {
            snprintf(logmsg, sizeof(logmsg),
                     "Task %d (%s) completed",
                     e.id, e.filename);
        } else {
            snprintf(logmsg, sizeof(logmsg),
                     "Task %d (%s) terminated",
                     e.id, e.filename);
        }

        log_event(logmsg);
    } 
    else {
        perror("fork failed");
    }
}