
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <fcntl.h>
#include <pthread.h>
#include <errno.h>
#include <sys/stat.h>
#include <signal.h>



#define BASE_FIFO_DIR "/tmp/chat_pipes"
const char* get_fifo_dir() {
    const char* env_dir = getenv("CHAT_PIPE_DIR");
    return (env_dir && strlen(env_dir) > 0) ? env_dir : "/tmp/chat_pipes";
}

typedef struct {
    char username[100];
    char pipe_c2s[256];
    char pipe_s2c[256];
} Client;

int fd2; // global read end from server
int fd1;


char pipe_c2s[256];  // Paths for cleanup
char pipe_s2c[256];
void handle_sigint(int sig) {
    printf("\n[Stopped abruptly]\n");

    if (fd1 != -1) {
        
        write(fd1, "/exit\n", strlen("/exit\n"));
        close(fd1);
    }
    if (fd2 != -1) close(fd2);

    unlink(pipe_c2s);
    unlink(pipe_s2c);

    printf("[Disconnected.]\n");
    exit(0);
}


void* read_from_server(void* arg) {
    char buffer[1024];
    while (1) {
        int n = read(fd2, buffer, sizeof(buffer) - 1);
        if (n > 0) {
            buffer[n] = '\0';
            printf("%s", buffer);
        } else if (n == 0) {
            printf("\n[Server disconnected. You cannot chat anymore. Press Enter to exit.]\n");
            exit(0);
        } else {
            perror("Error reading from server");
        }
    }
    return NULL;
}

int main() {
    Client client;

    
    printf("Enter your username: ");
    if (fgets(client.username, sizeof(client.username), stdin) == NULL) {
        fprintf(stderr, "Failed to read username.\n");
        exit(EXIT_FAILURE);
    }
    client.username[strcspn(client.username, "\n")] = '\0'; // remove newline

    if (strlen(client.username) == 0) {
        fprintf(stderr, "Username cannot be empty.\n");
        exit(EXIT_FAILURE);
    }

    
    // Create FIFO pipe paths
    snprintf(client.pipe_c2s, sizeof(client.pipe_c2s), "%s/%s_to_server", get_fifo_dir(),client.username);
    snprintf(client.pipe_s2c, sizeof(client.pipe_s2c), "%s/server_to_%s",get_fifo_dir(), client.username);

    


    char reg_fifo_path[100];
    snprintf(reg_fifo_path, sizeof(reg_fifo_path), "%s/registration_fifo", get_fifo_dir());

    int reg_fd = open(reg_fifo_path, O_WRONLY);
    if (reg_fd < 0) {
        perror("open registration_fifo");
        exit(EXIT_FAILURE);
    }

    if (write(reg_fd, client.username, strlen(client.username)) < 0 ||
        write(reg_fd, "\n", 1) < 0) {
        perror("Error writing to registration pipe");
        close(reg_fd);
        exit(EXIT_FAILURE);
    }
    close(reg_fd);
    
    
    
    
    while (access(client.pipe_c2s, F_OK) == -1 || access(client.pipe_s2c, F_OK) == -1) {
        sleep(1); // wait 0.1 seconds
    }
    
     fd2 = open(client.pipe_s2c, O_RDONLY);  // server_to_<id>
   
    char buf[100];
    read(fd2, buf, sizeof(buf));  // should get "READY\n"
    if (strncmp(buf, "READY\n", 6) == 0) {
         ;
    }else if(strncmp(buf, "Duplicate Client\n", 18) == 0){
        printf("Duplicate Client. Exiting...\n");
        exit(EXIT_FAILURE);
    }
    else{
        printf("no reply from server");
        exit(EXIT_FAILURE);
    }
    
    
    fd1 = open(client.pipe_c2s, O_WRONLY);
    if (fd1 < 0) {
        perror("Error opening pipe to server");
        exit(EXIT_FAILURE);
    }
    
    
    printf("[Registered successfully]\n");
    printf("[Connected to chat server. Type messages or /exit to quit]\n");

    // Start thread to read messages from server
    pthread_t tid;
    if (pthread_create(&tid, NULL, read_from_server, NULL) != 0) {
        perror("Failed to create thread");
        close(fd1); 
        close(fd2);
        exit(EXIT_FAILURE);
    }

    char msg[1024];
    strcpy(pipe_c2s, client.pipe_c2s);
strcpy(pipe_s2c, client.pipe_s2c);

    signal(SIGINT, handle_sigint);  // Handle Ctrl+C

    while (1) {
        if (fgets(msg, sizeof(msg), stdin) == NULL ) {
            printf("[Input error. Exiting...]\n");
            break;
        }

        if (strcmp(msg, "/exit\n") == 0) {

            write(fd1, msg, strlen(msg));
            close(fd1);
            close(fd2);
            unlink(client.pipe_c2s);
            unlink(client.pipe_s2c);
            printf("[Successfully Disconnected. ]\n");
            exit(0);
        }

        if (write(fd1, msg, strlen(msg)) < 0) {
            perror("Error writing to server");
        }

       
    }
            close(fd1);
            close(fd2);
            unlink(client.pipe_c2s);
            unlink(client.pipe_s2c);
            printf("[Disconnected.]\n");
        
            

    // return 0;
}
