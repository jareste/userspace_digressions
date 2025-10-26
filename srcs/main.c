#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>

#include "ud_hostname.h"
#include "ud_mount.h"
#include "ud_cli.h"
#include "ud_consoles.h"

#ifdef DEBUG
void debug_checks(void)
{
    int ret;

    ret = system("/bin/busybox ls");
    if (ret == -1)
    {
        printf("Failed to execute ls command\n");
        perror("system");
    }
    else
        printf("ls command completed with return code %d\n", ret);
}
#endif

int main()
{
    printf("Init started. (PID = %d)\n", getpid());

    mount_init();

    system("/bin/busybox syslogd");
    system("/bin/busybox crond");

    hostname_init();
    
    debug_checks();

    cli_run();

    /* Is it ok to wait until my cli is over to spawn the actual linux cli? */
    spawn_consoles();

    printf("Init finished, halting system...\n");
    while (1) {}
    system("poweroff -f");

    return 0;
}
