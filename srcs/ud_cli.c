#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <errno.h>
#include <dirent.h>
#include <limits.h>
#include <sys/types.h>
#include <sys/wait.h>

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

void cli_run(void)
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
