#include <limits.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>
#include <sys/mount.h>
#include <sys/types.h>
#include <sys/stat.h>

#include "ft_ud.h"

#define MOUNT(source, target, fstype, flags, data) \
    do { \
        if (mount(source, target, fstype, flags, data) != 0) \
        { \
            fprintf(stderr, "Failed to mount %s on %s: %s\n", \
                    source ? source : "none", target, strerror(errno)); \
        } \
        else \
        { \
            printf("Mounted %s on %s (%s)\n", \
                   source ? source : "none", target, fstype ? fstype : ""); \
        } \
    } while (0)

static void create_dir(const char *path)
{
    if (mkdir(path, 0755) < 0 && errno != EEXIST)
    {
        fprintf(stderr, "Failed to mkdir %s: %s\n", path, strerror(errno));
    }
}

int mount_init(void)
{
    int ret;

    /* Mount initial PID1 fs needed. */
    create_dir("/proc");
    create_dir("/sys");
    create_dir("/dev");
    create_dir("/run");
    create_dir("/tmp");


    MOUNT("proc", "/proc", "proc", 0, "");
    MOUNT("sysfs", "/sys", "sysfs", 0, "");
    MOUNT("devtmpfs", "/dev", "devtmpfs", 0, "");
    MOUNT("tmpfs", "/run", "tmpfs", 0, "");
    MOUNT("tmpfs", "/tmp", "tmpfs", 0, "");

    if (mount(NULL, "/", NULL, MS_REMOUNT | MS_RDONLY, NULL) == 0)
        printf("Root remounted read-only\n");

    /* Check proper fs integrity by checking no trash in /etc/fstab */
    printf(BLUE "fsck -A -y\n" RESET);
    ret = system("fsck -A -y");
    if (ret == -1)
        perror(RED "system" RESET);
    else
        printf("Filesystem check completed with return code %d\n", ret);

    if (mount(NULL, "/", NULL, MS_REMOUNT, NULL) == 0)
        printf("Root remounted read-write\n");

    printf(BLUE "swapon -a 2>/dev/null\n" RESET);
    ret = system("swapon -a 2>/dev/null");
    if (ret == -1)
        perror(RED "system" RESET);
    else
        printf("Swap activation completed with return code %d\n", ret);

    /* Mount all user defined fs from /etc/fstab */
    printf(BLUE "mount -a\n" RESET);
    system("/bin/busybox mount -a");

    create_dir("/root");
    create_dir("/home");
    create_dir("/home/jareste");


    printf(GREEN "Kernel filesystems mounted successfully.\n" RESET);

    return 0;
}
