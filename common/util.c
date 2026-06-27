#include <fcntl.h>
#include <stdio.h>
#include <string.h>
#include <openssl/md5.h>
#include "util.h"

void lock_file(int fd) {
    struct flock lock;
    lock.l_type = F_WRLCK;
    lock.l_whence = SEEK_SET;
    lock.l_start = 0;
    lock.l_len = 0;
    fcntl(fd, F_SETLKW, &lock);
}

void unlock_file(int fd) {
    struct flock lock;
    lock.l_type = F_UNLCK;
    lock.l_whence = SEEK_SET;
    lock.l_start = 0;
    lock.l_len = 0;
    fcntl(fd, F_SETLK, &lock);
}

void md5_hash(const char *input, char *output) {
    unsigned char digest[MD5_DIGEST_LENGTH];
    MD5((unsigned char*)input, strlen(input), digest);

    for (int i = 0; i < MD5_DIGEST_LENGTH; i++)
        sprintf(&output[i*2], "%02x", digest[i]);
}

int authenticate(const char* role, const char* username, const char* password) {
    FILE* file = fopen("data/users.txt", "r");
    if (!file) return 0;

    char r[20], u[50], stored_hash[100];
    char computed_hash[100];

    md5_hash(password, computed_hash);

    while (fscanf(file, "%[^,],%[^,],%s\n", r, u, stored_hash) == 3) {
        if (strcmp(r, role) == 0 &&
            strcmp(u, username) == 0 &&
            strcmp(stored_hash, computed_hash) == 0) {
            fclose(file);
            return 1;
        }
    }

    fclose(file);
    return 0;
}