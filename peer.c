#include <stdio.h>
#include <stdlib.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <unistd.h>
#include <errno.h>
#include <string.h>
#include <arpa/inet.h>
#include <pthread.h>
#include <semaphore.h>
#include <ctype.h>

#define SIG_SERVER_IP "127.0.0.1"
#define PORT 4444

pthread_mutex_t lock;

sem_t query;

struct peer *peers = NULL;
long peer_size = 0;

struct peer{
    int id;
    char ip[20];
    int port;
};

struct client_struct{
    int server_s;
    char *client_list;
    int *client_list_size;
};

void handle_peer(){
    
}

void *listen_for_peers(void *arg){
    int server_s = *(int *)arg;

    int listen_s = socket(AF_INET, SOCK_STREAM, 0);
    if(listen_s < 0){
        perror("Failed to create a socket\n");
        return NULL;
    }

    struct sockaddr_in p_addr;
    socklen_t p_addrlen = sizeof(p_addr);

    p_addr.sin_family = AF_INET;
    p_addr.sin_port = 0;
    p_addr.sin_addr.s_addr = INADDR_ANY;

    if(bind(listen_s, (struct sockaddr *)&p_addr, p_addrlen) < 0){
        perror("Failed to bind socket\n");
        close(listen_s);
        return NULL;
    }

    if(listen(listen_s, 5) < 0){
        perror("Failed to listen\n");
        close(listen_s);
        return NULL;
    }

    if(getsockname(listen_s, (struct sockaddr *)&p_addr, &p_addrlen) < 0){
        perror("Failed to get socket name\n");
        close(listen_s);
        return NULL;
    }

    int port = ntohs(p_addr.sin_port);
    char ip[INET_ADDRSTRLEN];
    if(inet_ntop(AF_INET, &p_addr.sin_addr, ip, sizeof(ip)) == NULL){
        perror("Failed to retrieve IP address\n");
        close(listen_s);
        return NULL;
    }

    printf("Assigned ip: %s\n", ip);
    printf("Assigned port: %d\n", port);

    char info[4096] = {0};
    char port_str[10] = {0};

    strcpy(info, ip);
    
    snprintf(port_str, sizeof(port_str), ",%d", port);

    strcat(info, port_str);

    send(server_s, info, sizeof(info), 0);

    sem_post(&query);

    while(1){
        int peer_s = accept(listen_s, (struct sockaddr*)&p_addr, &p_addrlen);
        if(peer_s < 0){
            perror("Failed to accept peer\n");
            continue;
        }
        char buffer[100];
        printf("Received:\n");
        recv(peer_s, buffer, sizeof(buffer), 0);
        printf("buffer:%s\n", buffer);

        //CHECK IF IP/PORT IS ON THE LIST
        //SHARE CHUNKS
    };

    close(listen_s);

    return NULL;
}

void *connect_to_peers(void *arg){
    //CHECK THE PEER LIST
    //ITERATE THROUGH PEERS AND TRY TO CONNECT
    //FOR NOW NO EXTRA THREADS
    //QUERY FOR CHUNKS
    //IF MISSING THEN DOWNLOAD ELSE DISCONNECT
    //REPEAT
    while(1){
        pthread_mutex_lock(&lock);
        for(int i = 0; i < peer_size; i++){
            int peer_s = socket(AF_INET, SOCK_STREAM, 0);
            if(peer_s < 0){
                perror("Failed to create a socket\n");
                continue;
            }
            struct sockaddr_in p_addr;
            socklen_t p_addrlen = sizeof(p_addr);
            p_addr.sin_family = AF_INET;
            p_addr.sin_port = htons(peers[i].port);
            inet_pton(AF_INET, peers[i].ip, &p_addr.sin_addr);

            int connected = connect(peer_s, (struct sockaddr*)&p_addr, p_addrlen);
            if(connected < 0){
                perror("Failed to connect to peer\n");
                close(peer_s);
                continue;
            }
            send(peer_s, "Hello, from peer!", sizeof("Hello, from peer!"), 0);

            close(peer_s);
        }
        pthread_mutex_unlock(&lock);
        sleep(5);
    }
    
    return NULL;
}

