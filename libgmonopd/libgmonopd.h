#ifndef GMONOPD_H
#define GMONOPD_H

#include <pthread.h>

struct event;
struct game;
struct sock;

typedef struct gmonopd 
{
	struct event *events;
	struct game *games;
	struct sock *socks;
	pthread_t thread;

	int gamecounter;
	int listen_fd;
	char *my_fqdn;
} gmonopd;

extern gmonopd *gmonopd_init(int port);
extern void gmonopd_iterate(gmonopd *server);
extern int gmonopd_run(gmonopd *server);
extern int gmonopd_kill(gmonopd *server);

#endif
