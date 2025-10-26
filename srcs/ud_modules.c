#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "ft_ud.h"

#define MODULES_FILE "/etc/modules"

void modules_load(void)
{
    FILE *f;
    char path[256];
    char cmd[512];
    char line[128];
    int ret;

    f = fopen(MODULES_FILE, "r");
    if (!f)
    {
        perror(RED "fopen " MODULES_FILE RESET);
        return;
    }

    while (fgets(line, sizeof(line), f))
    {
        if (line[0] == '#' || line[0] == '\n')
            continue;

        line[strcspn(line, "\n")] = '\0';

        snprintf(path, sizeof(path), "/lib/modules/%s", line);

        snprintf(cmd, sizeof(cmd), "insmod %s 2>/dev/null", path);
        ret = system(cmd);
        if (ret == -1)
            perror(RED "system" RESET);
        else
            printf(GREEN "Loaded module %s\n" RESET, line);
    }

    fclose(f);
}
