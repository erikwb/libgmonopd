#ifndef NETWORK_H
#define NETWORK_H

#define MAXLINE 1024
#define LISTENQ 1024

#define SOCK_NEW  1
#define SOCK_OK   0
#define SOCK_BYE  -1
#define SOCK_GONE -2

#include "const.h"

struct player;
struct gmonopd;

typedef struct sock 
{
	int fd;
	int status;
	int port;
	int is_listenport;
	char *data;
	char ip[16];
	char fqdn[BUFSIZE];
	
	struct player *player;
	struct gmonopd *server;

	struct sock *next;
} sock;

extern int tcp_listen(struct gmonopd *, int);
extern void do_select(struct gmonopd *);
extern void server_io_write(struct gmonopd *, char *, ...);
extern void sock_io_write(sock *, char *, ...);
extern void sock_handler(sock *, char *);

#endif