int main(){
    int s = socket(AF_INET, SOCK_STREAM, 0);
    if(s < 0){
        perror("Failed to create a socket");
        exit(EXIT_FAILURE);
    }

    struct sockaddr_in s_addr;
    socklen_t s_addrlen = sizeof(s_addr);
    s_addr.sin_family = AF_INET;
    s_addr.sin_port = htons(PORT);
    inet_pton(AF_INET, SIG_SERVER_IP, &s_addr.sin_addr);

    printf("Connecting");

    int connected = connect(s, (struct sockaddr*)&s_addr, s_addrlen);
    while(connected < 0){
        if(errno != EINPROGRESS && errno != ECONNREFUSED){
            perror("Failed to connect to the server");
            exit(EXIT_FAILURE);
        }
        sleep(1);
        connected = connect(s, (struct sockaddr*)&s_addr, s_addrlen);
        printf(".");
    }
    printf("\n");
    printf("CONNECTED\n");

    sem_init(&query, 0, 0);
    pthread_t listen_tid;

    int listen_thread_created = pthread_create(&listen_tid, NULL, listen_for_peers, (void *)&s);
    if(listen_thread_created != 0){
        perror("Failed to create a listen thread\n");
        exit(EXIT_FAILURE);
    }
    else {
        pthread_detach(listen_tid);
    }

    char request[6];

    sem_wait(&query);
    sem_destroy(&query);

    pthread_t connect_tid;

    int connect_thread_created = pthread_create(&connect_tid, NULL, connect_to_peers, NULL);
    if(connect_thread_created != 0){
        perror("Failed to create a listen thread\n");
        exit(EXIT_FAILURE);
    }
    else{
        pthread_detach(connect_tid);
    }


    char peer_list[4096];

    while(1){
        pthread_mutex_lock(&lock);
        memset(request, 0, sizeof(request));

        strcpy(request, "/list");

        send(s, request, sizeof(request), 0);

        memset(peer_list, 0, sizeof(peer_list));

        recv(s, peer_list, sizeof(peer_list), 0);
        int i = 0;
        char peer_size_arr[10] = {0};
        while(isdigit(peer_list[i])){
            peer_size_arr[i] = peer_list[i];
            i++;
        }
        peer_size_arr[i] = '\0';
        char *endptr;
        errno = 0;

        peer_size = strtol(peer_size_arr, &endptr, 10);
        if(errno != 0){
            perror("Failed to convert peer_size\n");
        }
        else if(*endptr != '\0'){
            fprintf(stderr, "Invalid character found:%s\n", endptr);
        }
        else if(peer_size != 0){
            struct peer *tmp = realloc(peers, peer_size * sizeof(struct peer));
            if(tmp == NULL){
                perror("Failed to allocate memory\n");
                if(peers != NULL){
                    free(peers);
                    peers = NULL;
                }
            }
            else{
                peers = tmp;
            }
        }
        else{
            if(peers != NULL){
                free(peers);
            }
            peers = NULL;
        }
        if(peers != NULL){
            char *data = strtok(peer_list + i, ",");
            for(int n = 0; n < peer_size; n++){
                peers[n].id = atoi(data);
                data = strtok(NULL, ",");
                printf("id: %d\n", peers[n].id);

                strncpy(peers[n].ip, data, sizeof(peers[n].ip)-1);
                peers[n].ip[sizeof(peers[n].ip) - 1] = '\0';
                data = strtok(NULL, ",");
                printf("ip: %s\n", peers[n].ip);

                peers[n].port = atoi(data);
                data = strtok(NULL, ",");
                printf("port: %d\n", peers[n].port);
            }
        }
        printf("SIZE: %ld\n", peer_size);

        pthread_mutex_unlock(&lock);

        sleep(5);
    }
    if(peers != NULL){
        free(peers);
    }

    close(s);

    return 0;
}
