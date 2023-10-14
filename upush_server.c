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
#include <ctype.h>
#include <time.h>
#include "send_packet.c"
#include "multiple.c"

int lookup_linked(struct Client* first, char* nick) {
    if (first == NULL) {return 0;}
    struct Client* current = first;
    while (current->next != NULL) {
        if (strcmp(current->nick, nick) == 0) {
            return 1;
        }
        current = current->next;
    }
    if (strcmp(current->nick, nick) == 0) {
            return 1;
    } else {
        return 0;
    }
}

int get_int_len (int val) {
    int l = 1;
    if (val > 9) {
        while (val > 9) {
            l++;
            val /= 10;
        }
    }
    return l;
}

int retur_buf(int a, char* buf) {
    int start = a;
    int end = a;
    while (buf[start] != ' ') {
        start++;
    }
    char number[start-end+1];
    int iterasjoner = start-end;
    for (int i = 0; i<iterasjoner; i++) {
        if (!isdigit(buf[end])) {
            return -1;
        }
        number[i] = buf[end];
        end++;
    }
    number[end-4] = '\0';
    int num;
    sscanf(number, "%d", &num);
    number[0] = 0;
    return num;
}

void print_client(struct Client* client) {
    printf("client->nick        : %s\n", client->nick);
    printf("client->reg_num     : %i\n", client->reg_num);
    printf("client->ipen        : %s\n", client->ipen);
    printf("client->port        : %i\n", client->port);
    if (client->next != NULL) {
        printf("client->next->nick  : %s\n\n", client->next->nick);
    }
}

void print_linked(struct Client* first) {
    if (first == NULL) { perror("first equals NULL"); exit(EXIT_FAILURE);}
    struct Client* current = first;
    printf("--------- START ----------\n");
    while (current->next != NULL) {
        print_client(current);
        current = current->next;
    }
    print_client(current);
}

void append_struct(struct Client** this, int reg_msg_num, char* ipen, int port, char* nick, int active) {
    struct Client* current = (struct Client*) malloc(sizeof(struct Client));
    if (current == NULL) { perror("malloc"); exit(EXIT_FAILURE);}
    struct Client* last = *this;

    current->reg_num = reg_msg_num;
    current->ipen = (char*) malloc(strlen(ipen)+1);
    if (current->ipen == NULL) { perror("malloc"); exit(EXIT_FAILURE);}
    strcpy(current->ipen, ipen);
    current->port = port;
    current->active = active;
    current->nick = (char*) malloc(strlen(nick)+1);
    if (current->nick == NULL) { perror("malloc"); exit(EXIT_FAILURE);}
    strcpy(current->nick, nick);
    current->next = NULL;

    if (*this == NULL) {
        *this = current;
        return;
    }

    if (strcmp(last->nick, nick) == 0) {
        if (last->next != NULL) {
            struct Client* next = last;
            current->next = last->next;
            *this = current;
            free(next->ipen);
            free(next->nick);
            free(next);
            return;
        }
        struct Client* next = last;
        free(next->ipen);
        free(next->nick);
        free(next);
        *this = current;
        return;
    }

    while (last->next != NULL && strcmp(last->next->nick, nick) != 0) {
        last = last->next;
    }
    if (last->next != NULL && strcmp(last->next->nick, nick) == 0){
        current->next = last->next->next;
        struct Client* next = last->next;
        free(next->ipen);
        free(next->nick);
        free(next);
        last->next = current;
        return;
    }
    last->next = current;
    return;
}

void free_linked(struct Client* first) {
    struct Client* current;
    while (first != NULL) {
        current = first;
        first = first ->next;
        free(current->ipen);
        free(current->nick);
        free(current);
    }
}

