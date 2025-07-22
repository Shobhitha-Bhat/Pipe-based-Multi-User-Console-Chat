#include <fcntl.h>
#include <unistd.h>
#include <sys/stat.h>
#include <poll.h>
#include <errno.h>
#include <sys/types.h>
#include <sys/stat.h>
#include<stdio.h>
#include<stdlib.h>
#include<string.h>
#include <signal.h>


#define MAX_MSG 256
const char* get_fifo_dir() {
    const char* env_dir = getenv("CHAT_PIPE_DIR");
    return (env_dir && strlen(env_dir) > 0) ? env_dir : "/tmp/chat_pipes";
}


struct Client {
    char id[50];
    int rfd;
    int wfd;
    int active;
};

struct Client* clients = NULL;
struct pollfd* fds = NULL;
int client_count = 0;




int regfd;

int reg_fd;  

void cleanup(int signo) {
    // printf("\n[SERVER] Caught signal %d, , signo);

    // Close and unlink registration FIFO
    close(reg_fd);
    char reg_fifo_path[100];
    snprintf(reg_fifo_path, sizeof(reg_fifo_path), "%s/registration_fifo", get_fifo_dir());
    unlink(reg_fifo_path);

    // Close all client FIFOs
    for (int i = 0; i < client_count; i++) {
        if (clients[i].active) {
            close(clients[i].rfd);
            close(clients[i].wfd);

            char to_server[100], to_client[100];
            snprintf(to_server, sizeof(to_server), "%s/%s_to_server", get_fifo_dir(), clients[i].id);
            snprintf(to_client, sizeof(to_client), "%s/server_to_%s", get_fifo_dir(), clients[i].id);

            unlink(to_server);
            unlink(to_client);
        }
    }

    free(clients);
    free(fds);

    printf("    Server Closed.\n");
    exit(0);
}

void ensure_dir_exists(const char *path) {
    struct stat st = {0};
    if (stat(path, &st) == -1) {
        if (mkdir(path, 0777) == -1) {
            perror("mkdir failed");
            exit(EXIT_FAILURE);
        }
    }
}


void create_fifo(const char *path) {
    unlink(path); // Remove if already exists
    if (mkfifo(path, 0666) == -1 && errno != EEXIST) {
        perror("mkfifo");
        exit(EXIT_FAILURE);
    }
}

int is_duplicate_client(const char* client_id) {
    for (int i = 0; i < client_count; i++) {
        if (clients[i].active && strcmp(clients[i].id, client_id) == 0) {
            return 1;
        }
    }
    return 0;
}


void broadcast(const char *msg, const char *sender_id) {
    char formatted_msg[MAX_MSG];
    snprintf(formatted_msg,sizeof(formatted_msg),"\n[%s] : %s\n",sender_id,msg);
    for (int i = 0; i < client_count; i++) {
        if (clients[i].active && strcmp(clients[i].id, sender_id) != 0) {  //broadcast to all except the sender
            write(clients[i].wfd, formatted_msg, strlen(formatted_msg));
        }
    }
}




void broadcastexit(const char *msg,const char *sender_id) {
    for (int i = 0; i < client_count; i++) {
        if (clients[i].active && strcmp(clients[i].id, sender_id) != 0) {
            write(clients[i].wfd, msg, strlen(msg));
        }
    }
}

void broadcastjoin(const char *msg,const char *sender_id) {
    for (int i = 0; i < client_count; i++) {
        if (clients[i].active && strcmp(clients[i].id, sender_id) != 0) {
            write(clients[i].wfd, msg, strlen(msg));
        }
    }
}

void list_members(const char *sender_id){
    char members[1024]="Active Members:\n";
    for (int i = 0; i < client_count; i++) {
        if (clients[i].active && strcmp(clients[i].id, sender_id) != 0) {
            strcat(members, clients[i].id);
            strcat(members, "\n");
        }
    }
    for (int i = 0; i < client_count; i++) {
        if (clients[i].active && strcmp(clients[i].id, sender_id) == 0) {
            write(clients[i].wfd, members, strlen(members));
            break;
        }
    }
}


