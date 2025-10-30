#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>

int main()
{
    system("echo daemon started! >> /tmp/daemon.log");
    while (1)
    {
        system("echo daemon running! >> /tmp/daemon.log");
        sleep(60);
    }
    return 0;
}