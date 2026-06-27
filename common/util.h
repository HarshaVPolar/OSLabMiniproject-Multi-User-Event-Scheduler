#ifndef UTILS_H
#define UTILS_H

void lock_file(int fd);
void unlock_file(int fd);

void md5_hash(const char *input, char *output);
int authenticate(const char* role, const char* username, const char* password);
#endif