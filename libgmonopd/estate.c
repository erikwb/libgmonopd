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

/* estate.c: allocates and manipulates estates */

#include "stub.h"
#include "game.h"
#include "player.h"
#include "card.h"
#include "estate.h"

estate *new_estate(game *g, int id, char *name) 
{
	estate *e = (estate *)malloc(sizeof(estate));
	e->name = (char *)malloc(strlen(name)+1);
	strcpy(e->name, name);
	e->game = g;
	
	e->group = 0;
	e->id = id;
	e->mortgaged = 0;
	e->houses = 0;
	e->price = 0;
	e->house_price = 0;
	e->sell_house_price = 0;
	e->mortgage_price = 0;
	e->unmortgage_price = 0;
	e->tax = 0;
	e->tax_percentage = 0;
	e->pay_location = 0;
	e->fp_money = 0;
	e->go_money = 0;
	e->rent[0] = 0;
	e->rent[1] = 0;
	e->rent[2] = 0;
	e->rent[3] = 0;
	e->rent[4] = 0;
	e->rent[5] = 0;
	
	e->type = ESTATE_OTHER;
	e->owner = NULL;

	g->estates[id] = e;

	return e;
}

int estate_get_rent(estate *e) 
{
	int i, mult = 0;
	
	switch(e->type) {
	case ESTATE_STREET:
		return e->rent[e->houses];
	case ESTATE_RR:
		for(i = 0; i < 40; i++)
			if((e->game->estates[i]->type == ESTATE_RR) &&
			   (e->game->estates[i]->owner == e->owner))
				mult++;
		return e->rent[mult];
	case ESTATE_UTILITY:
		for(i = 0; i < 40; i++)
			if((e->game->estates[i]->type == ESTATE_UTILITY) &&
			   (e->game->estates[i]->owner == e->owner))
				mult++;
		return e->rent[mult] * (e->game->dice[0] +
					e->game->dice[1]);
	default:
		return 0;
	}
}
			
int estate_group_has_buildings(estate *e)
{
	int i;
	for(i = 0; i < 40; i++)
		if(e->game->estates[i]->group == e->group && 
		   e->game->estates[i]->houses) return 1;
	return 0;
}

int estate_can_be_mortgaged(estate *e, player *p) 
{
	return (p == e->owner &&
		!e->mortgaged &&
		!estate_group_has_buildings(e));
}

int estate_can_be_unmortgaged(estate *e, player *p) 
{
	return (p == e->owner && e->mortgaged);
}

int estate_can_buy_houses(estate *e, player *p) 
{
	return (p == e->owner &&
		estate_group_is_monopoly(e) &&
		!estate_group_has_mortgages(e) &&
		e->houses < estate_max_houses(e));
}

int estate_can_sell_houses(estate *e, player *p) 
{
	return (p == e->owner &&
		estate_group_is_monopoly(e) &&
		e->houses > estate_min_houses(e));
}

int estate_can_be_owned(estate *e) 
{
	return ((e->type == ESTATE_STREET) ||
		(e->type == ESTATE_RR) ||
		(e->type == ESTATE_AIRPORT) ||
		(e->type == ESTATE_UTILITY));
}

int estate_type_to_int(char *data) 
{
	if(!strcmp(data, "street"))
		return ESTATE_STREET;
	if(!strcmp(data, "railroad"))
		return ESTATE_RR;
	if(!strcmp(data, "airport"))
		return ESTATE_AIRPORT;
	if(!strcmp(data, "utility"))
		return ESTATE_UTILITY;
	if(!strcmp(data, "communitychest"))
		return ESTATE_COMMUNITYCHEST;
	if(!strcmp(data, "chance"))
		return ESTATE_CHANCE;
	if(!strcmp(data, "freeparking"))
		return ESTATE_FREEPARKING;
	if(!strcmp(data, "tojail"))
		return ESTATE_TOJAIL;
	if(!strcmp(data, "tax"))
		return ESTATE_TAX;
	if(!strcmp(data, "jail"))
		return ESTATE_JAIL;
	if(!strcmp(data, "go"))
		return ESTATE_GO;
	return ESTATE_OTHER;
}
		
/* Warning: this is potentially a memory-leak function. */
char *estate_type_to_string(estate *e) 
{
	char *str = (char *)malloc(BUFSIZE);
	switch(e->type) {
	case ESTATE_STREET:
		strcpy(str, "street");
		break;
	case ESTATE_RR:
		strcpy(str, "railroad");
		break;
	case ESTATE_AIRPORT:
		strcpy(str, "airport");
		break;
	case ESTATE_UTILITY:
		strcpy(str, "utility");
		break;
	case ESTATE_COMMUNITYCHEST:
		strcpy(str, "communitychest");
		break;
	case ESTATE_CHANCE:
		strcpy(str, "chance");
		break;
	case ESTATE_FREEPARKING:
		strcpy(str, "freeparking");
		break;
	case ESTATE_TOJAIL:
		strcpy(str, "tojail");
		break;
	case ESTATE_TAX:
		strcpy(str, "tax");
		break;
	case ESTATE_JAIL:
		strcpy(str, "jail");
		break;
	case ESTATE_GO:
		strcpy(str, "go");
		break;
	case ESTATE_OTHER:
		strcpy(str, "other");
		break;
	}
	return str;
}

card *estate_get_card(estate *e, player *p)
{
	card *c;
	/* remove from deck */
	if(e->type == ESTATE_CHANCE) {
		c = e->game->cards_chance;
		e->game->cards_chance = c->next;
	} else {
		c = e->game->cards_communitychest;
		e->game->cards_communitychest = c->next;
	}
	
	game_io_write(p->game, "<monopd><display playerid=\"%d\" type=\"%s\" name=\"%s\" description=\"%s\"/></monopd>\n", p->id, estate_type_to_string(e), c->title, c->description);
	
	return c;
}
	
int estate_max_houses(estate *e) 
{
	estate *e_tmp;
	int i, maxhouses = 5;
	
	for(i = 0; i < 40; i++) {
		e_tmp = e->game->estates[i];
		if((e_tmp->group == e->group) && (e_tmp->houses < maxhouses))
			maxhouses = e_tmp->houses + 1;
	}
	return maxhouses;
}

int estate_min_houses(estate *e)
{
	estate *e_tmp;
	int i, minhouses = 0;

	for(i = 0; i < 40; i++) {
		e_tmp = e->game->estates[i];
		if((e_tmp->group == e->group) && (e_tmp->houses > minhouses))
			minhouses = e_tmp->houses - 1;
	}
	return minhouses;
}

int estate_group_is_monopoly(estate *e) 
{
	estate *e_tmp;
	int i, sameowner = 0;

	for(i = 0; i < 40; i++) {
		e_tmp = e->game->estates[i];
		if((e_tmp->group == e->group) && (e_tmp->owner == e->owner))
			sameowner++;
	}
	return (sameowner == estate_group_size(e));
}

int estate_group_size(estate *e) 
{
	int i, groupsize = 0;

	for(i = 0; i < 40; i++)
		if(e->game->estates[i]->group == e->group) groupsize++;
	return groupsize;
}

int estate_group_has_mortgages(estate *e) 
{
	int i;
	for(i = 0; i < 40; i++)	
		if(e->game->estates[i]->group == e->group &&
		   e->game->estates[i]->mortgaged) return 1;
	return 0;
}
