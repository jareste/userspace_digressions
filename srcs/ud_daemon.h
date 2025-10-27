#ifndef UD_DAEMON_H
#define UD_DAEMON_H

void daemons_load(const char *config_path);
int daemon_status(const char *name);
int daemon_stop(const char *name);
int daemon_start(const char *name);
void daemon_init(void);
void monitor_daemons(void);

void daemons_reload(void);

#endif /* UD_DAEMON_H */
