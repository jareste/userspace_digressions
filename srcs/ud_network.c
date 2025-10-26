#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <errno.h>
#include <sys/types.h>

#include "ft_ud.h"

#define NETCONF_PATH "/etc/net.conf"

static void start_dhcp(const char *iface)
{
    execlp("udhcpc", "udhcpc", "-i", iface, "-q", NULL);
    perror( RED "udhcpc" RESET);
}

static void configure_static(const char *iface, const char *addr, const char *mask, const char *gw)
{
    char cmd[256];

    snprintf(cmd, sizeof(cmd), "ip link set %s up", iface);
    system(cmd);

    snprintf(cmd, sizeof(cmd), "ip addr add %s/%s dev %s", addr, mask, iface);
    system(cmd);

    if (gw && strlen(gw) > 0)
    {
        snprintf(cmd, sizeof(cmd), "ip route add default via %s dev %s", gw, iface);
        system(cmd);
    }

    printf(GREEN "Configured static IP for %s: %s/%s gw %s" RESET "\n", iface, addr, mask, gw);
}

void network_init(void)
{
    pid_t pid;
    FILE *fp;
    int n;
    char line[256];
    char iface[32];
    char mode[16];
    char addr[64];
    char mask[64];
    char gw[64];

    /* Setup localhost */
    system("ifconfig lo 127.0.0.1 up");

    fp = fopen(NETCONF_PATH, "r");
    if (!fp)
    {
        perror(RED "fopen" RESET);
        return;
    }

    while (fgets(line, sizeof(line), fp))
    {
        if (line[0] == '#' || strlen(line) < 3)
            continue;

        memset(addr, 0, sizeof(addr));
        memset(mask, 0, sizeof(mask));
        memset(gw, 0, sizeof(gw));

        n = sscanf(line, "%31s %15s %63s %63s %63s",
                       iface, mode, addr, mask, gw);
        if (n < 2)
            continue;

        pid = fork(); /* Fork just in case. */
        if (pid == 0)
        {
            if (strcmp(mode, "dhcp") == 0)
                start_dhcp(iface);
            else if (strcmp(mode, "static") == 0 && n >= 5)
                configure_static(iface, addr, mask, gw);
            else
                printf(RED "Unknown mode %s for interface %s" RESET "\n", mode, iface);
            
            exit(0);
        }
    }

    fclose(fp);
}
