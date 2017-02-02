/*
 * gmonopd - a server for a real estate boardgame
 * Copyright (C) 2001 Erik Bourget and Rob Kaper
 * 
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License
 * as published by the Free Software Foundation; either version 2
 * of the License, or (at your option) any later version.
 * 
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 * 
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA  02111-1307, USA.
 */

/* network.c: sets up connections and calls input handler */

#include <stdio.h>
#include <stdlib.h>
#include <sys/socket.h>
#include <sys/time.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <unistd.h>
#include <fcntl.h>
#include <netdb.h>
#include <stdarg.h>
#include <malloc.h>
#include <string.h>
#include <errno.h>

#include "player.h"
#include "network.h"
#include "libgmonopd.h"
#include "input.h"

extern int errno;

/* filewide globals */

/* local function prototypes */
static void tcp_read(gmonopd *server);
static char *get_line_sock(sock *s);
static sock *new_sock(int fd, gmonopd *server);
static void delete_sock(sock *s);
static int data_on_sock(sock *s);
static void accept_sock(int fd, sock *s);
static void check_new_connection(int fd, gmonopd *server);

/* external function bodies */
int tcp_listen(gmonopd *server, int port)
{
	struct sockaddr_in servaddr;
	struct hostent *host;
	int reuse = 1;
	int flags;
	sock *s;
	char *ip;
	
	ip = (char *)malloc(strlen("0.0.0.0")+1);
	strcpy(ip, "0.0.0.0");
	server->listen_fd = socket(AF_INET, SOCK_STREAM, 0);

	memset(&servaddr, 0, sizeof(servaddr));
	servaddr.sin_family = AF_INET;
	inet_pton(AF_INET, ip, &servaddr.sin_addr);
	servaddr.sin_port = htons(port);

	/* get the fqdn */
	if((host = gethostbyaddr((char *)&servaddr.sin_addr, 
				 sizeof(servaddr.sin_addr), 
				 servaddr.sin_family)) == NULL)
		server->my_fqdn = ip;
	else
		server->my_fqdn = host->h_name;
	
	if(setsockopt(server->listen_fd, SOL_SOCKET, SO_REUSEADDR, 
		      &reuse, sizeof(reuse)) == -1) { /* error */
		printf("Error in setsockopt(): %s\n", strerror(errno));
		close(server->listen_fd);
		return 0;
	}
	
	if((bind(server->listen_fd, (struct sockaddr *) &servaddr, 
		 sizeof(servaddr))) == -1) {
		/* error */
		printf("Error in bind(): %s\n", strerror(errno));
		close(server->listen_fd);
		return 0;
	}
	
	if(listen(server->listen_fd, LISTENQ) == -1) { /* error */
		printf("Error in listen(): %s\n", strerror(errno));
		close(server->listen_fd);
		return 0;
	}
	
	/* get current socket flags */
	if((flags = fcntl(server->listen_fd, F_GETFL)) == -1) {
		printf("Error in F_GETFL: %s\n", strerror(errno));
		exit(1);
	}
	
	/* set socket to non-blocking */
	flags |= O_NDELAY;
	if(fcntl(server->listen_fd, F_SETFL, flags) == -1) {
		printf("Error in F_SETFL: %s\n", strerror(errno));
		exit(1);
	}

	return 1;
}

void do_select(gmonopd *server)
{
	sock *s_tmp, *s_trailer, *l_tmp;
	struct timeval tv;
	char *buf;
	int selectval;
	int highest = 0;
	fd_set fdset;

	/* check to see if it's time to kill some socks */
	for(s_trailer = NULL, s_tmp = server->socks; 
	    s_tmp != NULL; 
	    s_trailer = s_tmp, s_tmp = s_tmp->next) {
		if(s_tmp->status == SOCK_BYE) {
			sock_handler(s_tmp, "");
			close(s_tmp->fd);
			if(s_trailer == NULL) server->socks = s_trailer->next;
			else s_trailer->next = s_tmp->next;
		}
	}

	/* do the select */
	FD_ZERO(&fdset);
	highest = server->listen_fd;
	FD_SET(server->listen_fd, &fdset);
	for(s_tmp = server->socks; s_tmp; s_tmp = s_tmp->next) {
		if(s_tmp->fd > 0) {
			FD_SET(s_tmp->fd, &fdset);
			highest = ((s_tmp->fd > highest) ?
				   s_tmp->fd :
				   highest);
		}
	}

	tv.tv_sec = 0;
	tv.tv_usec = 1;
	selectval = select(highest+1, &fdset, NULL, NULL, &tv);

	if(selectval > 0) { /* data to be read */
		check_new_connection(server->listen_fd, server);
		tcp_read(server);
	} else if(selectval == -1) {
		printf("Error in main select() call.\n");
		exit(1);
	}
	
	/* process data that tcp_read grabbed from the socket */
	for(s_tmp = server->socks; s_tmp; s_tmp = s_tmp->next)
		if((buf = get_line_sock(s_tmp))) {
			if(strlen(buf)) sock_handler(s_tmp, buf);
			free(buf);
		}
}

