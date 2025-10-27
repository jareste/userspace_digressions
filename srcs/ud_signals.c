#include <signal.h>
#include <sys/wait.h>
#include <stddef.h>
#include "ud_cli.h"
#include "ud_daemon.h"
#include "ft_ud.h"

void sigchld_handler(int signo)
{
    int status;
    pid_t pid;

    (void)signo;
    while ((pid = waitpid(-1, &status, WNOHANG)) > 0)
        ;

    monitor_daemons();
}

int signals_init(void)
{
    struct sigaction sa;
    sa.sa_handler = sigchld_handler;
    sigemptyset(&sa.sa_mask);
    sa.sa_flags = SA_RESTART | SA_NOCLDSTOP;
    sigaction(SIGCHLD, &sa, NULL);

    signal(SIGINT, cli_handle_signal);
    signal(SIGTERM, cli_handle_signal);
    return 0;
}
