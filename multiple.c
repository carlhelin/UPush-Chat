#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <netdb.h>
#include <string.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <sys/select.h>
#include <sys/time.h>
#include <arpa/inet.h>

struct Client {
    int reg_num;
    int active;
    int port;
    char* ipen;
    char* nick;
    struct Client* next;
};

void check_error(int res, char *msg) {
    if (res < 0) {
        perror(msg);
        exit(EXIT_FAILURE); 
    }
}