void server_io_write(gmonopd *server, char *data, ...)
{
	sock *s;
	va_list arg;
	char buf[BUFSIZE];
	
	va_start(arg, data);
	vsprintf(buf, data, arg);
	va_end(arg);
	
	for(s = server->socks; s; s = s->next)
		sock_io_write(s, buf);
}

void sock_io_write(sock *s, char *data, ...) 
{
	va_list arg;
	char buf[BUFSIZE];

	va_start(arg, data);
	vsprintf(buf, data, arg);
	va_end(arg);
	
	write(s->fd, buf, strlen(buf));
}

/* local function bodies */
void check_new_connection(int fd, gmonopd *server) 
{
	sock *newsock = new_sock(fd, server);
	if(newsock)
		sock_handler(newsock, "");
}

void tcp_read(gmonopd *server) 
{
	int data = 0;
	int n = 0;
	char msg[MAXLINE+1];
	sock *s_tmp, *s_trailer = NULL;
	
	for(s_tmp = server->socks; s_tmp;
	    s_trailer = s_tmp, s_tmp = s_tmp->next) {
		if(s_tmp->status == SOCK_GONE)
			delete_sock(s_tmp);
		else if(s_tmp->status == SOCK_OK &&
			s_tmp->fd > 0 &&
			data_on_sock(s_tmp)) {
			/* stuff to read */
			n = read(s_tmp->fd, msg, MAXLINE);
			if(n <= 0) {
				s_tmp->status = SOCK_GONE;
				sock_handler(s_tmp, "");
				delete_sock(s_tmp);
				break;
			} 
			msg[n] = 0;
			if(s_tmp->data[0] == '\0')
				strcpy(s_tmp->data, msg);
			else
				strcat(s_tmp->data, msg);
		}
	}
}

/* Warning: potential memory leak. */	
char *get_line_sock(sock *s) 
{
	char *buf;
	char buf2[MAXLINE+1];
	int i;

	if(!strlen(s->data)) return NULL;
	buf = (char *)malloc(MAXLINE+1);

	for(i = 0; 
	    s->data[i] != '\0' && (s->data[i] != '\r');
	    i++)
		buf[i] = s->data[i];
	buf[i] = '\0';

	/* trim it from the buffer */
	if(s->data[i] == '\0') s->data[0] = '\0';
	
	else { /* s->data[i] is "\r\n" */
		strcpy(buf2, s->data+i+2);
		strcpy(s->data, buf2);
	}
	
	return buf;
}

/* spawns a new sock by accepting a connect request from fd */
sock *new_sock(int fd, gmonopd *server) 
{
	sock *s = (sock *)malloc(sizeof(sock));
	s->data = (char *)malloc(BUFSIZE);
	s->status = SOCK_NEW;
	s->is_listenport = 0;
	s->next = NULL;
	s->server = server;

	accept_sock(fd, s);
	if(s->fd ==  -1) {
		free(s->data);
		free(s);
		return NULL;
	}
	
	s->player = new_player(s);
	/* linked list management */
	s->next = server->socks;
	server->socks = s;

	return s;
}

void delete_sock(sock *s) 
{
	sock *s_step, *s_trailer;
	
	for(s_trailer = NULL, s_step = s->server->socks; 
	    s_step; 
	    s_trailer = s_step, s_step = s_step->next) {
		if(s_step = s) {
                        if(s_trailer) {
                                s_trailer->next = s_step->next;
                        } else {
                                s->server->socks = s_step->next;
                        }
			break;
		}
	}
	free(s->data);
	free(s);
}

/* boolean test function */
int data_on_sock(sock *s) 
{
	fd_set rfds;
	struct timeval tv;
	int response;
	
	FD_ZERO(&rfds);
	FD_SET(s->fd, &rfds);
	tv.tv_sec = 0;
	tv.tv_usec = 1;
	response = select(s->fd+1, &rfds, NULL, NULL, &tv);
	
	if(response == -1) {
		printf("Error in select(): %s\n", strerror(errno));
		exit(1);
	}

	return FD_ISSET(s->fd, &rfds);
}

/* fills a sock structure with information about a new connection */
void accept_sock(int fd, sock *s) 
{
	int len;
	struct sockaddr_in clientaddr;
	struct hostent *host;
	
	len = sizeof(clientaddr);
	s->fd = accept(fd, (struct sockaddr *)&clientaddr, 
		       (socklen_t *)&len);
	
	if(s->fd == -1) return; /* no point gethostbyaddr()ing it, eh */
	strcpy(s->ip, inet_ntoa(clientaddr.sin_addr));
	if((host = gethostbyaddr((char *)&clientaddr.sin_addr,
				 sizeof(clientaddr.sin_addr),
				 AF_INET)) == NULL)
		strcpy(s->fqdn, s->ip);
	else strcpy(s->fqdn, host->h_name);
}

void sock_handler(sock *s, char *data) 
{
	switch(s->status) {
	case SOCK_NEW:
		s->status = SOCK_OK;
		player_io_intro(s->player);
		break;


	case SOCK_BYE:
		delete_sock(s);
	case SOCK_GONE:
		if(s->player) delete_player(s->player);

		break;

	case SOCK_OK:
		if(s->player != NULL) parse_input(s->player, data);
		break;
	}
}
