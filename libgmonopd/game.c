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

/* game.c: manipulates the game state */

#include <time.h>
#include <stdarg.h>
#include <string.h>
#include <stdio.h>

#include "const.h"
#include "event.h"
#include "player.h"
#include "auction.h"
#include "estate.h"
#include "trade.h"
#include "card.h"
#include "game.h"
#include "libgmonopd.h"

game *new_game(player *p, char *theme)
{
	game *g;
	char buf[BUFSIZE];
	
	g = (game *)malloc(sizeof(game));
	g->server = p->sock->server;

	g->players = NULL;
	g->master = NULL;
	g->go = NULL;
	g->auctions = NULL;
	g->trades = NULL;

	g->numplayers = 0;
	g->status = GAME_CONFIG;
	/* configurable!! */
	g->jail_price = 50;
	g->houses = 32;
	g->hotels = 16;
	g->start_position = 0;
	g->cards_chance = (card *)NULL;
	g->cards_communitychest = (card *)NULL;

	sprintf(buf, "%s's game", p->name);
	g->description = (char *)malloc(strlen(buf) + 1);
	strcpy(g->description, buf);

	g->dice[0] = 0;
	g->dice[1] = 0;

	g->id = g->server->gamecounter++;

	g->playercounter = g->tradecounter = g->auctioncounter = 0;
	game_init_estates(g, theme);
	game_init_cards(g, CARD_CHANCE, theme);
	game_init_cards(g, CARD_COMMUNITYCHEST, theme);

	g->next = g->server->games;
	g->server->games = g;

	player_io_info(p, "Created a new game with id %d.", g->id);
	server_io_write(g->server, "<monopd><updategamelist type=\"add\"><game id=\"%d\" players=\"0\" description=\"%s\"/></updategamelist></monopd>\n", g->id, g->description);
	return g;
}

void delete_game(game *g) 
{
	game *g_trailer, *g_tmp;
	player *p_tmp;
	event *e_tmp;
	
	for(g_trailer = NULL, g_tmp = g->server->games;
	    g_tmp != g;
	    g_trailer = g_tmp, g_tmp = g_tmp->next);
	
	if(g_trailer == NULL) /* first one */
		g->server->games = g->next;
	else
		g_trailer->next = g->next;
	
	/* delete events */
	for(e_tmp = g->server->events; e_tmp; e_tmp = e_tmp->next)
		delete_event(e_tmp);
	
	/* boot players */
	for(p_tmp = g->players; p_tmp; p_tmp = p_tmp->next) {
		p_tmp->game = NULL;
		p_tmp->next = NULL;
		p_tmp->status = 0;
		p_tmp->commands = 0;
		p_tmp->location = 0;
		p_tmp->doublecount = 0;
		p_tmp->jailcount = 0;
		p_tmp->debt = NULL;
		p_tmp->cards = NULL;
		p_tmp->timeout = NULL;
		p_tmp->id = 0;
		
		player_io_write(p_tmp, "<monopd><gameupdate gameid=\"%d\" status=\"finished\"/></monopd>\n", g->id);
		player_io_commandlist(p_tmp);
	}

	free(g);
}

