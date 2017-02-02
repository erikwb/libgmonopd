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

/* auction.c: handles auctions and auction I/O */

#include <stdlib.h>
#include "const.h"
#include "stub.h"

#include "player.h"
#include "game.h"
#include "estate.h"
#include "event.h"
#include "auction.h"

auction *new_auction(estate *e) 
{
	auction *a = (auction *)malloc(sizeof(auction));
	
	a->game = e->game;
	a->id = a->game->auctioncounter++;
	a->estate = e;
	a->lastbid = 0;
	a->highbid = 0;
	a->lastbidder = NULL;
	a->highbidder = NULL;
	a->event = NULL;
	a->going = 0;
	
	a->next = a->game->auctions;
	a->game->auctions = a;

	game_io_write(a->game, "<monopd><auctionupdate type=\"new\" auctionid=\"%d\" estateid=\"%d\"/></monopd>\n", a->id, e->id);
	
	return a;
}

void delete_auction(auction *a) 
{
	auction *a_tmp, *a_trailer;
	game *g = a->game;
	
	/* remove from list */
	for(a_trailer = NULL, a_tmp = g->auctions;
	    a_tmp != a;
	    a_trailer = a_tmp, a_tmp = a_tmp->next);
	if(a_trailer == NULL) /* first one */
		g->auctions = a->next;
	else
		a_trailer->next = a->next;

	free(a);
}

void auction_cancel_bid(auction *a) 
{
	a->highbidder = a->lastbidder;
	a->highbid = a->lastbid;
	
	if(a->highbidder == NULL)
		delete_event(a->event);

	auction_set_going(a, 0);

	game_io_write(a->game, "<monopd><auctionupdate type=\"edit\" auctionid=\"%d\" status=\"0\" message=\"Bid cancelled.\"/></monopd>\n", a->id);
}

void auction_complete(auction *a) 
{
	game *g = a->game;

	a->estate->owner = a->highbidder;
	player_pay(a->highbidder, NULL, a->highbid, TRUE);
	game_io_info(g, "%s received %s in an auction for $%d.",
		     a->highbidder->name, a->estate->name, a->highbid);
	game_io_write(g, "<monopd><auctionupdate type=\"completed\" auctionid=\"%d\"/></monopd>\n", a->id);
	game_io_estateupdate(g, a->estate);
	
	delete_auction(a);
	game_newturn(g);
}

void auction_set_going(auction *a, int going) 
{
	char str[BUFSIZE];

	a->going = going;

	if(going == 0)
		sprintf(str, "");
	if(going == 1)
		sprintf(str, "Going once...");
	if(going == 2)
		sprintf(str, "Going twice...");
	if(going == 3)
		sprintf(str, "SOLD!");
	
	game_io_write(a->game, "<monopd><auctionupdate type=\"edit\" auctionid=\"%d\" status=\"%d\" message=\"%s\"/></monopd>\n", a->id, going, str);
	
	if(a->event)
		a->event->launchtime = time(0) + (int)(AUCTIONTIMEOUT /
						       (going ? 3 : 1));
}

void auction_set_bid(auction *a, player *p, int bid) 
{
	if(!a->event) {
		a->event = new_event(p->sock->server, a, time(0) + AUCTIONTIMEOUT, 
				     event_auctiontimeout);
		a->event->permanent = 1;
	}

	a->lastbid = a->highbid;
	a->highbid = bid;

	a->lastbidder = a->highbidder;
	a->highbidder = p;

	game_io_write(p->game, "<monopd><auctionupdate type=\"edit\" auctionid=\"%d\" highbid=\"%d\" highbidder=\"%d\"/></monopd>\n", a->id, a->highbid, a->highbidder->id);
	
	auction_set_going(a, 0);
}
