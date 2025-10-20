#define _GNU_SOURCE
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/mount.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <errno.h>
#include <string.h>
#include <dirent.h>
#include <limits.h>

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

typedef struct
{
    char *name;
    void (*func)(void* arg);
} t_cmd;

static void m_ls(void* arg);
static void m_cd(void* arg);

static const t_cmd m_cmds[] =
{
    {"ls", m_ls},
    {"cd", m_cd},
    {"exit", NULL},
    {NULL, NULL}
};

static void create_dir(const char *path)
{
    if (mkdir(path, 0755) < 0 && errno != EEXIST)
    {
        fprintf(stderr, "Failed to mkdir %s: %s\n", path, strerror(errno));
    }
}

static void m_ls(void* arg)
{
    const char* path = (const char*)arg;
    struct dirent *entry;
    DIR *dir;
    
    path = path[0] ? path : ".";
    dir = opendir(path);
    if (!dir)
    {
        perror("opendir");
        return;
    }

    while ((entry = readdir(dir)) != NULL)
        printf("%s  ", entry->d_name);

    printf("\n");

    closedir(dir);
}

static void m_cd(void* arg)
{
    const char* path = (const char*)arg;

    if (chdir(path) != 0)
        perror("chdir");
}

static void m_cli(void)
{
    char cwd[PATH_MAX];
    char input[256];
    const char* path;
    const char* arg;
    size_t cmd_len;
    size_t i;

    while (1)
    {
        if (!getcwd(cwd, sizeof(cwd)))
            strcpy(cwd, "?");

        printf("%s$ ", cwd);
        fflush(stdout);

        memset(input, 0, sizeof(input));
        if (!fgets(input, sizeof(input), stdin))
            break;

        input[strcspn(input, "\n")] = '\0';

        if (input[0] == '\0')
            continue;

        for (i = 0; m_cmds[i].name != NULL; i++)
        {
            cmd_len = strlen(m_cmds[i].name);
            if (strncmp(input, m_cmds[i].name, cmd_len) == 0 &&
                (input[cmd_len] == ' ' || input[cmd_len] == '\0'))
            {
                if (m_cmds[i].func)
                {
                    arg = input + cmd_len;
                    while (*arg == ' ') arg++;
                    m_cmds[i].func((void*)arg);
                }
                else
                {
                    return;
                }
                break;
            }
        }

        if (m_cmds[i].name == NULL)
            printf("Unknown command: %s\n", input);

    }
}

int main(void)
{
    printf("Init started. (PID = %d)\n", getpid());

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

    system("fsck -A -y");

    if (mount(NULL, "/", NULL, MS_REMOUNT, NULL) == 0)
        printf("Root remounted read-write\n");

    system("swapon -a 2>/dev/null");

    printf("Kernel filesystems mounted successfully.\n");

    m_cli();

    printf("Init finished, halting system...\n");
    system("poweroff -f");

    return 0;
}
