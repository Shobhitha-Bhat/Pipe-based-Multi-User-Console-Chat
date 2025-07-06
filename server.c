#include <fcntl.h>
#include <unistd.h>
#include <sys/stat.h>
#include <poll.h>
#include <errno.h>
#include <sys/types.h>
#include <sys/stat.h>

#define MAX_MSG 256
#define BASE_FIFO_DIR "/tmp/chat_pipes"

struct Client {
    char id[50];
    int rfd;
    int wfd;
    int active;
};

struct Client* clients = NULL;
struct pollfd* fds = NULL;
int client_count = 0;

void ensure_dir_exists(const char *path) {
    struct stat st = {0};
    if (stat(path, &st) == -1) {
        if (mkdir(path, 0777) == -1) {
            perror("mkdir failed");
            exit(EXIT_FAILURE);
        }
    }
}

// Create a FIFO at a specific path
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

// Broadcast message to all clients except the sender
void broadcast(const char *msg, const char *sender_id) {
    for (int i = 0; i < client_count; i++) {
        if (clients[i].active && strcmp(clients[i].id, sender_id) != 0) {
            write(clients[i].wfd, msg, strlen(msg));
        }
    }
}


// Add a new client dynamically
void add_client(const char* client_id) {
    clients = realloc(clients, sizeof(struct Client) * (client_count + 1));
    fds = realloc(fds, sizeof(struct pollfd) * (client_count + 2)); // +1 for reg_fd

    char to_server[100], to_client[100];
    snprintf(to_server, sizeof(to_server), "%s/%s_to_server", BASE_FIFO_DIR, client_id);
    snprintf(to_client, sizeof(to_client), "%s/server_to_%s", BASE_FIFO_DIR, client_id);

    create_fifo(to_server);
    create_fifo(to_client);

    int rfd = open(to_server, O_RDONLY | O_NONBLOCK);
    int wfd = open(to_client, O_WRONLY);

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
    client_count++;
}