int main(int argc, char **argv) {

    if (argc < 3) {
        perror("Too few arguments for server");
        exit(EXIT_FAILURE);
    }

    int port = (int) atoi(argv[1]);
    float lost_prob = (float) atoi(argv[2]);
    float the_value = ((float) lost_prob)/100;
    set_loss_probability((float) the_value);

    int fd, rec;
    char buf[255];
    fd = socket(AF_INET, SOCK_DGRAM, 0);
    check_error(fd, "Failed setting up socket");

    struct sockaddr_in servaddr, cliaddr;
    socklen_t addr_len;

    memset(&servaddr, 0, sizeof(servaddr));
    memset(&cliaddr, 0, sizeof(cliaddr));

    servaddr.sin_family = AF_INET;
    servaddr.sin_addr.s_addr = INADDR_ANY;
    servaddr.sin_port = htons(port);

    rec = bind(fd, (struct sockaddr*)&servaddr, sizeof(struct sockaddr_in));
    check_error(rec, "Failed bind up receiver");

    addr_len = sizeof(struct sockaddr_in);

    struct Client* first = NULL;

    int counter = 0;
    while (1) {

        struct Client* new = NULL;

        rec = recvfrom(fd, buf, 254, 0, (struct sockaddr*) &cliaddr, &addr_len);
        check_error(rec, "Failed recvfrom");
        buf[rec] = 0;

        // Prints message from nettwork
        printf("%s\n", buf);

        if (strlen(buf) < 11 || strlen(buf) > 32) {
            continue;
        }

        char* ipen = "a";
        ipen = inet_ntoa(cliaddr.sin_addr);
        int port = ntohs(cliaddr.sin_port);
        int reg_msg_num = 0;
        char* register_or_lookup = NULL;

        reg_msg_num = retur_buf(4, buf);
        int reg_msg_len = get_int_len(reg_msg_num);
        int continues = 4 + reg_msg_len + 1;

        if (buf[continues] == 'R' && buf[continues+1] == 'E' &&
                buf[continues+2] == 'G') {
                    register_or_lookup = "REG";
                    continues += 4;
        } else if (buf[continues] == 'L' && buf[continues+1] == 'O' &&
                    buf[continues+2] == 'O' && buf[continues+3] == 'K'&&
                    buf[continues+4] == 'U' && buf[continues+5] == 'P') {
            register_or_lookup = "LOOKUP";
            continues += 7;
        } 
 
        int count = 0;
        int keep_on = continues;
        while (buf[continues] != '\0') {
            continues++;
            count++;
        }

        char nick[count];
        int x = 0;
        while (x < count) {
            nick[x] = buf[keep_on];
            keep_on++;
            x++;
        }
        nick[x] = '\0';

        if (reg_msg_num == -1 || register_or_lookup == NULL) {
            continue;
        }

        if (strcmp(register_or_lookup, "REG") == 0) {
            time_t current_time = time(NULL);
            append_struct(&first, reg_msg_num, ipen, port, nick, current_time);

            int reg_num_len = get_int_len(reg_msg_num);
            char reg_text[reg_num_len];
            sprintf(reg_text, "%d", reg_msg_num);

            char* retur;
            retur = (char*) malloc((reg_num_len+10)*sizeof(char));
            if (retur == NULL) { perror("malloc"); exit(EXIT_FAILURE);}
            strcpy(retur, "ACK ");
            strcat(retur, reg_text);
            strcat(retur, " OK");
            rec = send_packet(fd, retur, strlen(retur), 0, 
                (struct sockaddr*)&cliaddr, sizeof(struct sockaddr_in));
            printf("Sent: %s\n", retur);
            free(retur);
        }

        if (strcmp(register_or_lookup, "LOOKUP") == 0) {
            if (lookup_linked(first, nick)) {
                int reg_num_len = get_int_len(reg_msg_num);
                char reg_text[reg_num_len];
                sprintf(reg_text, "%d", reg_msg_num);

                struct Client* current = first;
                int new_port = 0;
                char* new_ipen = "a";
                if (strcmp(current->nick, nick) == 0) {
                    new_port = current->port;
                    new_ipen = current->ipen;
                }
                while (current->next != NULL) {
                    if (strcmp(current->next->nick, nick) == 0) {
                        new_port = current->next->port;
                        new_ipen = current->next->ipen;
                    }
                    current = current->next;
                }
                time_t current_time = time(NULL);
                if ((current_time - current->active) > 30) {
                    char* retur;
                    retur = (char*) malloc((strlen(nick)+18)*sizeof(char));
                    if (retur == NULL) { perror("malloc"); exit(EXIT_FAILURE);}
                    strcpy(retur, "NICK ");
                    strcat(retur, nick);
                    strcat(retur, " UNREACHABLE");
                    rec = send_packet(fd, retur, strlen(retur), 0, 
                            (struct sockaddr*)&cliaddr, sizeof(struct sockaddr_in));
                    printf("Sent: %s\n", retur);
                    continue;
                } else {
                    int port_len = get_int_len(new_port);
                    char port_text[port_len];
                    sprintf(port_text, "%d", new_port);

                    char* retur;
                    retur = (char*) malloc((reg_num_len+port_len+strlen(nick)+strlen(ipen)+22)*sizeof(char));
                    if (retur == NULL) { perror("malloc"); exit(EXIT_FAILURE);}
                    strcpy(retur, "ACK ");
                    strcat(retur, reg_text);
                    strcat(retur, " NICK ");
                    strcat(retur, nick);
                    strcat(retur, " IP ");
                    strcat(retur, new_ipen);
                    strcat(retur, " PORT ");
                    strcat(retur, port_text);
                    rec = send_packet(fd, retur, strlen(retur), 0, 
                        (struct sockaddr*)&cliaddr, sizeof(struct sockaddr_in));
                    printf("Sent: %s\n", retur);
                    free(retur);
                }

            } else {
                int reg_num_len = get_int_len(reg_msg_num);
                char reg_text[reg_num_len];
                sprintf(reg_text, "%d", reg_msg_num);

                char* retur;
                retur = (char*) malloc((reg_num_len+16)*sizeof(char));
                if (retur == NULL) { perror("malloc"); exit(EXIT_FAILURE);}
                strcpy(retur, "ACK ");
                strcat(retur, reg_text);
                strcat(retur, " NOT FOUND\n");
                rec = send_packet(fd, retur, strlen(retur), 0, 
                    (struct sockaddr*)&cliaddr, sizeof(struct sockaddr_in));
                printf("Sent: %s\n", retur);
                free(retur);
            }
        }
    }
    print_linked(first);
    free_linked(first);

    close(fd);
    return EXIT_SUCCESS;
}
