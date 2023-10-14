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

#define BUFSIZE 1422

struct Print 
{
    char* mes;
};

struct Message
{   
    int times_sent;
    int went_trough;
    int ack_num;
    char *message;
    char *reciever;
    struct Message *next;
};

struct PktAck
{   
    int ack_num;
};

struct Blocked
{
    char *nick;
    int yes_or_no;
    struct Blocked *next;
};

int check_print(struct Print* first, char* msg) {
    struct Print* current = first;
    if (first != NULL) {
        if (strcmp(current->mes, msg) == 0) {
            return 0;
        } else {
            return 1; 
        }
    } else {
        return 1;
    }
}

void set_print(struct Print** this, char* msg) {

    struct Print *current = (struct Print *)malloc(sizeof(struct Print));
    if (current == NULL){ perror("malloc"); exit(EXIT_FAILURE);}

    struct Print *last = *this;
    current->mes = strdup(msg);

    if (*this == NULL) { *this = current; return; }

    else {
        free(last->mes);
        free(last);
        *this = current;
    }
}

void free_print(struct Print** this) {
    struct Print *last = *this;
    free(last->mes);
    free(last);
}

void set_cur_ack(struct PktAck** cur, int num) {
    struct PktAck *current = (struct PktAck *)malloc(sizeof(struct PktAck));
    if (current == NULL){ perror("malloc"); exit(EXIT_FAILURE);}
    struct PktAck *last = *cur;
    current->ack_num = num;
    free(*cur);
    *cur = current;
}

void free_pkt_ack(struct PktAck** first) {
    struct PktAck *current;
    current = *first;
    free(current);
}

