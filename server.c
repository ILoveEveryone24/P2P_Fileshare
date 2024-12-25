#include <stdio.h>
#include <stdlib.h>
#include <sys/socket.h>
#include <string.h>
#include <netinet/in.h>
#include <pthread.h>

#define PORT 4444

void *handle_client(void *arg){
    int client_s = *((int *)arg); 
    char buffer[4096];
    memset(buffer, 0, sizeof(buffer));

    printf("CLIENT s: %d\n", client_s);
    
    int bytes_r; 
    while((bytes_r = recv(client_s, buffer, sizeof(buffer), 0)) != 0){
        if(bytes_r < 0){
            continue;
        }
        printf("Client: %s\n", buffer);
        send(client_s, buffer, sizeof(buffer), 0);
    }
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
  
    while(1){
        int client_s = accept(s, (struct sockaddr*)&s_addr, &s_addrlen);
        if(client_s < 0){
            perror("Failed to accept client\n");
            exit(EXIT_FAILURE);
        }
        printf("before CLIENT s: %d\n", client_s);
        pthread_t tid;

        pthread_create(&tid, NULL, handle_client, (void *)&client_s);
    }

    return 0;
}
