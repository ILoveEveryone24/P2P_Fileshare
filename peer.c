#include <stdio.h>
#include <stdlib.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <unistd.h>
#include <errno.h>
#include <string.h>
#include <arpa/inet.h>
#include <pthread.h>

#define SIG_SERVER_IP "127.0.0.1"
#define PORT 4444

pthread_mutex_t lock;

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

    while(1){};

    return NULL;
}

void *connect_to_peers(void *arg){
    
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

    char request[4096];
    memset(request, 0, sizeof(request));
    char response[4096];
    memset(response, 0, sizeof(response));

    pthread_t tid;

    int thread_created = pthread_create(&tid, NULL, listen_for_peers, (void *)&s);
    if(thread_created != 0){
        perror("Failed to create a thread\n");
    }
    else {
        pthread_detach(tid);
    }

    while(1){
        memset(request, 0, sizeof(request));
        memset(response, 0, sizeof(response));
        if(fgets(request, sizeof(request), stdin) != NULL){
            request[strcspn(request, "\n")] = '\0';
            send(s, request, sizeof(request), 0);
        }

        recv(s, response, sizeof(response), 0);

        printf("Server: %s\n", response);
    }

    return 0;
}
