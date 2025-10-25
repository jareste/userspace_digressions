#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/utsname.h>

#include "ft_ud.h"

int hostname_init(void)
{
    FILE *f;
    char hostname[64];
    const char *default_host = "jareste-";
    
    f = fopen("/etc/hostname", "r");
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

    return 0;
}