void game_start(game *g) 
{
	player *p;
	estate *e;
	int i;

	g->status = GAME_RUN;
	g->player_current = g->players;
	
	/* TODO: shuffle? */

	server_io_write(g->server, "<monopd><updategamelist type=\"del\"><game id=\"%d\"/></updategamelist></monopd>\n", g->id);
	game_io_write(g, "<monopd><gameupdate gameid=\"%d\" status=\"init\"/>", g->id);

	for(p = g->players; p; p = p->next) { /* for all players in the game */
		player_io_write(p, "<client playerid=\"%d\"/>", p->id);
		p->location = g->start_position;
		/* tell all players about him */
		game_io_write(g, "<playerupdate playerid=\"%d\" name=\"%s\" money=\"%d\" location=\"%d\" jailed=\"0\" chancejailcard=\"0\" chestjailcard=\"0\" directmove=\"1\"/>", p->id, p->name, p->money, p->location);
		/* tell him about all estates */
		for(i = 0; i < 40; i++) {
			e = g->estates[i];
			player_io_write(p, "<estateupdate estateid=\"%d\" type=\"%s\" name=\"%s\" owner=\"-1\" houses=\"%d\" houseprice=\"%d\" sellhouseprice=\"%d\" mortgageprice=\"%d\" unmortgageprice=\"%d\" mortgaged=\"%d\" groupid=\"%d\" can_be_owned=\"%d\" can_be_mortgaged=\"%d\" can_be_unmortgaged=\"%d\" can_buy_houses=\"%d\" can_sell_houses=\"%d\" price=\"%d\" rent0=\"%d\" rent1=\"%d\" rent2=\"%d\" rent3=\"%d\" rent4=\"%d\" rent5=\"%d\"/>", e->id, estate_type_to_string(e), e->name, e->houses, e->house_price, e->sell_house_price, e->mortgage_price, e->unmortgage_price, e->mortgaged, e->group, estate_can_be_owned(e), estate_can_be_mortgaged(e, p), estate_can_be_unmortgaged(e, p), estate_can_buy_houses(e, p), estate_can_sell_houses(e, p), e->price, e->rent[0], e->rent[1], e->rent[2], e->rent[3], e->rent[4], e->rent[5]);
		}
	}

	for(p = g->players; p; p = p->next)
		player_io_write(p, "<client playerid=\"%d\"/>", p->id);

	game_io_write(g, "<gameupdate gameid=\"%d\" status=\"started\"/></monopd>\n", g->id);

	game_newturn(g);
}

void game_newturn(game *g) 
{
	player *p = g->player_current;
	if(player_has_status(p, PLAYER_DOUBLES)) {
		player_remove_status(p, PLAYER_DOUBLES);
		player_give_status(p, PLAYER_CANROLL);
		player_io_commandlist(p);
		game_io_info(g, "%s rolled doubles, roll again.", p->name);
		return;
	}
	player_remove_status(p, PLAYER_CANROLL);
	player_remove_status(p, PLAYER_HASTURN);
	player_io_commandlist(p);

	g->player_current = g->player_current->next;
	if(g->player_current == NULL)
		g->player_current = g->players;
	
	p = g->player_current;
	player_give_status(p, PLAYER_HASTURN);

	if(player_has_status(p, PLAYER_JAILED))
		p->jailcount++;
	else player_give_status(p, PLAYER_CANROLL);
	
	game_io_write(g, "<monopd><newturn player=\"%d\"/></monopd>\n", p->id);
	player_io_commandlist(p);
}

void game_set_description(game *g, char *data) 
{
	free(g->description);
	g->description = escape_xml(data);

	strcpy(g->description, data);
	server_io_write(g->server, "<monopd><updategamelist type=\"edit\"><game id=\"%d\" description=\"%s\"/></updategamelist></monopd>\n", g->id, g->description);
}

void game_add_player(game *g, player *p) 
{
	player *p_tmp, *p_trailer;
	
	if(g->playercounter >= CONFIG_MAXPLAYERS) {
		player_io_error(p, "This game is full.");
		return;
	}
	if(g->status != GAME_CONFIG) {
		player_io_error(p, "This game has already started.");
		return;
	}

	for(p_trailer = NULL, p_tmp = g->players; 
	    p_tmp; 
	    p_trailer = p_tmp, p_tmp = p_tmp->next);
	if(p_trailer == NULL)
		g->players = p;
	else
		p_trailer->next = p;
	
	p->game = g;
	p->id = ++g->playercounter;

	player_io_write(p, "<monopd><joinedgame playerid=\"%d\"/></monopd>\n", g->id, p->id);
	player_io_info(p, "Joined game %d.", g->id);
	
	game_io_write(g, "<monopd><updateplayerlist type=\"add\">");
	for(p_tmp = g->players; p_tmp != p; p_tmp = p_tmp->next)
		player_io_write(p, "<player playerid=\"%d\" name=\"%s\" host=\"%s\"/>", p_tmp->id, p_tmp->name, p_tmp->host);

	game_io_write(g, "<player playerid=\"%d\" name=\"%s\" host=\"%s\"/></updateplayerlist></monopd>\n", p->id, p->name, p->host);

}

