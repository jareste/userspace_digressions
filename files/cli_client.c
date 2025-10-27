#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/socket.h>
#include <sys/un.h>

#define SOCKET_PATH "/run/initctl.sock"
#define BUF_SIZE 1024

int main(int argc, char *argv[])
{
    int fd;
    int i;
    ssize_t n;
    struct sockaddr_un addr;
    char message[BUF_SIZE];

    if (argc < 2)
    {
        fprintf(stderr, "Usage: %s <command> [args...]\n", argv[0]);
        exit(EXIT_FAILURE);
    }

    memset(message, 0, sizeof(message));
    for (i = 1; i < argc; i++)
    {
        strncat(message, argv[i], BUF_SIZE - strlen(message) - 1);
        if (i < argc - 1)
            strncat(message, " ", BUF_SIZE - strlen(message) - 1);
    }

    if ((fd = socket(AF_UNIX, SOCK_STREAM, 0)) == -1)
    {
        perror("socket");
        exit(EXIT_FAILURE);
    }

    memset(&addr, 0, sizeof(addr));
    addr.sun_family = AF_UNIX;
    strncpy(addr.sun_path, SOCKET_PATH, sizeof(addr.sun_path) - 1);

    if (connect(fd, (struct sockaddr*)&addr, sizeof(addr)) == -1)
    {
        perror("connect");
        exit(EXIT_FAILURE);
    }

    write(fd, message, strlen(message));

    memset(message, 0, sizeof(message));
    n = read(fd, message, sizeof(message) - 1);
    if (n > 0)
        printf("%s", message);

    close(fd);
    return 0;
}
