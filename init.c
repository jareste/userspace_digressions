#define _GNU_SOURCE
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <errno.h>
#include <string.h>
#include <dirent.h>
#include <limits.h>
#include <sys/mount.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <sys/utsname.h>

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

#define RESET   "\033[0m"
#define RED     "\033[31m"
#define GREEN   "\033[32m"
#define YELLOW  "\033[33m"
#define BLUE    "\033[34m"
#define MAGENTA "\033[35m"
#define CYAN    "\033[36m"
#define WHITE   "\033[37m"

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

static void m_exec(char *input)
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
        fprintf(stderr, "Command not found: %s\n", argv[0]);
        _exit(1);
    }
    else if (pid > 0)
    {
        waitpid(pid, &status, 0);
    }
    else
        perror("fork");
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
        {
            m_exec(input);
        }
    }
}

int main(void)
{
    int ret;
    const char *default_host = "jareste-";
    char hostname[64];

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

    ret = system("fsck -A -y");
    if (ret == -1)
    {
        printf("Failed to execute fsck command\n");
        perror("system");
    }
    else
        printf("Filesystem check completed with return code %d\n", ret);

    ret = system("/bin/busybox fsck -A -y");
    if (ret == -1)
    {
        printf("Failed to execute fsck command\n");
        perror("system");
    }
    else
        printf("Filesystem check completed with return code %d\n", ret);

    if (mount(NULL, "/", NULL, MS_REMOUNT, NULL) == 0)
        printf("Root remounted read-write\n");

    ret = system("swapon -a 2>/dev/null");
    printf("Swap activation completed with return code %d\n", ret);

    system("/bin/busybox mount -a");
    system("/bin/busybox syslogd");
    system("/bin/busybox crond");

    FILE *f = fopen("/etc/hostname", "r");
    if (f)
    {
        if (fgets(hostname, sizeof(hostname), f))
        {
            hostname[strcspn(hostname, "\n")] = '\0';
            sethostname(hostname, strlen(hostname));
            printf(GREEN "Hostname set to: %s\n" RESET, hostname);
        }
        fclose(f);
    }
    else
    {
        
        if (sethostname(default_host, strlen(default_host)) == 0)
            printf("No /etc/hostname found. Hostname set to default: %s\n", default_host);
        else
            perror("sethostname");
    }

    
    printf("Kernel filesystems mounted successfully.\n");

    /* easy way of validation that busybox it's ok. */
    ret = system("/bin/busybox ls");
    if (ret == -1)
    {
        printf("Failed to execute ls command\n");
        perror("system");
    }
    else
        printf("ls command completed with return code %d\n", ret);
    /* erase */

    m_cli();

    printf("Init finished, halting system...\n");
    system("poweroff -f");

    return 0;
}
