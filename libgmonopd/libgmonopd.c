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

#include <stdlib.h>
#include <pthread.h>
#include "libgmonopd.h"
#include "event.h"

static void gmonopd_run_internal(void *sv);

gmonopd *gmonopd_init(int port) 
{
	gmonopd *server = (gmonopd *)malloc(sizeof(gmonopd));

	server->events = NULL;
	server->games = NULL;
	server->socks = NULL;
	server->thread = 0;
	server->gamecounter = 0;
	
	server->listen_fd = 0;
	server->my_fqdn = NULL;

	if(!tcp_listen(server, port)) {
		free(server);
		server = NULL;
	}
	
	return server;
}

int gmonopd_run(gmonopd *server)
{
	pthread_attr_t attr;
	pthread_attr_init(&attr);
	
	return pthread_create(&server->thread, &attr,
			      (void *)&gmonopd_run_internal,
			      (void *)server);
}

int gmonopd_kill(gmonopd *server)
{
	int error1 = 0;
	int error2 = close(server->listen_fd);
	
	if(server->thread)
		error1 = pthread_cancel(server->thread);
	
	return error1 + error2;
}

void gmonopd_iterate(gmonopd *server) 
{
	event *ev_tmp = NULL;
	
	/* check for network events */
	do_select(server);

	/* check for internal events */
	for(ev_tmp = server->events; ev_tmp != NULL; 
	    ev_tmp = ev_tmp->next)
		if(time(0) >= ev_tmp->launchtime) {
			ev_tmp->action(ev_tmp);
			if(!ev_tmp->permanent) {
				delete_event(ev_tmp);
				break;
			}
		}
}
	
void gmonopd_run_internal(void *sv) 
{
	for(;;) gmonopd_iterate((gmonopd *)sv);
}