void game_set_master(game *g, player *p) 
{
	if(g->master)
		game_io_write(g, "<monopd><updateplayerlist type=\"edit\"><player playerid=\"%d\" master=\"0\"/></updateplayerlist></monopd>\n", g->master->id);
	g->master = p;
	game_io_write(g, "<monopd><updateplayerlist type=\"edit\"><player playerid=\"%d\" master=\"1\"/></updateplayerlist></monopd>\n", p->id);
}

void game_set_players_moving(game *g, player *p)
{
	player *p_tmp;
	for(p_tmp = g->players; p_tmp; p_tmp = p_tmp->next)
		player_give_status(p_tmp, PLAYER_MOVING);

	p->timeout = new_event(p->sock->server, p, time(0) + TOKENTIMEOUT, event_tokentimeout);
}

int game_has_players_moving(game *g)
{
	player *p;
	for(p = g->players; p; p = p->next)
		if(player_has_status(p, PLAYER_MOVING)) return 1;
	return 0;
}


auction *game_find_auction(game *g, int id) 
{
	auction *a;
	for(a = g->auctions; a; a = a->next)
		if(a->id == id) return a;
	return NULL;
}

int game_has_auction(game *g) 
{
	return (g->auctions ? 1 : 0);
}

player *game_find_player(game *g, int id) 
{
	player *p;
	for(p = g->players; p; p = p->next)
		if(p->id == id) return p;
	return NULL;
}

trade *game_find_trade(game *g, int id) 
{
	trade *t;
	for(t = g->trades; t; t = t->next)
		if(t->id == id) return t;
	return NULL;
}

estate *game_find_estate(game *g, int id) 
{
	return g->estates[id];
}

int game_has_debt(game *g) 
{
	player *p;
	for(p = g->players; p; p = p->next)
		if(p->debt) return 1;
	return 0;
}

