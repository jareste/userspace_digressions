#include <sys/types.h>
#include <unistd.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>
#include "ft_ud.h"
#include "ud_log.h"

int spawn_tty(const char *tty, const char *baud, const char *term)
{
    pid_t pid;
    
    pid = fork();
    if (pid < 0)
    {
        log_msg(LOG_LEVEL_ERROR, "Failed to fork for %s: %s\n", tty, strerror(errno));
        return -1;
    }

    if (pid == 0)
    {
        execl("/bin/getty", "getty", "-L", baud, tty, term, NULL);
        log_msg(LOG_LEVEL_INFO, RED "Failed to exec getty on %s: %s\n" RESET, tty, strerror(errno));
        _exit(1);
    }

    /* No error control? can we assume it's happening? */
    log_msg(LOG_LEVEL_INFO, "Spawned getty on %s (PID %d)\n", tty, pid);
    return pid;
}

void spawn_consoles(void)
{
    spawn_tty("/dev/tty1", "115200", "linux");
    spawn_tty("/dev/tty2", "115200", "linux");
    spawn_tty("/dev/tty3", "115200", "linux");
    spawn_tty("/dev/ttyS0", "115200", "vt100");
}
