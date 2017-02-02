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

/* card.c: allocates and manipulates cards */

#include "stub.h"
#include "game.h"
#include "card.h"
#include "player.h"

card *new_card(game *g, int type, char *text) 
{
	card *c = (card *)malloc(sizeof(card));
	
	c->pay = 0;
	c->pay_house = 0;
	c->pay_hotel = 0;
	c->pay_each = 0;
	c->pay_location = 0;
	c->distance = 0;
	c->location = -1;
	c->advancetype = 0;
	c->type = type;
	
	c->actions = 0;
	
	c->game = g;
	c->title = (char *)malloc(strlen(text)+1);
	c->description = (char *)malloc(strlen(text)+1);
	strcpy(c->title, text);
	strcpy(c->description, text);
	
	if(type == CARD_CHANCE) {
		c->next = g->cards_chance;
		g->cards_chance = c;
	} else { /* type == CARD_COMMUNITYCHEST */
		c->next = g->cards_communitychest;
		g->cards_communitychest = c;
	}
		
	return c;
}

int card_has_action(card *c, int actionbit) 
{
	return c->actions & actionbit;
}

void card_give_action(card *c, int actionbit) 
{
	c->actions |= actionbit;
}

void card_remove_action(card *c, int actionbit) 
{
	if(card_has_action(c, actionbit))
		c->actions ^= actionbit;
}

void card_do_action(card *c, player *p) 
{
	card *c_tmp, *c_trailer;

	if(card_has_action(c, CARD_JAILCARD)) {
		player_give_card(p, c);
		return;
	}
	if(card_has_action(c, CARD_TOJAIL))
		player_tojail(p);
	if(card_has_action(c, CARD_PAY))
		if(c->pay > 0)
			player_pay_to_location(p, p->game->estates[
						       c->pay_location],
					       c->pay);
		else player_pay(p, NULL, c->pay, FALSE);
	if(card_has_action(c, CARD_PAY_EACH))
		player_pay_each(p, c->pay_each);
	if(card_has_action(c, CARD_PAY_HOUSE))
		player_pay_per_house(p, c->pay_house);
	if(card_has_action(c, CARD_PAY_HOTEL))
		player_pay_per_hotel(p, c->pay_hotel);
	if(card_has_action(c, CARD_ADVANCE))
		player_advance(p, c->distance);
	if(card_has_action(c, CARD_ADVANCE_TO))
		player_advance_to_with_go(p, c->location);
	if(card_has_action(c, CARD_ADVANCE_NEXT))
		player_advance_next(p, c->advancetype);

	/* put the card back in the deck */
	c_tmp = (c->type == CARD_CHANCE) ?
		(p->game->cards_chance) :
		(p->game->cards_communitychest);
	for(c_trailer = NULL;
	    c_tmp;
	    c_trailer = c_tmp, c_tmp = c_tmp->next);
	c_trailer->next = c;
	c->next = NULL;
}
