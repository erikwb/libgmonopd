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

/* event.c: handles creation of events */

#include <stdlib.h>
#include "stub.h"
#include "game.h"
#include "player.h"
#include "auction.h"
#include "event.h"
#include "libgmonopd.h"

/* external function bodies */
event *new_event(gmonopd *server, void *parent, int time, void (*func)(event *))
{
	event *ev = (event *)malloc(sizeof(event));
	ev->launchtime = time;
	ev->action = func;
	ev->parent = parent;
	ev->server = server;
	ev->next = server->events;
	server->events = ev;
	
	return ev;
}

void delete_event(event *ev) 
{
	event *ev_tmp, *ev_trailer;
	
	for(ev_trailer = NULL, ev_tmp = ev->server->events;
	    ev_tmp != ev;
	    ev_trailer = ev_tmp, ev_tmp = ev_tmp->next);
	
	if(ev_trailer == NULL) /* first one */
		ev->server->events = ev_tmp->next;
	else
		ev_trailer->next = ev_tmp->next;
	
	free(ev);
}

void event_tokentimeout(event *ev) 
{
	player *p_tmp;
	player *p = (player *)ev->parent;
	game *g = p->game;

	for(p_tmp = g->players; p_tmp; p_tmp = p_tmp->next)
		player_remove_status(p_tmp, PLAYER_MOVING);
	
	player_land(p);
}

void event_auctiontimeout(event *ev)
{
	auction *a = (auction *)ev->parent;
	auction_set_going(a, ++a->going);

	if(a->going >= 3) {
		auction_complete(a);
		ev->permanent = 0;
	}
}