void game_init_estates(game *g, char *theme) 
{
	FILE *f;
	char str[256], *buf;
	estate *e;
	int i = 0;
	int mortgageprice_set, unmortgageprice_set, sellhouseprice_set;
	sellhouseprice_set = mortgageprice_set = unmortgageprice_set = 0;
	
	strcpy(str, GMONOPD_DATA "/conf/");
	strcat(str, theme);
	strcat(str, "/estates.conf");

	f=fopen(str, "r");
	if (!f) return;
	
	fgets(str, sizeof(str), f);
	while(!feof(f)) {
		if (str[0]=='[') {
			buf=strtok(str, "]\n\0");
			e = new_estate(g, i++, buf+1);
			mortgageprice_set = 0;
			unmortgageprice_set = 0;
			sellhouseprice_set = 0;
		}
		else if (strstr(str, "=")) {
			buf = strtok(str, "=");
			if (!strcmp(buf, "type")) {	
				buf = strtok(NULL, "\n\0");
				if (!strcmp(buf, "street"))
					e->type = ESTATE_STREET;
				else if (!strcmp(buf, "rr"))
					e->type = ESTATE_RR;
				else if (!strcmp(buf, "airport"))
					e->type = ESTATE_AIRPORT;
				else if (!strcmp(buf, "utility"))
					e->type = ESTATE_UTILITY;
				else if (!strcmp(buf, "cc"))
					e->type = ESTATE_COMMUNITYCHEST;
				else if (!strcmp(buf, "chance"))
					e->type = ESTATE_CHANCE;
				else if (!strcmp(buf, "freeparking"))
					e->type = ESTATE_FREEPARKING;
				else if (!strcmp(buf, "tojail"))
					e->type = ESTATE_TOJAIL;
				else if (!strcmp(buf, "tax"))
					e->type = ESTATE_TAX;
				else if (!strcmp(buf, "go")) {
					e->type = ESTATE_GO;
					g->go = e;
				} else if (!strcmp(buf, "jail")) {
					g->jail_position = e->id;
					e->type = ESTATE_JAIL;
				} else
					e->type = ESTATE_OTHER;
			}
			else if (!strcmp(buf, "group"))
				e->group = atoi(strtok(NULL, "\n\0"));
			else if (!strcmp(buf, "price"))
				e->price = atoi(strtok(NULL, "\n\0"));
			else if (!strcmp(buf, "mortgageprice")) 
			{
				e->mortgage_price = atoi(strtok(NULL, "\n\0"));
				mortgageprice_set = 1;
			}
			else if (!strcmp(buf, "unmortgageprice")) 
			{
				e->unmortgage_price = atoi(strtok(NULL, 
								  "\n\0"));
				unmortgageprice_set = 1;
			}
			else if (!strcmp(buf, "houseprice"))
				e->house_price = atoi(strtok(NULL, "\n\0"));
			else if (!strcmp(buf, "sellhouseprice")) 
			{
				e->sell_house_price = atoi(strtok(NULL,
								  "\n\0"));
				sellhouseprice_set = 1;
			}
			else if (!strcmp(buf, "tax"))
				e->tax = atoi(strtok(NULL, "\n\0"));
			else if (!strcmp(buf, "taxpercentage"))
				e->tax_percentage = atoi(strtok(NULL, "\n\0"));
			else if (!strcmp(buf, "pay_location"))
				e->pay_location = atoi(strtok(NULL, "\n\0"));
			else if (!strcmp(buf, "rent0"))
				e->rent[0] = atoi(strtok(NULL, "\n\0"));
			else if (!strcmp(buf, "rent1"))
				e->rent[1] = atoi(strtok(NULL, "\n\0"));
			else if (!strcmp(buf, "rent2"))
				e->rent[2] = atoi(strtok(NULL, "\n\0"));
			else if (!strcmp(buf, "rent3"))
				e->rent[3] = atoi(strtok(NULL, "\n\0"));
			else if (!strcmp(buf, "rent4"))
				e->rent[4] = atoi(strtok(NULL, "\n\0"));
			else if (!strcmp(buf, "rent5"))
				e->rent[5] = atoi(strtok(NULL, "\n\0"));
			else if (!strcmp(buf, "go_money"))
				e->go_money = atoi(strtok(NULL, "\n\0"));
		}
		fgets(str, sizeof(str), f);
	
		/* defaults */
		if(!mortgageprice_set)
			e->mortgage_price = (int)(e->price/2);
		if(!unmortgageprice_set)
			e->unmortgage_price = (int)(e->mortgage_price*1.1);
		if(!sellhouseprice_set)
			e->sell_house_price = (int)(e->house_price / 2);
	}
	fclose(f);
}

void game_init_cards(game *g, int type, char *theme)
{
	FILE *f;
	char str[1024], *buf;
	card *c;

	buf = (char *)malloc(1024);
	strcpy(buf, GMONOPD_DATA "/conf/");
	strcat(buf, theme);

	if (type == CARD_CHANCE) 
		strcat(buf, "/chance_cards.conf");
	else /* type == CARD_COMMUNITYCHEST */
		strcat(buf, "/cc_cards.conf");

	printf("%s", buf);

	f = fopen(buf, "r");
	free(buf);
	if (!f) return;

	fgets(str, sizeof(str), f);
	while(!feof(f)) {
		if(str[0]=='[') {
			buf=strtok(str, "]\n\0");
			c = new_card(g, type, buf+1);
		} else if(strstr(str, "=")) {
			buf = strtok(str, "=");
			if(!strcmp(buf, "pay")) {
				c->pay = atoi(strtok(NULL, "\n\0"));
				card_give_action(c, CARD_PAY);
			} else if(!strcmp(buf, "payhouse")) {
				c->pay_house = atoi(strtok(NULL, "\n\0"));
				card_give_action(c, CARD_PAY_HOUSE);
			} else if(!strcmp(buf, "payhotel")) {
				c->pay_hotel = atoi(strtok(NULL, "\n\0"));
				card_give_action(c, CARD_PAY_HOTEL);
			} else if(!strcmp(buf, "payeach")) {
				c->pay_each = atoi(strtok(NULL, "\n\0"));
				card_give_action(c, CARD_PAY_EACH);
			} else if(!strcmp(buf, "tojail")) {
				card_give_action(c, CARD_TOJAIL);
			} else if(!strcmp(buf, "jailcard")) {
				card_give_action(c, CARD_JAILCARD);
			} else if(!strcmp(buf, "advance")) {
				c->distance = atoi(strtok(NULL, "\n\0"));
				card_give_action(c, CARD_ADVANCE);
			} else if (!strcmp(buf, "advanceto")) {
				c->location = atoi(strtok(NULL, "\n\0"));
				card_give_action(c, CARD_ADVANCE_TO);
			} else if(!strcmp(buf, "advancetype")){
				c->advancetype = 
					estate_type_to_int(strtok(NULL,
								  "\n\0"));
				card_give_action(c, CARD_ADVANCE_NEXT);
			} else if (!strcmp(buf, "pay_location"))
				c->pay_location = atoi(strtok(NULL, "\n\0"));
		}
		fgets(str, sizeof(str), f);
	}
	fclose(f);
}

