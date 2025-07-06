/*
=================
Required header files..
=================

=====================
a structure definition containing client info like - username , 
                                          pipe names it wants the server to create 
                                          any extra info
=====================



function1(){
            while(true){
                read( fd2,  " the content that server broadcasted")

                // sleep(1)
            }
}



main{

i) ask user for client name

ii) put that info the struct

iii) open regpipe and notify itself to the server
iv) wait till it gets acknowledgement from server to proceed with chatting

once acknowledged,

    int fd1 = open( client to server pipe )
    int fd2 = open(server to client pipe)

v)  pthread( read from server for other client messages , function1())

vi) loop(){
       
        fgets( get input from stdin )

        if( strcmp(msg == "/exit") ){
        write( fd1, " client is leaving ")
        close(client to server pipe )
        close(server to client pipe)
        unlink(client to server pipe )
        unlink(server to client pipe)

        exit(0)
        }

        write(fd1, " the text that user entered")                
                    
}

} 


*/


#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <fcntl.h>
#include <pthread.h>

typedef struct {
    char username[100];
    char pipe_c2s[256];
    char pipe_s2c[256];
} Client;

int fd2; 

void* read_from_server(void* arg) {
    char buffer[1024];
    while (1) {
        int n = read(fd2, buffer, sizeof(buffer) - 1);
        if (n > 0) {
            buffer[n] = '\0';
            printf("Server: %s", buffer);
        }
    }
    return NULL;
}

int main() {
    Client client;
    printf("Enter your username: ");
    fgets(client.username, sizeof(client.username), stdin);
    client.username[strcspn(client.username, "\n")] = '\0'; 

    sprintf(client.pipe_c2s, "./Pipes/%s_to_s", client.username);
    sprintf(client.pipe_s2c, "./Pipes/server_to_%s", client.username);


    const char* regpipe = getenv("REGPIPE");
    

    int reg_fd = open(regpipe, O_WRONLY);

    write(reg_fd, client.username, strlen(client.username));
    write(reg_fd, "\n", 1);
    close(reg_fd);

    int fd1 = open(client.pipe_c2s, O_WRONLY);
    fd2 = open(client.pipe_s2c, O_RDONLY);

    pthread_t tid;
    pthread_create(&tid, NULL, read_from_server, NULL);

    char msg[1024];
    while (1) {
        fgets(msg, sizeof(msg), stdin);

        if (strcmp(msg, "/exit\n") == 0) {
            write(fd1, "Client is leaving\n", 19);
            close(fd1);
            close(fd2);
            unlink(client.pipe_c2s);
            unlink(client.pipe_s2c);
            exit(0);
        }

        write(fd1, msg, strlen(msg));
    }

    return 0;
}
