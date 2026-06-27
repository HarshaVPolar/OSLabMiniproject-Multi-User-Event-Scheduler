#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <arpa/inet.h>

#define PORT 8080
#define BUFFER_SIZE 2048

// Colors
#define RESET   "\033[0m"
#define RED     "\033[31m"
#define GREEN   "\033[32m"
#define YELLOW  "\033[33m"
#define BLUE    "\033[34m"
#define CYAN    "\033[36m"
#define BOLD    "\033[1m"

// UI helpers
void line() {
    printf(CYAN "========================================\n" RESET);
}

void clear_screen() {
    system("clear");
}

void wait_for_enter() {
    printf("\nPress Enter to continue...");
    while (getchar() != '\n');
    getchar();
}

// Send command to server
void send_command(const char* command) {
    int sock;
    struct sockaddr_in serv_addr;
    char buffer[BUFFER_SIZE] = {0};

    sock = socket(AF_INET, SOCK_STREAM, 0);
    if (sock < 0) {
        printf(RED "Socket failed\n" RESET);
        return;
    }

    serv_addr.sin_family = AF_INET;
    serv_addr.sin_port = htons(PORT);
    inet_pton(AF_INET, "127.0.0.1", &serv_addr.sin_addr);

    if (connect(sock, (struct sockaddr*)&serv_addr, sizeof(serv_addr)) < 0) {
        printf(RED "Connection failed\n" RESET);
        close(sock);
        wait_for_enter();
        return;
    }

    // 🔥 IMPORTANT: send exact length
    send(sock, command, strlen(command), 0);

    int n = read(sock, buffer, BUFFER_SIZE - 1);
    if (n > 0) {
        buffer[n] = '\0';

        clear_screen();
        printf(GREEN "\n[Server Response]\n" RESET);
        line();
        printf("%s\n", buffer);
        line();
    }

    close(sock);
    wait_for_enter();
}

int main() {
    int main_choice;

    while (1) {
        clear_screen();

        line();
        printf(BOLD BLUE "   TASK SCHEDULER CLIENT\n" RESET);
        line();

        printf(YELLOW "1. Create User\n");
        printf("2. Create Admin\n");
        printf("3. Login\n");
        printf("0. Exit\n" RESET);

        printf("\n> ");
        scanf("%d", &main_choice);

        if (main_choice == 0) break;

        // -------- CREATE USER --------
        if (main_choice == 1) {
            char username[50], password[50], command[BUFFER_SIZE];

            printf("Username: ");
            scanf("%s", username);

            printf("Password: ");
            scanf("%s", password);

            sprintf(command, "CREATE_USER %s %s", username, password);

            // printf("DEBUG SENT: [%s]\n", command);
            send_command(command);
        }

        // -------- CREATE ADMIN --------
        else if (main_choice == 2) {
            char username[50], password[50], key[50], command[BUFFER_SIZE];

            printf("Admin Username: ");
            scanf("%s", username);

            printf("Password: ");
            scanf("%s", password);

            printf("Admin Key: ");
            scanf("%s", key);

            sprintf(command, "CREATE_ADMIN %s %s %s", username, password, key);

            send_command(command);
        }

        // -------- LOGIN --------
        else if (main_choice == 3) {
            int role_choice;
            char role[10];
            char username[50] = "", password[50] = "";

            printf("\nSelect Role:\n");
            printf("1. ADMIN\n2. USER\n3. GUEST\n> ");
            scanf("%d", &role_choice);

            if (role_choice == 1) {
                strcpy(role, "ADMIN");
                printf("Username: ");
                scanf("%s", username);
                printf("Password: ");
                scanf("%s", password);
            }
            else if (role_choice == 2) {
                strcpy(role, "USER");
                printf("Username: ");
                scanf("%s", username);
                printf("Password: ");
                scanf("%s", password);
            }
            else {
                strcpy(role, "GUEST");
            }

            // -------- SESSION LOOP --------
            while (1) {
                clear_screen();

                line();
                printf(BOLD "Logged in as: %s\n" RESET, role);
                line();

                printf("1. View Tasks\n");

                if (strcmp(role, "GUEST") != 0) {
                    printf("2. Add Task\n");
                    printf("3. Delete Task\n");
                }

                if (strcmp(role, "ADMIN") == 0) {
                    printf("4. Stop Server\n");
            }

                printf("0. Logout\n");
                printf("\n> ");

                int choice;
                scanf("%d", &choice);

                if (choice == 0) break;

                char command[BUFFER_SIZE];

                // -------- VIEW --------
                if (choice == 1) {
                    if (strcmp(role, "GUEST") == 0) {
                        sprintf(command, "GUEST VIEW");
                    } else {
                        sprintf(command, "%s %s %s VIEW",
                                role, username, password);
                    }

                    send_command(command);
                }

                // -------- ADD --------
                else if (choice == 2 && strcmp(role, "GUEST") != 0) {
                    char file[100];
                    int interval, priority;

                    printf("File: ");
                    scanf("%s", file);

                    int interval_choice;

                    printf("\nSelect Interval:\n");
                    printf("1. Hourly\n");
                    printf("2. Daily\n");
                    printf("3. Weekly\n");
                    printf("4. Custom\n> ");
                    scanf("%d", &interval_choice);

                    switch (interval_choice) {
                        case 1: interval = 3600; break;
                        case 2: interval = 86400; break;
                        case 3: interval = 604800; break;
                        case 4:
                            printf("Enter seconds: ");
                            scanf("%d", &interval);
                            break;
                        default:
                            interval = 5;
                    }

                    printf("Priority: ");
                    scanf("%d", &priority);

                    sprintf(command, "%s %s %s ADD %s %d %d",
                            role, username, password,
                            file, interval, priority);

                    // printf("DEBUG SENT: [%s]\n", command);
                    send_command(command);
                }

                // -------- DELETE --------
                else if (choice == 3 && strcmp(role, "GUEST") != 0) {
                    int id;
                    printf("Task ID: ");
                    scanf("%d", &id);

                    sprintf(command, "%s %s %s DELETE %d",
                            role, username, password, id);

                    // printf("DEBUG SENT: [%s]\n", command);
                    send_command(command);
                }
                else if (choice == 4 && strcmp(role, "ADMIN") == 0) {
                    sprintf(command, "%s %s %s STOP",role, username, password);
                    send_command(command);
                    printf("Server stopped. Exiting client...\n");
                    exit(0);
                }

                else {
                    printf(RED "Invalid option\n" RESET);
                    wait_for_enter();
                }
            }
        }

        else {
            printf(RED "Invalid choice\n" RESET);
            wait_for_enter();
        }
    }

    return 0;
}