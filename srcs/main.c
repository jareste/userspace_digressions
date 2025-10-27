#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>

#include "ft_ud.h"
#include "ud_hostname.h"
#include "ud_mount.h"
#include "ud_cli.h"
#include "ud_consoles.h"
#include "ud_network.h"
#include "ud_modules.h"
#include "ud_daemon.h"
#include "ud_signals.h"
#include "ud_log.h"

#ifdef DEBUG
void debug_checks(void)
{
    int ret;

    ret = system("/bin/busybox ls");
    if (ret == -1)
    {
        log_msg(LOG_LEVEL_INFO, "Failed to execute ls command\n");
        perror("system");
    }
    else
        log_msg(LOG_LEVEL_INFO, "ls command completed with return code %d\n", ret);
}
#else
#define debug_checks() \
    do { } while (0)
#endif

int main()
{
    mount_init();

    log_init();

    log_msg(LOG_LEVEL_INFO, "Init started. (PID = %d)\n", getpid());

    hostname_init();

    modules_load();

    network_init();

    signals_init();

    daemon_init();

    debug_checks();

    cli_run();

    system("busybox crond -f -l 8 -L /var/log/cron.log & 2>/dev/null");

    /* Is it ok to wait until my cli is over to spawn the actual linux cli? */
    spawn_consoles();

    log_msg(LOG_LEVEL_INFO, "Init finished\n");
    while (1) {}
    system("poweroff -f");

    return 0;
}
