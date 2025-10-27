
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <limits.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <signal.h>
#include "ft_list.h"
#include "ft_ud.h"
#include "ud_log.h"

#define DAEMON_CONF_PATH "/etc/daemons.conf"

typedef struct s_daemon
{
    list_item_t l;

    char name[64];
    char path[PATH_MAX];
    pid_t pid;
    int enabled;
    int running;
} t_daemon;

static t_daemon* m_daemons = NULL;

static t_daemon* m_find_daemon(const char *name)
{
    t_daemon* d;

    d = m_daemons;
    while (d)
    {
        if (strcmp(d->name, name) == 0)
            return d;
        d = FT_LIST_GET_NEXT(&m_daemons, d);
    }
    return NULL;
}

static int m_start_daemon(t_daemon *d)
{
    pid_t pid;

    if (d->running)
        return 0;

    pid = fork();
    if (pid < 0) return -1;
    if (pid == 0)
    {
        setsid();
        execl(d->path, d->name, NULL);
        exit(EXIT_FAILURE);
    }

    d->pid = pid;
    d->running = 1;
    log_msg(LOG_LEVEL_INFO, "Started daemon %s (PID %d)\n", d->name, pid);
    return 0;
}

static int m_stop_daemon(t_daemon *d)
{
    if (!d->running) return 0;

    kill(d->pid, SIGTERM);
    waitpid(d->pid, NULL, 0);
    d->running = 0;
    return 0;
}

void m_daemons_reload_all(void)
{
    FILE *fp;
    char line[256];
    t_daemon *d;
    t_daemon *tmp;
    t_daemon *new_list = NULL;
    t_daemon *next;
    t_daemon *found;

    fp = fopen(DAEMON_CONF_PATH, "r");
    if (!fp)
        return;

    while (fgets(line, sizeof(line), fp))
    {
        if (line[0] == '#' || strlen(line) < 3)
            continue;

        t_daemon *nd = calloc(1, sizeof(t_daemon));
        if (!nd)
            continue;

        sscanf(line, "%63s %255s %d", nd->name, nd->path, &nd->enabled);
        FT_LIST_ADD_LAST(&new_list, nd);
    }
    fclose(fp);

    d = m_daemons;
    while (d)
    {
        next = FT_LIST_GET_NEXT(&m_daemons, d);
        found = NULL;

        tmp = new_list;
        while (tmp)
        {
            if (strcmp(tmp->name, d->name) == 0)
            {
                found = tmp;
                break;
            }
            tmp = FT_LIST_GET_NEXT(&new_list, tmp);
        }

        if (!found)
        {
            if (d->running)
                m_stop_daemon(d);
            FT_LIST_POP(&m_daemons, d);
            free(d);
        }
        else
        {
            strcpy(d->path, found->path);
            d->enabled = found->enabled;

            if (d->enabled && !d->running)
                m_start_daemon(d);

            FT_LIST_POP(&new_list, found);
            free(found);
        }
        d = next;
    }

    tmp = new_list;
    while (tmp)
    {
        next = FT_LIST_GET_NEXT(&new_list, tmp);

        FT_LIST_POP(&new_list, tmp);
        FT_LIST_ADD_LAST(&m_daemons, tmp);

        if (tmp->enabled)
            m_start_daemon(tmp);

        tmp = next;
    }
}

void daemons_reload(const char *name)
{
    t_daemon* d;

    if (!name)
        m_daemons_reload_all();

    d = m_find_daemon(name);
    if (!d)
        return;

    if (d->enabled && d->running)
    {
        m_stop_daemon(d);
        m_start_daemon(d);
    }
    else if (d->enabled && !d->running)
    {
        m_start_daemon(d);
    }
    else if (!d->enabled && d->running)
    {
        m_stop_daemon(d);
    }
}

void monitor_daemons(void)
{
    t_daemon* d;

    d = m_daemons;
    while (d)
    {
        if (d->running && kill(d->pid, 0) != 0)
        {
            d->running = 0;
        }
        d = FT_LIST_GET_NEXT(&m_daemons, d);
    }
}

int daemon_start(const char *name)
{
    t_daemon* d;

    d = m_find_daemon(name);
    if (!d)
        return -1;

    return m_start_daemon(d);
}

int daemon_stop(const char *name)
{
    t_daemon* d;

    d = m_find_daemon(name);
    if (!d)
        return -1;

    return m_stop_daemon(d);
}

int daemon_status(const char *name)
{
    t_daemon* d;

    d = m_find_daemon(name);
    if (!d)
        return -1;

    return d->running;
}

void daemons_load(const char *config_path)
{
    FILE *fp;
    char line[256];
    t_daemon *d;

    fp = fopen(config_path, "r");
    if (!fp)
    {
        return;
    }

    while (fgets(line, sizeof(line), fp))
    {
        if (line[0] == '#' || strlen(line) < 3)
            continue;

        d = malloc(sizeof(t_daemon));
        if (!d)
            continue;

        sscanf(line, "%63s %255s %d", d->name, d->path, &d->enabled);
        d->running = 0;
        FT_LIST_ADD_LAST(&m_daemons, d);

        if (d->enabled)
            m_start_daemon(d);
    }

    fclose(fp);
}

void daemon_init(void)
{
    daemons_load(DAEMON_CONF_PATH);
}