void game_io_write(game *g, char *data, ...)
{
	va_list arg;
	player *p;
	char buf[BUFSIZE];
	
	va_start(arg, data);
	vsprintf(buf, data, arg);
	va_end(arg);
	
	for(p = g->players; p; p = p->next)
		player_io_write(p, buf);
}

void game_io_commandlist(game *g) 
{
	player *p;
	for(p = g->players; p; p = p->next)
		player_io_commandlist(p);
}

void game_io_estateupdate(game *g, estate *e)
{
	int i;
	player *p;
	estate *e_tmp = g->estates[0];
	
	for(p = g->players; p; p = p->next)
		for(i = 0; i < 40; i++) {
			e_tmp = g->estates[i];
			
			if((g->estates[i]->group == e->group) &&
			   (g->estates[i]->owner))
				player_io_write(p, "<monopd><estateupdate estateid=\"%d\" owner=\"%d\" can_be_mortgaged=\"%d\" can_be_unmortgaged=\"%d\" can_buy_houses=\"%d\" can_sell_houses=\"%d\" mortgaged=\"%d\" houses=\"%d\"/></monopd>\n", e_tmp->id, e_tmp->owner->id, estate_can_be_mortgaged(e_tmp, p), estate_can_be_unmortgaged(e_tmp, p), estate_can_buy_houses(e_tmp, p), estate_can_sell_houses(e_tmp, p), e_tmp->mortgaged, e_tmp->houses);
		}
}

void game_io_chat(player *p, char *data)
{
	char *temp = escape_xml(data);
	game_io_write(p->game, "<monopd><msg type=\"chat\" playerid=\"%d\" author=\"%s\" value=\"%s\"/></monopd>\n", p->id, p->name, temp);
	free(temp);
}


void game_io_info(game *g, char *data, ...)
{
	va_list arg;
	player *p;
	char buf[BUFSIZE];
	
	va_start(arg, data);
	vsprintf(buf, data, arg);
	va_end(arg);
	
	for(p = g->players; p; p = p->next)
		player_io_info(p, buf);
}

void game_io_error(game *g, char *data, ...)
{
	va_list arg;
	player *p;
	char buf[BUFSIZE];
	
	va_start(arg, data);
	vsprintf(buf, data, arg);
	va_end(arg);
	
	for(p = g->players; p; p = p->next)
		player_io_error(p, buf);
}

char *escape_xml(char *str) 
{
	char buf[1024];
	char *retval;
	int i, j = 0;
	buf[0] = '\0';
	
	for(i = 0; i < strlen(str); i++) {
		if(str[i] == '&') {
			strcat(buf, "&amp;");
			j += 5;
		} else if(str[i] == '<') {
			strcat(buf, "&lt;");
			j += 4;
		} else if(str[i] == '>') {
			strcat(buf, "&gt;");
			j += 4;
		} else if(str[i] == '"') {
			strcat(buf, "&quot;");
			j += 6; 
		}
		else {
			buf[j] = str[i];
			buf[++j] = '\0';
		}
	}

	retval = (char *)malloc(strlen(buf)+1);
	strcpy(retval, buf);
	return retval;
}