int check_pktack (struct PktAck* first) {
    return first->ack_num;
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

struct Message* lookup_message(struct Message* first, int ack_num) {
    struct Message* current = first;
    if (current->ack_num == ack_num) {
            return current;
    }
    while (current->next != NULL) {
        if (current->ack_num == ack_num) {
            return current;
        }
        current = current->next;
    }
    if (current->ack_num == ack_num) {
            return current;
    }
    return NULL;
}

struct Client* get_client(struct Client* first, char* nick) {
    struct Client* current = first;
    if (strcmp(current->nick, nick) == 0) {
            return current;
    }
    while (current->next != NULL) {
        if (strcmp(current->nick, nick) == 0) {
            return current;
        }
        current = current->next;
    }
    if (strcmp(current->nick, nick) == 0) {
            return current;
    } else {
        return NULL;
    }
}

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

void append_blocked(struct Blocked **this, char *nick)
{
    struct Blocked *current = (struct Blocked *)malloc(sizeof(struct Blocked));
    if (current == NULL){ perror("malloc"); exit(EXIT_FAILURE);}
    struct Blocked *last = *this;
    current->nick = strdup(nick);
    current->yes_or_no = 0;
    current->next = NULL;

    if (*this == NULL)
    {
        *this = current;
        return;
    }
    if (strcmp(last->nick, nick) == 0)
    {
        if (last->next != NULL)
        {
            struct Blocked *next = last;
            current->next = last->next;
            *this = current;
            free(next->nick);
            free(next);
            return;
        }
        struct Blocked *next = last;
        free(next->nick);
        free(next);
        *this = current;
        return;
    }

    while (last->next != NULL && strcmp(last->next->nick, nick) != 0)
    {
        last = last->next;
    }
    if (last->next != NULL && strcmp(last->next->nick, nick) == 0)
    {
        current->next = last->next->next;
        struct Blocked *next = last->next;
        free(next->nick);
        free(next);
        last->next = current;
        return;
    }
    last->next = current;
    return;
}

int check_blocked_status(struct Blocked **first, char *nick)
{
    if (*first == NULL ) {return 0;}
    struct Blocked *current = *first;
    while (current->next != NULL)
    {
        if (strcmp(current->nick, nick) == 0)
        {
            return current->yes_or_no;
        }
        current = current->next;
    }
    if (strcmp(current->nick, nick) == 0)
    {
        return current->yes_or_no;
    }
    else
    {
        return 0;
    }
}

void block_person(struct Blocked **first, char *nick)
{
    if (first == NULL)
    {
        perror("first blocked equals NULL");
        exit(EXIT_FAILURE);
    }
    struct Blocked *current = *first;
    while (current->next != NULL)
    {
        if (strcmp(current->nick, nick) == 0)
        {
            current->yes_or_no = 1;
        }
        current = current->next;
    }
    if (strcmp(current->nick, nick) == 0)
    {
        current->yes_or_no = 1;
    }
}

void unblock_person(struct Blocked **first, char *nick)
{
    if (first == NULL)
    {
        perror("first blocked equals NULL");
        exit(EXIT_FAILURE);
    }
    struct Blocked *current = *first;
    while (current->next != NULL)
    {
        if (strcmp(current->nick, nick) == 0)
        {
            current->yes_or_no = 0;
        }
        current = current->next;
    }
    if (strcmp(current->nick, nick) == 0)
    {
        current->yes_or_no = 0;
    }
}

void free_blocked_link(struct Blocked *first)
{
    struct Blocked *current;
    while (first != NULL)
    {
        current = first;
        first = first->next;
        free(current->nick);
        free(current);
    }
}

void append_message(struct Message **this, char *message, int awk_num, int went_trough, char* reciever, int times)
{
    struct Message *current = (struct Message *)malloc(sizeof(struct Message));
    if (current == NULL){ perror("malloc"); exit(EXIT_FAILURE);}
    struct Message *last = *this;
    current->went_trough = went_trough;
    current->ack_num = awk_num;
    current->times_sent = times;
    current->message = strdup(message);
    current->reciever = strdup(reciever);
    current->next = NULL;

    if (*this == NULL)
    {
        *this = current;
        return;
    }

    while (last->next != NULL)
    {
        last = last->next;
    }
    last->next = current;
}

void free_messages(struct Message *first)
{
    struct Message *current;
    while (first != NULL)
    {
        current = first;
        first = first->next;
        free(current->message);
        free(current->reciever);
        free(current);
    }
}

void print_messages(struct Message *first)
{
    printf("MESSAGES: \n");
    while (first != NULL)
    {
        printf("   message: %s\n", first->message);
        printf("   ack_num: %i\n", first->ack_num);
        printf("   went_trough: %i\n", first->went_trough);
        printf("   reciever: %s\n", first->reciever);
        printf("   times sent: %i\n\n", first->times_sent);
        first = first->next;
    }
}

void get_string(char buf[], int size)
{
    char c;
    fgets(buf, size, stdin);
    if (buf[strlen(buf) - 1] == '\n')
    {
        buf[strlen(buf) - 1] = 0;
    }
    else
    {
        while ((c = getchar()) != '\n' && c != EOF)
            ;
    }
}

void print_client(struct Client *client)
{
    printf("client->nick        : %s\n", client->nick);
    printf("client->ipen        : %s\n", client->ipen);
    printf("client->port        : %i\n", client->port);
    if (client->next != NULL)
    {
        printf("client->next->nick  : %s\n\n", client->next->nick);
    }
}

void print_linked(struct Client *first)
{
    if (first == NULL)
    {
        perror("first equals NULL");
        exit(EXIT_FAILURE);
    }
    struct Client *current = first;
    while (current->next != NULL)
    {
        print_client(current);
        current = current->next;
    }
    print_client(current);
}

void append_struct(struct Client **this, char *ipen, int port, char *nick)
{
    struct Client *current = (struct Client *)malloc(sizeof(struct Client));
    if (current == NULL){ perror("malloc"); exit(EXIT_FAILURE);}
    struct Client *last = *this;

    current->ipen = strdup(ipen);
    current->port = port;
    current->nick = strdup(nick);
    current->next = NULL;

    if (*this == NULL)
    {
        *this = current;
        return;
    }

    if (strcmp(last->nick, nick) == 0)
    {
        if (last->next != NULL)
        {
            struct Client *next = last;
            current->next = last->next;
            *this = current;
            free(next->ipen);
            free(next->nick);
            free(next);
            return;
        }
        struct Client *next = last;
        free(next->ipen);
        free(next->nick);
        free(next);
        *this = current;
        return;
    }

    while (last->next != NULL && strcmp(last->next->nick, nick) != 0)
    {
        last = last->next;
    }
    if (last->next != NULL && strcmp(last->next->nick, nick) == 0)
    {
        current->next = last->next->next;
        struct Client *next = last->next;
        free(next->ipen);
        free(next->nick);
        free(next);
        last->next = current;
        return;
    }
    last->next = current;
    return;
}

void free_linked(struct Client *first)
{
    struct Client *current;
    while (first != NULL)
    {
        current = first;
        first = first->next;
        free(current->nick);
        free(current->ipen);
        free(current);
    }
}

int main(int argc, char const *argv[])
{
    printf("'QUIT' + <Enter> to quit program\n");
    if (argc < 6)
    {
        perror("Too few arguments for server\n");
        exit(EXIT_FAILURE);
    }

    int msg_fd, rec, wc;
    fd_set fds;
    struct sockaddr_in my_addr, server_addr, send_to;
    struct in_addr ip_addr;
    char buf[BUFSIZE + 22];


    msg_fd = socket(AF_INET, SOCK_DGRAM, 0);
    check_error(msg_fd, "Failed setting up socket");

    rec = inet_pton(AF_INET, "127.0.0.1", &server_addr);
    check_error(rec, "inet_pton");

    int nick_len = strlen(argv[1]);
    char *nick = malloc(nick_len + 1);
    if (nick == NULL){ perror("malloc"); exit(EXIT_FAILURE);}
    strcpy(nick, argv[1]);

    const char *ip = argv[2];
    int port = (int)atoi(argv[3]);
    int timeout = (int)atoi(argv[4]);
    int lost_prob = (int)atoi(argv[5]);
    float the_value = ((float) lost_prob)/100;
    set_loss_probability((float)the_value);
    
    server_addr.sin_family = AF_INET;
    server_addr.sin_port = htons(port);
    server_addr.sin_addr.s_addr = inet_addr(ip);

    int teller = 0;

    struct Client *first = NULL;
    struct Message *first_msg = NULL;
    struct Blocked *first_blocked = NULL;
    struct PktAck *first_ack = NULL;
    struct Print* first_print = NULL;
    set_cur_ack(&first_ack, -1);

    struct timeval pause; // Time out value
    pause.tv_sec = timeout;     // Set the time out value (2 sec)
    pause.tv_usec = 0;

    int ack_recv = 0;
    
    // While loop for registration of client to server
    // that waits for ack-message from server-side
    while (1) 
    {
        FD_ZERO(&fds);
        fflush(NULL);
        FD_SET(msg_fd, &fds);

        if (ack_recv == 0) {
            char str[4];
            sprintf(str, "%d", teller);
            char *reg_client;
            reg_client = (char *)malloc((strlen(str) + strlen(nick) + 11) * sizeof(char));
            if (reg_client == NULL){ perror("malloc"); exit(EXIT_FAILURE);}
            strcpy(reg_client, "PKT ");
            strcat(reg_client, str);
            strcat(reg_client, " REG ");
            strcat(reg_client, nick);
            int send = send_packet(msg_fd, reg_client, strlen(reg_client), 0,
                              (struct sockaddr *)&server_addr,
                              sizeof(struct sockaddr_in));
            check_error(send, "Send to not working");
            free(reg_client);
            teller++;
        }
        ack_recv = select(FD_SETSIZE, &fds, NULL, NULL, &pause);
        check_error(rec, "Select not working properly");

        if (FD_ISSET(msg_fd, &fds)) {
            recv(msg_fd, buf, 256, 0);
            break;
        } 
        else if (ack_recv == 0) {
            printf("NOT ABLE TO REACH SERVER\n");
            free(first_ack);
            free(nick);
            close(msg_fd);
            return EXIT_SUCCESS;
        }
    }

    time_t mytime = time(NULL);
    int switcher = 1;
    while (1)
    {   
        char str[4];
        time_t current_time = time(NULL);
        if ((mytime - current_time) % 10 == 0 && switcher) {
            sprintf(str, "%d", teller);
            char retur[strlen(nick) + strlen(str) + 10];
            strcpy(retur, "PKT ");
            strcat(retur, str);
            strcat(retur, " REG ");
            strcat(retur, nick);

            int ny = send_packet(msg_fd, retur, strlen(retur), 0,
                                    (struct sockaddr *)&server_addr,
                                    sizeof(struct sockaddr_in));
            check_error(ny, "Send to not working");
            teller++;
            switcher = 0;
        }
        current_time++;
        if ((mytime+current_time) % 10 == 0 ) {
                switcher = 1;
            }

        int time_out_tries = 0;
        FD_ZERO(&fds);
        FD_SET(msg_fd, &fds);
        FD_SET(STDIN_FILENO, &fds);
       
        sprintf(str, "%d", teller);
        rec = select(FD_SETSIZE, &fds, NULL, NULL, &pause);
        check_error(rec, "Select not working properly");

        // Input from user keyboard
        if (FD_ISSET(STDIN_FILENO, &fds))
        {   
            get_string(buf, BUFSIZE);
            char quit[5];
            memcpy(quit, &buf[0], 4);
            quit[4] = '\0';

            if (strcmp(quit, "QUIT") == 0) { break; }

            if (buf[0] == '@')
            {

                int count_nick = 0;
                while (buf[count_nick] != ' ' && buf[count_nick] != EOF)
                {
                    count_nick++;
                }
                count_nick += 1;

                char to_nick[count_nick];
                memcpy(to_nick, &buf[1], count_nick);
                to_nick[count_nick - 2] = '\0';
                

                if (lookup_linked(first, to_nick) == 0) {
                    append_message(&first_msg, buf, teller, 0, to_nick, 0);
                    teller++;
                    sprintf(str, "%d", teller);
                    char retur[strlen(to_nick) + strlen(str) + 13];
                    strcpy(retur, "PKT ");
                    strcat(retur, str);
                    strcat(retur, " LOOKUP ");
                    strcat(retur, to_nick);

                    int ny = send_packet(msg_fd, retur, strlen(retur), 0,
                                          (struct sockaddr *)&server_addr,
                                          sizeof(struct sockaddr_in));
                    check_error(ny, "Send to not working");
                    char* server = "SERVER";
                    append_message(&first_msg, retur, teller, 0, server, 1);
                    teller++;
                }

                if (first != NULL && lookup_linked(first, to_nick) == 1 && check_blocked_status(&first_blocked, to_nick) == 0)
                {
                    int sockfd;
                    sockfd = socket(AF_INET, SOCK_DGRAM, 0);
                    struct Client *current = first;
                    current = get_client(first, to_nick);
                    
                    rec = inet_pton(AF_INET, current->ipen, &send_to);
                    check_error(rec, "inet_pton");

                    send_to.sin_family = AF_INET;
                    send_to.sin_port = htons(current->port);
                    send_to.sin_addr.s_addr = inet_addr(current->ipen);

                    int length = strlen(str) + strlen(nick) +
                                 strlen(to_nick) + strlen(nick) + strlen(buf) + 20;
                    sprintf(str, "%d", teller);
                    char subtext[length];
                    strcpy(subtext, "PKT ");
                    strcat(subtext, str);
                    strcat(subtext, " FROM ");
                    strcat(subtext, nick);
                    strcat(subtext, " TO ");
                    strcat(subtext, to_nick);
                    strcat(subtext, " MSG ");
                    strcat(subtext, &buf[strlen(to_nick) + 2]);
                    subtext[length] = '\0';

                    int new = send_packet(sockfd, subtext, strlen(subtext), 0,
                                          (struct sockaddr *)&send_to,
                                          sizeof(struct sockaddr_in));
                    check_error(new, "Send to not working");
                    append_message(&first_msg, subtext, teller, 0, current->nick, 1);
                    teller++;
                }
            
            }

            if (buf[0] == 'B' && buf[1] == 'L' &&
                buf[2] == 'O' && buf[3] == 'C' &&
                buf[4] == 'K')
            {
                int count_nick = 6;
                int count_end_nick = 6;
                while (buf[count_end_nick] != '\0')
                {
                    count_end_nick++;
                }
                char block_nick[count_end_nick - count_nick + 1];
                memcpy(block_nick, &buf[count_nick], count_end_nick - count_nick + 1);
                if (lookup_linked(first, block_nick) == 0)
                {
                    printf("NICK %s NOT REGISTERED\n", block_nick);
                }
                else
                {
                    block_person(&first_blocked, block_nick);
                }
            }

            if (buf[0] == 'U' && buf[1] == 'N' &&
                buf[2] == 'B' && buf[3] == 'L' &&
                buf[4] == 'O' && buf[5] == 'C' &&
                buf[6] == 'K')
            {
                int count_nick = 8;
                int count_end_nick = 8;
                while (buf[count_end_nick] != '\0')
                {
                    count_end_nick++;
                }
                char block_nick[count_end_nick - count_nick + 1];
                memcpy(block_nick, &buf[count_nick], count_end_nick - count_nick + 1);
                if (lookup_linked(first, block_nick) == 0)
                {
                    printf("NICK %s NOT REGISTERED\n", block_nick);
                }
                else
                {
                    unblock_person(&first_blocked, block_nick);
                }
            }
        }


        // Message from internet incomming
        if (FD_ISSET(msg_fd, &fds))
        {   
            int count_ack = 0;
            rec = read(msg_fd, buf, BUFSIZE - 1);
            check_error(rec, "Read not working");
            buf[rec] = '\0';

            // The messages that start with P is always the ones
            // comming from another client and in this if-check
            // it will print out the message if allowed 
            if (buf != NULL && buf[0] == 'P')
            {   
                
                int count_to_ack_num = 0;
                while (!isdigit(buf[count_to_ack_num])) {
                    count_to_ack_num++;
                }
                int start_ack = count_to_ack_num;
                while (isdigit(buf[count_to_ack_num])) {
                    count_to_ack_num++;
                }
                count_to_ack_num++;
                char *ack_num_char = malloc(count_to_ack_num-start_ack+1);
                if (ack_num_char == NULL){ perror("malloc"); exit(EXIT_FAILURE);}
                memcpy(ack_num_char, &buf[start_ack], count_to_ack_num-start_ack);
                ack_num_char[count_to_ack_num-start_ack-1] = '\0';

                int ack_num_int = atoi(ack_num_char);
                set_cur_ack(&first_ack, ack_num_int);
                
                int count_start_nick = 0;
                while (buf[count_start_nick] != 'M')
                {
                    count_start_nick++;
                }
                count_start_nick += 2;

                int count_end_nick = count_start_nick;
                while (buf[count_end_nick] != ' ')
                {
                    count_end_nick++;
                }
                count_end_nick--;

                int length_nick = 0;
                length_nick = count_end_nick - count_start_nick;

                char from_nick[length_nick + 1];
                memcpy(from_nick, &buf[count_start_nick], length_nick + 1);
                from_nick[length_nick + 1] = '\0';

                int count_start_msg = count_end_nick;
                while (buf[count_start_msg] == 'O')
                {
                    count_start_msg++;
                }
                count_start_msg += 3;
                count_start_msg += strlen(nick);
                count_start_msg += 7;
                       
                if (lookup_linked(first, from_nick) == 0)
                {   
                    char retur[strlen(from_nick)+ strlen(ack_num_char) + 14];
                    strcpy(retur, "PKT ");
                    strcat(retur, ack_num_char);
                    strcat(retur, " LOOKUP ");
                    strcat(retur, from_nick);

                    int rec = send_packet(msg_fd, retur, strlen(retur), 0,
                                          (struct sockaddr *)&server_addr,
                                          sizeof(struct sockaddr_in));
                    check_error(rec, "Send to not working");
                } 
                
                if (lookup_linked(first, from_nick) == 1) {
                    struct Client* cur_cli;
                    cur_cli = get_client(first, from_nick);
                    int len_ack;
                    len_ack = get_int_len(ack_num_int);
                    int len = len_ack + 8;
                    char retur[len];
                    sprintf(retur, "ACK %i OK", first_ack->ack_num);

                    struct sockaddr_in send_mes;
                    send_mes.sin_family = AF_INET;
                    send_mes.sin_port = htons(cur_cli->port);
                    send_mes.sin_addr.s_addr = inet_addr(cur_cli->ipen);

                    int rec = send_packet(msg_fd, retur, strlen(retur), 0,
                                        (struct sockaddr *)&send_mes,
                                        sizeof(struct sockaddr_in));
                    check_error(rec, "Send to not working");
                }

                char *stdrr;
                stdrr = (char *)malloc(strlen(buf)+strlen(nick)+2 * sizeof(char));
                if (stdrr == NULL){ perror("malloc"); exit(EXIT_FAILURE);}
                strcpy(stdrr, "@");
                strcat(stdrr, from_nick);
                strcat(stdrr, " ");
                strcat(stdrr, &buf[count_start_msg]);

                if (check_blocked_status(&first_blocked, from_nick) == 0) {
                    struct Print* printed;
                    if (first_print != NULL) {
                        int val = check_print(first_print, stdrr);
                        if (val) {
                            printf("%45s\n", stdrr);
                            set_print(&first_print, stdrr);
                        }
                    } else {
                        set_print(&first_print, stdrr);
                        printf("%45s\n", stdrr);
                    }
                }
                
                free(stdrr);
                free(ack_num_char);
            }

            else if (buf != NULL && buf[0] == 'A')
            {
                
                char *arr[8];
                arr[0] = "ACK";
                int count = 0;
                while (!isdigit(buf[count]) && buf[count] != '\0')
                {
                    count++;
                }
                int e = count;
                while (buf[count] != ' ' && buf[count] != '\0')
                {
                    count++;
                }

                char *char_num = (char*) malloc(count - e+1 * sizeof(char));
                if (char_num == NULL){ perror("malloc"); exit(EXIT_FAILURE);}
                memcpy(char_num, &buf[e], count - e);
                char_num[count-e] = '\0';
                arr[1] = char_num;

                int ack_number = *char_num - '0';

                count++;
                int d = count;
                while (buf[count] != ' ' && buf[count] != '\0')
                {
                    count++;
                }
                char *contain = (char*) malloc(count - d+1 * sizeof(char));
                if (contain == NULL){ perror("malloc"); exit(EXIT_FAILURE);}
                memcpy(contain, &buf[d], count - d);
                contain[count-d] = 0;

                if (strcmp(contain, "OK") == 0)
                {   
                    arr[2] = "OK";
                }

                if (strcmp(contain, "NOT") == 0)
                {
                    arr[2] = "NOT FOUND";
                    int val = atoi(char_num);
                    struct Message* cur_msg;
                    if (first_msg != NULL) {
                        cur_msg = lookup_message(first_msg, val-1);
                        printf("NICK %s NOT REGISTERED\n", cur_msg->reciever);
                    }
                }

                if (strcmp(contain, "WRONG") == 0)
                {
                    int next = count + 1;
                    if (buf[next] == 'N')
                    {
                        arr[2] = "WRONG NAME";
                    }
                    if (buf[next] == 'F')
                    {
                        arr[2] = "WRONG FORMAT";
                    }
                    printf("%s %s %s\n", arr[0], arr[1], arr[2]);
                }

                // Important segment that will set the messages in the message-list
                // in a state they have been acknowledged by the other clients/server 
                struct Message* cur;
                if (first_msg != NULL) {
                    cur = lookup_message(first_msg, ack_number);
                    if (cur != NULL) {
                        cur->went_trough = 1;
                    }
                }

                char *nick_name;
                char *that_ip = "a";
                char *ip_sin = "a";
                int reg_port = 0;
                char *port_place = "a";
                char *port = "a";

                if (strcmp(contain, "NICK") == 0)
                {
                    arr[3] = "NICK";
                    int new_count = count + 1;
                    int y = new_count;
                    while (buf[new_count] != ' ' && buf[new_count] != '\0')
                    {
                        new_count++;
                    }
                    nick_name = (char *) malloc(new_count - y+1);
                    if (nick_name == NULL){ perror("malloc"); exit(EXIT_FAILURE);}
                    memcpy(nick_name, &buf[y], new_count - y);
                    nick_name[new_count-y] = '\0';
                    arr[4] = nick_name;
                    
                    new_count++;
                    int o = new_count;
                    while (buf[new_count] != ' ' && buf[new_count] != '\0')
                    {
                        new_count++;
                    }
                    ip_sin = (char*) malloc(new_count - o + 1);
                    if (ip_sin == NULL){ perror("malloc"); exit(EXIT_FAILURE);}
                    memcpy(ip_sin, &buf[o], new_count - o);
                    ip_sin[new_count-o] = '\0';
                    arr[5] = ip_sin;
                    
                    new_count++;
                    int q = new_count;
                    while (buf[new_count] != ' ' && buf[new_count] != '\0')
                    {
                        new_count++;
                    }
                    that_ip = (char*) malloc(new_count - q + 1);
                    if (that_ip == NULL){ perror("malloc"); exit(EXIT_FAILURE);}
                    memcpy(that_ip, &buf[q], new_count - q);
                    that_ip[new_count-q] = '\0';
                    arr[6] = that_ip;
                    new_count++;

                    int w = new_count;
                    while (buf[new_count] != ' ' && buf[new_count] != '\0')
                    {
                        new_count++;
                    }
                    port_place = (char*) malloc(new_count - w + 1);
                    if (port_place == NULL){ perror("malloc"); exit(EXIT_FAILURE);}
                    memcpy(port_place, &buf[w], new_count - w);
                    port_place[new_count-w] = '\0';
                    arr[7] = port_place;
                    new_count++;

                    int e = new_count;
                    while (buf[new_count] != ' ' && buf[new_count] != '\0')
                    {
                        new_count++;
                    }
                    port = (char*) malloc(new_count - e + 1);
                    if (port == NULL){ perror("malloc"); exit(EXIT_FAILURE);}
                    memcpy(port, &buf[e], new_count - e);
                    port[new_count-e] = '\0';
                    arr[7] = port;

                    char *reg_nick[strlen(arr[4])];
                    char *reg_ipen[strlen(arr[4])];
                    sscanf(arr[7], "%d", &reg_port);
                    append_struct(&first, that_ip, reg_port, nick_name);
                    append_blocked(&first_blocked, nick_name);

                    if (first_ack->ack_num != -1) {
                        struct Client* reci;
                        reci = get_client(first, nick_name);

                        rec = inet_pton(AF_INET, reci->ipen, &send_to);
                        check_error(rec, "inet_pton");

                        send_to.sin_family = AF_INET;
                        send_to.sin_port = htons(reci->port);
                        send_to.sin_addr.s_addr = inet_addr(reci->ipen);

                        char *subtext = malloc(get_int_len(first_ack->ack_num) + 8);
                        if (subtext == NULL){ perror("malloc"); exit(EXIT_FAILURE);}
                        sprintf(subtext, "ACK %i OK", first_ack->ack_num);

                        int new = send_packet(msg_fd, subtext, strlen(subtext), 0,
                                        (struct sockaddr *)&send_to,
                                        sizeof(struct sockaddr_in));
                        free(subtext);
                    }

                    free(nick_name);
                    free(that_ip);
                    free(ip_sin);
                    free(port_place);
                    free(port);
                }               
            
                free(char_num);
                free(contain);
            }            

            // If the server finds a client that has not sent reg-message in 30sec
            // it will respond with a message starting with 'N'
            else if (buf != NULL && buf[0] == 'N') {
                printf("%s\n", buf);
            }
        }



        // This while loop will try to send messages two more times if they have not 
        // recieved an acknowledgement from the clients or the server

        struct Message* tmp_message = first_msg;
        while (tmp_message != NULL && first != NULL) {
            int l = 0;
            tmp_message = lookup_message(tmp_message, tmp_message->ack_num);
            
            if (tmp_message->went_trough == 0 && tmp_message->times_sent < 4) {
                if (tmp_message->message[0] == 'P') {

                    // This is for a lookup to the server if the message has 
                    // been sent two times and not recieved an acknowledgement

                    if (tmp_message->message[0] == 'P' && tmp_message->times_sent == 2) {
                        struct Client* reci;
                        if (first != NULL) {
                            reci = get_client(first, tmp_message->reciever);
                        }
                        if (reci != NULL) {
                            sprintf(str, "%d", teller);
                            char retur[strlen(reci->nick)+ strlen(str) + 14];
                            strcpy(retur, "PKT ");
                            strcat(retur, str);
                            strcat(retur, " LOOKUP ");
                            strcat(retur, reci->nick);

                            int rec = send_packet(msg_fd, retur, strlen(retur), 0,
                                                (struct sockaddr *)&server_addr,
                                                sizeof(struct sockaddr_in));
                            check_error(rec, "Send to not working");
                            teller++;
                        }
                    }
                    
                    if (strcmp(tmp_message->reciever, "SERVER") == 0) {
                        int new = send_packet(msg_fd, tmp_message->message, strlen(tmp_message->message), 0,
                                        (struct sockaddr *)&server_addr,
                                        sizeof(struct sockaddr_in));
                        check_error(new, "Send to not working");
                        tmp_message->times_sent++;
                    } else {
                        struct Client* reci;
                        if (first != NULL) {
                            reci = get_client(first, tmp_message->reciever);
                        }

                        if (reci != NULL) {
                            rec = inet_pton(AF_INET, reci->ipen, &send_to);
                            check_error(rec, "inet_pton");

                            send_to.sin_family = AF_INET;
                            send_to.sin_port = htons(reci->port);
                            send_to.sin_addr.s_addr = inet_addr(reci->ipen);

                            int new = send_packet(msg_fd, tmp_message->message, strlen(tmp_message->message), 0,
                                            (struct sockaddr *)&send_to,
                                            sizeof(struct sockaddr_in));
                            check_error(new, "Send to not working");
                            tmp_message->times_sent++;
                            break;
                        }
                    }
                }
                
                if (tmp_message->message[0] == '@') {

                    struct Client* reci;
                    if (first != NULL) {
                        reci = get_client(first, tmp_message->reciever);
                    }
                    if (reci != NULL) {
                        rec = inet_pton(AF_INET, reci->ipen, &send_to);
                        check_error(rec, "inet_pton");

                        send_to.sin_family = AF_INET;
                        send_to.sin_port = htons(reci->port);
                        send_to.sin_addr.s_addr = inet_addr(reci->ipen);

                        int i = 0;
                        while (tmp_message->message[i] != ' ') {
                            i++;
                        }
                        i++;
                        int start_mess = i;
                        while (tmp_message->message[i] != '\0') {
                            i++;
                        }
                        char *mess = malloc(i-start_mess+1);
                        if (mess == NULL){ perror("malloc"); exit(EXIT_FAILURE);}
                        memcpy(mess, &tmp_message->message[start_mess], i-start_mess);
                        mess[i-start_mess] = 0;

                        char *subtext = malloc(get_int_len(tmp_message->ack_num) + strlen(nick) + strlen(tmp_message->reciever) + strlen(mess) + 20);
                        if (subtext == NULL){ perror("malloc"); exit(EXIT_FAILURE);}
                        sprintf(subtext, "PKT %i FROM %s TO %s MSG %s", tmp_message->ack_num, nick, tmp_message->reciever, mess);

                        int new = send_packet(msg_fd, subtext, strlen(subtext), 0,
                                        (struct sockaddr *)&send_to,
                                        sizeof(struct sockaddr_in));
                        check_error(new, "Send to not working");
                        free(subtext);
                        free(mess);
                        tmp_message->times_sent++;
                        break;
                    } else {
                        tmp_message->times_sent++;
                        break;
                    }
                }
                
                tmp_message->times_sent++;
                
            } else {
                tmp_message = tmp_message->next;
                l++;
            }
        }
    
    }

    if (first != NULL) {free_linked(first);}
    if (first_blocked != NULL) {free_blocked_link(first_blocked);}
    if (first_msg != NULL) {free_messages(first_msg);}
    if (first_ack != NULL) {free_pkt_ack(&first_ack);}
    if (first_print != NULL) {free_print(&first_print);}
    free(nick);
    
    close(msg_fd);
    return EXIT_SUCCESS;
}