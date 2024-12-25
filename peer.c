#include <stdio.h>
#include <stdlib.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <unistd.h>
#include <errno.h>
#include <string.h>
#include <arpa/inet.h>

#define SIG_SERVER_IP "127.0.0.1"
#define PORT 4444

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

    while(1){
        if(fgets(request, sizeof(request), stdin) != NULL){
            request[strcspn(request, "\n")] = '\0';
            send(s, request, sizeof(request), 0);
        }
        recv(s, response, sizeof(response), 0);
        printf("Server: %s\n", response);
    }

    return 0;
}
