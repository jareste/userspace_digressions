#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <errno.h>
#include <dirent.h>
#include <limits.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <sys/socket.h>
#include <sys/un.h>
#include <signal.h>
#include "ft_ud.h"
#include "ud_daemon.h"
#include "ud_log.h"

#define SOCKET_PATH "/run/systemctl.sock"
#define BUF_SIZE 1024

typedef struct
{
    char *name;
    void (*func)(void* arg, int client_fd);
} t_cmd;

#ifdef USE_MY_LS
static void m_ls(void* arg, int client_fd);
static void m_cd(void* arg, int client_fd);
#endif

static void m_check_status(void* arg, int client_fd);
static void m_check_start(void* arg, int client_fd);
static void m_check_stop(void* arg, int client_fd);
static void m_check_reload(void* arg, int client_fd);

static const t_cmd m_cmds[] =
{
#ifdef USE_MY_LS_CD
    {"ls", m_ls},
    {"cd", m_cd},
#endif
    {"exit", NULL},
    {"status", m_check_status},
    {"start", m_check_start},
    {"stop", m_check_stop},
    {"reload", m_check_reload},
    {NULL, NULL}
};

static int m_running = 1;

#ifdef USE_MY_LS
static void m_ls(void* arg, int client_fd)
{
    const char* path = (const char*)arg;
    struct dirent *entry;
    DIR *dir;
    
    (void)client_fd;
    path = path[0] ? path : ".";
    dir = opendir(path);
    if (!dir)
    {
        perror("opendir");
        return;
    }

    while ((entry = readdir(dir)) != NULL)
        log_msg(LOG_LEVEL_INFO, "%s  ", entry->d_name);

    log_msg(LOG_LEVEL_INFO, "\n");

    closedir(dir);
}

static void m_cd(void* arg, int client_fd)
{
    const char* path = (const char*)arg;

    (void)client_fd;
    if (chdir(path) != 0)
        perror("chdir");
}

static void m_exec(char *input, int client_fd)
{
    char *argv[32];
    int argc = 0;
    char *token = strtok(input, " ");
    int status;
    pid_t pid;

    while (token && argc < 31)
    {
        argv[argc++] = token;
        token = strtok(NULL, " ");
    }
    argv[argc] = NULL;

    if (argc == 0)
        return;

    if (!getenv("PATH"))
        setenv("PATH", "/bin:/sbin:/usr/bin:/usr/sbin", 1);

    pid = fork();
    if (pid == 0)
    {
        execvp(argv[0], argv);
        dprintf(client_fd, "Command not found\n");
        _exit(1);
    }
    else if (pid > 0)
    {
        waitpid(pid, &status, 0);
    }
    else
        perror("fork");
}
#endif

static void m_check_status(void* arg, int client_fd)
{
    const char* service = (const char*)arg;
    int ret;

    ret = daemon_status(service);
    if (ret == 1)
        dprintf(client_fd, "Service %s is running.\n", service);
    else if (ret == 0)
        dprintf(client_fd, "Service %s is stopped.\n", service);
    else
        dprintf(client_fd, "Service %s not found.\n", service);
}

static void m_check_start(void* arg, int client_fd)
{
    const char* service = (const char*)arg;
    int ret;

    ret = daemon_start(service);
    if (ret == 0)
        dprintf(client_fd, "Started service: %s\n", service);
    else
        dprintf(client_fd, "Failed to start service: %s\n", service);
}

static void m_check_stop(void* arg, int client_fd)
{
    const char* service = (const char*)arg;
    int ret;

    ret = daemon_stop(service);
    if (ret == 0)
        dprintf(client_fd, "Stopped service: %s\n", service);
    else
        dprintf(client_fd, "Failed to stop service: %s\n", service);
}

static void m_check_reload(void* arg, int client_fd)
{
    (void)arg;
    (void)client_fd;
    daemons_reload();
}

void cli_handle_signal(int sig)
{
    (void)sig;
    m_running = 0;
}

static void m_cli_run(int client_fd)
{
    char buf[BUF_SIZE];
    const char* arg;
    size_t cmd_len;
    size_t i;
    ssize_t n;

    memset(buf, 0, sizeof(buf));
    n = read(client_fd, buf, sizeof(buf) - 1);
    if (n > 0)
    {
        i = 0;
        while (m_cmds[i].name)
        {
            cmd_len = strlen(m_cmds[i].name);
            if (strncmp(m_cmds[i].name, buf, cmd_len) == 0 &&
                (buf[cmd_len] == ' ' || buf[cmd_len] == '\n' || buf[cmd_len] == '\0'))
            {
                arg = buf + cmd_len;
                while (*arg == ' ')
                    arg++;

                if (m_cmds[i].func)
                    m_cmds[i].func((void*)arg, client_fd);
                else
                {
                    return;
                }
                break;
            }
            i++;
        }
        if (!m_cmds[i].name)
        {
            dprintf(client_fd, "Unknown command verb: %s\n", buf);
        }
    }
}

void m_run_server(void)
{
    int server_fd;
    int client_fd;
    struct sockaddr_un addr;

    unlink(SOCKET_PATH);

    if ((server_fd = socket(AF_UNIX, SOCK_STREAM, 0)) == -1)
    {
        perror("socket");
        exit(EXIT_FAILURE);
    }

    memset(&addr, 0, sizeof(addr));
    addr.sun_family = AF_UNIX;
    strncpy(addr.sun_path, SOCKET_PATH, sizeof(addr.sun_path) - 1);

    if (bind(server_fd, (struct sockaddr*)&addr, sizeof(addr)) == -1)
    {
        perror("bind");
        exit(EXIT_FAILURE);
    }

    if (listen(server_fd, 3) == -1)
    {
        perror("listen");
        exit(EXIT_FAILURE);
    }

    log_msg(LOG_LEVEL_INFO, "[init] Listening on %s...\n", SOCKET_PATH);

    while (m_running)
    {
        client_fd = accept(server_fd, NULL, NULL);
        if (client_fd == -1)
        {
            if (errno == EINTR)
                continue;
            perror("accept");
            break;
        }

        monitor_daemons();
        m_cli_run(client_fd);

        close(client_fd);
    }

    close(server_fd);
    unlink(SOCKET_PATH);
    log_msg(LOG_LEVEL_INFO, "[init] Shutting down.\n");

}

void cli_run(void)
{
    pid_t pid;

    pid = fork();
    if (pid == 0)
    {
        m_run_server();
        _exit(0);
    }
    else if (pid < 0)
    {
        perror("fork");
    }
}
