#include <stdio.h>
#include <stdlib.h>
#include <sys/socket.h>
#include <string.h>
#include <netinet/in.h>
#include <pthread.h>
#include <unistd.h>

#define PORT 4444

struct peer{
    int id;
    char ip[20];
    int port;
};

struct client_struct{
    struct peer **client_list;
    int *client_list_size;
    int client_s;
    pthread_mutex_t *lock;
};

void parse_info(struct peer *client, char *info){
}

void remove_client(struct peer **list, int *size, int id){
    int list_loc = -1;
    for(int i = 0; i < *size; i++){
        if((*list)[i].id == id){
            list_loc = i;
            break;
        }
    }
    if(list_loc != -1){
        for(int i = list_loc; i < *size - 1; i++){
            (*list)[i] = (*list)[i+1];
        }
    }
    (*size)--;
    if(*size > 0){
        struct peer *temp = realloc(*list, *size * sizeof(struct peer));
        if(temp == NULL){
            perror("Failed to allocate memory\n");
            free(*list);
            exit(EXIT_FAILURE);
        }
        *list = temp;
    }
    else{
        memset(list[0], 0, sizeof(struct peer));
    }
}

void *handle_client(void *arg){
    struct client_struct c_struct = *((struct client_struct *)arg); 
    char buffer[4096];
    memset(buffer, 0, sizeof(buffer));

    printf("CLIENT %d CONNECTED\n", c_struct.client_s);

    int bytes_r; 
    while((bytes_r = recv(c_struct.client_s, buffer, sizeof(buffer), 0)) != 0){
        if(bytes_r < 0){
            continue;
        }
        if(strcmp(buffer, "/list") == 0){
            pthread_mutex_lock(c_struct.lock);
            memset(buffer, 0, sizeof(buffer));
            for(int i = 0; i < *c_struct.client_list_size; i++){
                char c_str[100] = {0};
                struct peer tmp_peer = (*c_struct.client_list)[i];
                printf("CLIENT %d\n", tmp_peer.id);
                snprintf(c_str, sizeof(c_str), "id: %d, ip: %s, port: %d\n", tmp_peer.id, tmp_peer.ip, tmp_peer.port);
                strcat(buffer, c_str);
            }
            pthread_mutex_unlock(c_struct.lock);
            send(c_struct.client_s, buffer, sizeof(buffer), 0);
        }
        else{
            printf("Client %d: %s\n", c_struct.client_s, buffer);
            send(c_struct.client_s, buffer, sizeof(buffer), 0);
        }
        memset(buffer, 0, sizeof(buffer));
    }
    pthread_mutex_lock(c_struct.lock);
    remove_client(c_struct.client_list, c_struct.client_list_size, c_struct.client_s);
    pthread_mutex_unlock(c_struct.lock);
    printf("CLIENT %d DISCONNECTED\n", c_struct.client_s);
    close(c_struct.client_s);

    return NULL;
}

int main(){
    int s = socket(AF_INET, SOCK_STREAM, 0);
    if(s < 0){
        perror("Failed to create a socket\n");
        exit(EXIT_FAILURE);
    }

    struct sockaddr_in s_addr;
    socklen_t s_addrlen = sizeof(s_addr);

    s_addr.sin_family = AF_INET;
    s_addr.sin_port = htons(PORT);
    s_addr.sin_addr.s_addr = INADDR_ANY;
    
    if(bind(s, (struct sockaddr*)&s_addr, s_addrlen) < 0){
        perror("Failed to bind socket\n");
        exit(EXIT_FAILURE);
    }

    if(listen(s, 5) < 0){
        perror("Failed to listen\n");
        exit(EXIT_FAILURE);
    }

    int client_cnt = 0;
    struct peer *clients = NULL;

    char info[4096] = {0};

    pthread_mutex_t lock;
    pthread_mutex_init(&lock, NULL);
  
    while(1){
        int client_s = accept(s, (struct sockaddr*)&s_addr, &s_addrlen);
        if(client_s < 0){
            perror("Failed to accept client\n");
            exit(EXIT_FAILURE);
        }
        
        pthread_mutex_lock(&lock);
        client_cnt++;
        struct peer *temp = realloc(clients, client_cnt * sizeof(struct peer));
        if(temp == NULL){
            perror("Failed to allocate memory\n");
            exit(EXIT_FAILURE);
        }
        clients = temp;

        pthread_mutex_unlock(&lock);

        recv(client_s, info, sizeof(info), 0);

        struct peer client;

        client.id = client_s;
        strncpy(client.ip, strtok(info, ","), sizeof(client.ip) - 1);
        client.port = atoi(strtok(NULL, ","));

        clients[client_cnt - 1] = client;

        struct client_struct c_struct;
        c_struct.client_list = &clients;
        c_struct.client_list_size = &client_cnt;
        c_struct.client_s = client_s;
        c_struct.lock = &lock;

        pthread_t tid;

        int thread_created = pthread_create(&tid, NULL, handle_client, (void *)&c_struct);
        if(thread_created != 0){
            perror("Failed to create a thread\n");
        }
        else{
            pthread_detach(tid);
        }
    }
    if(clients != NULL){
        free(clients);
    }
    pthread_mutex_destroy(&lock);
    close(s);

    return 0;
}