void add_client(const char* client_id) {
    clients = realloc(clients, sizeof(struct Client) * (client_count + 1));
    fds = realloc(fds, sizeof(struct pollfd) * (client_count + 2)); // +1 for reg_fd

    char to_server[100], to_client[100];
    const char* base_dir = get_fifo_dir();
    snprintf(to_server, sizeof(to_server), "%s/%s_to_server", base_dir, client_id);
    snprintf(to_client, sizeof(to_client), "%s/server_to_%s", base_dir, client_id);


    create_fifo(to_server);
    create_fifo(to_client);

    int rfd = open(to_server, O_RDONLY | O_NONBLOCK);

    // Prevent blocking on open(to_client, O_WRONLY)
    int dummy_fd = open(to_client, O_RDONLY | O_NONBLOCK);
    int wfd = open(to_client, O_WRONLY);
    if (is_duplicate_client(client_id)) {
                    printf("[SERVER] Duplicate client '%s' ignored.\n", client_id);
                    write(wfd, "Duplicate Client\n", 18);
                    return;
                }

    if (rfd < 0 || wfd < 0) {
        perror("open client FIFO");
        return;
    }

    write(wfd, "READY\n", 6);

    strcpy(clients[client_count].id, client_id);
    clients[client_count].rfd = rfd;
    clients[client_count].wfd = wfd;
    clients[client_count].active = 1;

    fds[client_count + 1].fd = rfd;
    fds[client_count + 1].events = POLLIN;

    printf("[SERVER] Added client: %s\n", client_id);
    char msg[100];
    snprintf(msg,sizeof(msg),"%s joined the chat\n\n",clients[client_count].id);
    broadcastjoin(msg,clients[client_count].id);
    client_count++;
}

int main() {
    
    ensure_dir_exists(get_fifo_dir());

    signal(SIGINT, cleanup);
    // Step 2: Create and open the registration FIFO
    char reg_fifo_path[100];
    snprintf(reg_fifo_path, sizeof(reg_fifo_path), "%s/registration_fifo", get_fifo_dir());
    create_fifo(reg_fifo_path);

    reg_fd = open(reg_fifo_path, O_RDONLY | O_NONBLOCK);
    if (reg_fd < 0) {
        perror("open registration_fifo");
        exit(EXIT_FAILURE);
    }

    fds = malloc(sizeof(struct pollfd));
    fds[0].fd = reg_fd;
    fds[0].events = POLLIN;

    printf("[SERVER] Running... Waiting for clients.\n");

    char buffer[MAX_MSG];

    while (1) {
        int ret = poll(fds, client_count + 1, 1000);
        if (ret < 0) {
            perror("poll");
            continue;
        }

        // Step 3: Handle new registrations
        if (fds[0].revents & POLLIN) {
            char client_id[50];
            int bytes = read(reg_fd, client_id, sizeof(client_id) - 1);
            if (bytes > 0) {
                client_id[bytes] = '\0';
                client_id[strcspn(client_id, "\n")] = '\0';

                while (strlen(client_id) > 0 && (client_id[strlen(client_id) - 1] == ' ' || client_id[strlen(client_id) - 1] == '\t'))
                client_id[strlen(client_id) - 1] = '\0';

                if (strlen(client_id) == 0) {
                // printf("[SERVER] Ignored empty client ID\n");
                continue;
                }

                add_client(client_id);
            }
        }

        // Step 4: Handle client messages
        for (int i = 0; i < client_count; i++) {
            if (clients[i].active && fds[i + 1].revents & POLLIN) {
                int n = read(clients[i].rfd, buffer, MAX_MSG);
                if (n > 0) {
                    buffer[n] = '\0';
                    if (strcmp(buffer, "/exit\n") == 0) {
                        char exit_msg[MAX_MSG];
                        snprintf(exit_msg, sizeof(exit_msg), "%s left the chat.\n", clients[i].id);
                        printf("[SERVER] %s", exit_msg);  // Server log
                        broadcastexit(exit_msg, clients[i].id); 
                    }else if(strcmp(buffer,"/members\n")==0){
                        printf("Listing participants for %s\n",clients[i].id);
                        list_members(clients[i].id);
                    }
                    
                    else{
                        printf("[%s]: %s", clients[i].id, buffer);
                        broadcast(buffer, clients[i].id);
                    }
                } else if (n == 0) {
                    char exit_msg[MAX_MSG];
                    snprintf(exit_msg, sizeof(exit_msg), "%s left the chat.\n\n", clients[i].id);
                    printf("[SERVER] %s", exit_msg);
                    
                    broadcastexit(exit_msg, clients[i].id); 
                    clients[i].active = 0;
                    close(clients[i].rfd);
                    close(clients[i].wfd);
                   
                }
            }
        }
    }

    close(reg_fd);
    unlink(reg_fifo_path);
    return 0;
}
