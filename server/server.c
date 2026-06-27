#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <pthread.h>
#include <fcntl.h>
#include <time.h>
#include <arpa/inet.h>
#include <sys/socket.h>
#include <signal.h>

#include "scheduler.h"
#include "util.h"
#include "events.h"
#include "worker.h"

#define PORT 8080
#define BUFFER_SIZE 1024
#define TASK_FILE "data/tasks.txt"
#define ADMIN_KEY_HASH "5f4dcc3b5aa765d61d8327deb882cf99"

volatile int server_running = 1;

/* ---------------- SIGNAL HANDLER (IPC) ---------------- */
void handle_sigint(int sig) {
    log_event("SERVER STOPPED (SIGINT)");
    exit(0);
}

/* ---------------- ADD TASK ---------------- */
void add_task(const char* filename, int interval, int priority, const char* created_by) {
    int fd = open(TASK_FILE, O_WRONLY | O_CREAT | O_APPEND, 0644);
    if (fd < 0) return;

    lock_file(fd);

    time_t now = time(NULL);
    int id = time(NULL) + rand();

    char line[256];
    snprintf(line, sizeof(line),
             "%d,%s,%d,%ld,%ld,%d,%s\n",
             id, filename, interval, now, (long int)0, priority, created_by);

    write(fd, line, strlen(line));

    unlock_file(fd);
    close(fd);
}

/* ---------------- HANDLE CLIENT ---------------- */
void* handle_client(void* arg) {
    int client_socket = *(int*)arg;
    free(arg);

    char buffer[1024] = {0};
    read(client_socket, buffer, sizeof(buffer));

    char role[20], username[50], password[50], command[20];
    char filename[100];
    int interval, priority, id;

    // CREATE USER
    if (sscanf(buffer, "%s %s %s", command, username, password) == 3 &&
        strcmp(command, "CREATE_USER") == 0) {

        char pass_hash[100];
        md5_hash(password, pass_hash);

        FILE* file = fopen("data/users.txt", "a");
        fprintf(file, "USER,%s,%s\n", username, pass_hash);
        fclose(file);

        send(client_socket, "User created\n", 13, 0);
    }

    // CREATE ADMIN
    else if (sscanf(buffer, "%s %s %s %s", command, username, password, filename) == 4 &&
             strcmp(command, "CREATE_ADMIN") == 0) {

        char key_hash[100];
        md5_hash(filename, key_hash);

        if (strcmp(key_hash, ADMIN_KEY_HASH) != 0) {
            send(client_socket, "Invalid admin key\n", 18, 0);
        } else {
            char pass_hash[100];
            md5_hash(password, pass_hash);

            FILE* file = fopen("data/users.txt", "a");
            fprintf(file, "ADMIN,%s,%s\n", username, pass_hash);
            fclose(file);

            send(client_socket, "Admin created\n", 14, 0);
        }
    }

    // GUEST VIEW
    else if (sscanf(buffer, "%s %s", role, command) == 2 &&
             strcmp(role, "GUEST") == 0 &&
             strcmp(command, "VIEW") == 0) {

        int fd = open(TASK_FILE, O_RDONLY);
        if (fd < 0) {
            send(client_socket, "No tasks found\n", 15, 0);
        } else {
            char out[2048];
            int n = read(fd, out, sizeof(out)-1);
            if (n > 0) {
                out[n] = '\0';
                send(client_socket, out, strlen(out), 0);
            }
            close(fd);
        }
    }

    else {
        int base = sscanf(buffer, "%s %s %s %s", role, username, password, command);
        if (base < 4) {
            send(client_socket, "Invalid format\n", 15, 0);
            close(client_socket);
            return NULL;
        }

        if (!authenticate(role, username, password)) {
            send(client_socket, "Auth failed\n", 12, 0);
            close(client_socket);
            return NULL;
        }

        // VIEW
        if (strcmp(command, "VIEW") == 0) {
            int fd = open(TASK_FILE, O_RDONLY);

            if (fd < 0) {
                send(client_socket, "No tasks found\n", 15, 0);
            } else {
                char out[2048];
                int n = read(fd, out, sizeof(out)-1);

                if (n > 0) {
                    out[n] = '\0';
                    send(client_socket, out, strlen(out), 0);
                }
                close(fd);
            }
        }

        // ADD
        else if (strcmp(command, "ADD") == 0) {

            int parsed = sscanf(buffer, "%s %s %s %s %s %d %d",
                                role, username, password,
                                command, filename, &interval, &priority);

            if (parsed != 7) {
                send(client_socket, "Invalid ADD format\n", 19, 0);
                close(client_socket);
                return NULL;
            }

            add_task(filename, interval, priority, username);
            send(client_socket, "Task added\n", 11, 0);
        }

        // DELETE
        else if (strcmp(command, "DELETE") == 0) {

            sscanf(buffer, "%s %s %s %s %d",
                   role, username, password, command, &id);

            FILE *file = fopen(TASK_FILE, "r");
            FILE *temp = fopen("data/temp.txt", "w");

            char line[256];
            Event e;
            int deleted = 0;

            while (fgets(line, sizeof(line), file)) {
                if (parse_event(line, &e) == 7) {

                    int can_delete = 0;

                    if (strcmp(role, "ADMIN") == 0)
                        can_delete = 1;
                    else if (strcmp(role, "USER") == 0 &&
                             strcmp(e.created_by, username) == 0)
                        can_delete = 1;

                    if (e.id == id && can_delete) {
                        deleted = 1;
                        continue;
                    }

                    fputs(line, temp);
                }
            }

            fclose(file);
            fclose(temp);

            remove(TASK_FILE);
            rename("data/temp.txt", TASK_FILE);

            send(client_socket,
                 deleted ? "Task deleted\n" : "Delete failed\n",
                 deleted ? 13 : 14, 0);
        }

        // 🔥 STOP SERVER
        else if (strcmp(command, "STOP") == 0) {

            if (strcmp(role, "ADMIN") != 0) {
                send(client_socket, "Only admin can stop server\n", 28, 0);
                close(client_socket);
                return NULL;
            }

            char msg[100];
            snprintf(msg, sizeof(msg), "SERVER STOPPED by %s", username);
            log_event(msg);

            send(client_socket, "Server shutting down\n", 22, 0);

            server_running = 0;
            close(client_socket);
            exit(0);
        }

        else {
            send(client_socket, "Invalid command\n", 16, 0);
        }
    }

    close(client_socket);
    return NULL;
}

/* ---------------- MAIN ---------------- */
int main() {
    int server_fd, new_socket;
    struct sockaddr_in address;
    int addrlen = sizeof(address);

    signal(SIGINT, handle_sigint);  

    server_fd = socket(AF_INET, SOCK_STREAM, 0);

    address.sin_family = AF_INET;
    address.sin_addr.s_addr = INADDR_ANY;
    address.sin_port = htons(PORT);

    bind(server_fd, (struct sockaddr*)&address, sizeof(address));
    listen(server_fd, 5);

    printf("Server running...\n");
    log_event("SERVER STARTED");

    pthread_t scheduler_thread;
    pthread_create(&scheduler_thread, NULL, scheduler_loop, NULL);

    while (server_running) {
        new_socket = accept(server_fd,
                            (struct sockaddr*)&address,
                            (socklen_t*)&addrlen);

        int* client_sock = malloc(sizeof(int));
        *client_sock = new_socket;

        pthread_t tid;
        pthread_create(&tid, NULL, handle_client, client_sock);
        pthread_detach(tid);
    }

    close(server_fd);
    return 0;
}