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

/* input.c: takes action based on player input */

#include <stdlib.h>
#include <sys/time.h>

#include "stub.h"
#include "input.h"
#include "player.h"
#include "auction.h"
#include "debt.h"
#include "trade.h"
#include "estate.h"
#include "card.h"
#include "game.h"
#include "libgmonopd.h"

/* local function prototypes */
static void parse_command(player *, char *);

static void cmd_roll(player *);
static void cmd_tokenmoved(player *, char *);
static void cmd_auctionbid(player *, char *);

static void cmd_game_new(player *, char *);
static void cmd_game_list(player *);
static void cmd_game_desc(player *, char *);
static void cmd_game_start(player *);
static void cmd_game_join(player *, char *);

static void cmd_trade_new(player *, char *);
static void cmd_trade_estate(player *, char *);
static void cmd_trade_money(player *, char *);
static void cmd_trade_accept(player *, char *);
static void cmd_trade_reject(player *, char *);

static void cmd_house_buy(player *, char *);
static void cmd_house_sell(player *, char *);
static void cmd_mortgage(player *, char *);
static void cmd_unmortgage(player *, char *);

static void cmd_estate_buy(player *);
static void cmd_estate_auction(player *);

static void cmd_jail_roll(player *);
static void cmd_jail_pay(player *);
static void cmd_jail_card(player *);

static void cmd_declarebankruptcy(player *);
static void cmd_paydebt(player *);

/* external function bodies */
void parse_input(player *p, char *data) 
{
	switch(data[0]) {
	case '.':
		parse_command(p, data+1);
		break;
	default:
		if(p->game)
			game_io_chat(p, data);
		else
			player_io_error(p, "You are not within a game.");
		break;
	}
}

void parse_command(player *p, char *data) 
{
	player_set_commands(p);

	switch(data[0]) {
	case 'd': /* disconnect */
		delete_player(p);
		break;

	case 'n': /* set name */
		if(player_has_command(p, COMMAND_SETNAME))
			player_set_name(p, data+1);
		else player_io_error(p, "Command unavailable.");
		break;
	case 'g': /* game */
		switch(data[1]) {
		case 'n':
			if(player_has_command(p, COMMAND_GAME_NEW))
				cmd_game_new(p, "city");
			else player_io_error(p, "Command unavailable.");
			break;
		case 'l':
			if(player_has_command(p, COMMAND_GAME_LIST))
				cmd_game_list(p);
			else player_io_error(p, "Command unavailable.");
			break;
		case 'j':
			if(player_has_command(p, COMMAND_GAME_JOIN))
				cmd_game_join(p, data+2);
			else player_io_error(p, "Command unavailable.");
			break;
		case 's':
			if(player_has_command(p, COMMAND_GAME_START))
				cmd_game_start(p);
			else player_io_error(p, "Command unavailable.");
			break;
		case 'd':
			if(player_has_command(p, COMMAND_GAME_DESC))
				cmd_game_desc(p, data+2);
			else player_io_error(p, "Command unavailable.");
			break;
		default:
			player_io_error(p, "No such command.");
			break;
		}
		break;
	case 'T': /* trading */
		switch(data[1]) {
		case 'e':
			if(player_has_command(p, COMMAND_TRADE_ESTATE))
				cmd_trade_estate(p, data+2);
			else player_io_error(p, "Command unavailable.");
			break;
		case 'm':
			if(player_has_command(p, COMMAND_TRADE_MONEY))
				cmd_trade_money(p, data+2);
			else player_io_error(p, "Command unavailable.");
			break;
		case 'n':
			if(player_has_command(p, COMMAND_TRADE_NEW))
				cmd_trade_new(p, data+2);
			else player_io_error(p, "Command unavailable.");
			break;
		case 'a':
			if(player_has_command(p, COMMAND_TRADE_ACCEPT))
				cmd_trade_accept(p, data+2);
			else player_io_error(p, "Command unavailable.");
			break;
		case 'r':
			if(player_has_command(p, COMMAND_TRADE_REJECT))
				cmd_trade_reject(p, data+2);
			else player_io_error(p, "Command unavailable.");
			break;
		default:
			player_io_error(p, "No such command.");
			break;
		}
		break;
	case 't': /* confirm token move */
		if(player_has_command(p, COMMAND_TOKENMOVE))
			cmd_tokenmoved(p, data+1);
		else player_io_error(p, "Command unavailable.");
		break;
	case 'h': /* houses */
		switch(data[1]) {
		case 'b':
			if(player_has_command(p, COMMAND_HOUSE_BUY))
				cmd_house_buy(p, data+2);
			else player_io_error(p, "Command unavailable.");
			break;
		case 's':
			if(player_has_command(p, COMMAND_HOUSE_SELL))
				cmd_house_sell(p, data+2);
			else player_io_error(p, "Command unavailable.");
			break;
		default:
			player_io_error(p, "No such command.");
			break;
		}
		break;
	case 'm': /* mortgage */
		if(player_has_command(p, COMMAND_MORTGAGE))
			cmd_mortgage(p, data+1);
		else player_io_error(p, "Command unavailable.");
		break;
	case 'u': /* unmortgage */
		if(player_has_command(p, COMMAND_UNMORTGAGE))
			cmd_unmortgage(p, data+1);
		else player_io_error(p, "Command unavailable.");
		break;
	case 'a': /* auction */
		if(player_has_command(p, COMMAND_AUCTION_BID))
			cmd_auctionbid(p, data+1);
		else player_io_error(p, "Command unavailable.");
		break;
	case 'D': /* declare bankruptcy */
		if(player_has_command(p, COMMAND_DECLARE))
			cmd_declarebankruptcy(p);
		else player_io_error(p, "Command unavailable.");
		break;
	case 'p': /* pay off debt */
		if(player_has_command(p, COMMAND_PAYDEBT))
			cmd_paydebt(p);
		else player_io_error(p, "Command unavailable.");
		break;
	case 'e': /* estate buy/auction */
		switch(data[1]) {
		case 'b':
			if(player_has_command(p, COMMAND_ESTATE_BUY))
				cmd_estate_buy(p);
			else player_io_error(p, "Command unavailable.");
			break;
		case 'a':
			if(player_has_command(p, COMMAND_ESTATE_AUCTION))
				cmd_estate_auction(p);
			else player_io_error(p, "Command unavailable.");
			break;
		default:
			player_io_error(p, "No such command.");
			break;
		}
		break;
	case 'j': /* jail */
		switch(data[1]) {
		case 'c':
			if(player_has_command(p, COMMAND_JAIL_CARD))
				cmd_jail_card(p);
			else player_io_error(p, "Command unavailable.");
			break;
		case 'p':
			if(player_has_command(p, COMMAND_JAIL_PAY))
				cmd_jail_pay(p);
			else player_io_error(p, "Command unavailable.");
			break;
		case 'r':
			if(player_has_command(p, COMMAND_JAIL_ROLL))
				cmd_jail_roll(p);
			else player_io_error(p, "Command unavailable.");
			break;
		default:
			player_io_error(p, "No such command.");
			break;
		}
		break;
	case 'r':
		if(player_has_command(p, COMMAND_ROLL))
			cmd_roll(p);
		else player_io_error(p, "Command unavailable.");
		break;
	default:
		player_io_error(p, "No such command.");
		break;
	}
}

/* local function bodies */
void cmd_roll(player *p) 
{
	struct timeval tv;

	player_remove_status(p, PLAYER_CANROLL);
	player_remove_status(p, PLAYER_CANBUY);
	player_remove_status(p, PLAYER_DOUBLES);

	gettimeofday(&tv, NULL);
	srand(tv.tv_usec);
	p->game->dice[0] = 1 + (int) (6.0f * rand()/(RAND_MAX + 1.0));
	gettimeofday(&tv, NULL);
	srand(tv.tv_usec);
	p->game->dice[1] = 1 + (int) (6.0f * rand()/(RAND_MAX + 1.0));
	
	game_io_info(p->game, "%s rolled %d and %d.", p->name, 
		     p->game->dice[0], p->game->dice[1]);

	if(p->game->dice[0] == p->game->dice[1]) { /* doubles! */
		player_give_status(p, PLAYER_DOUBLES);
		p->doublecount++;
		if(p->doublecount == 3) {
			player_tojail(p);
			game_newturn(p->game);
			return;
		}
	} else p->doublecount = 0;
	
	player_advance(p, p->game->dice[0] + p->game->dice[1]);
	game_set_players_moving(p->game, p);
}

void cmd_tokenmoved(player *p, char *data) 
{
	player *p_turn = p->game->player_current;	
	int estate_id = atoi(data);
	
	if(estate_id == 0) {
		/* send go money message? */ 
	}
	
	if(!player_has_status(p, PLAYER_MOVING)) return;
	if(estate_id == p_turn->location)
		player_remove_status(p, PLAYER_MOVING);
	
	if(game_has_players_moving(p->game)) return;
	else player_land(p_turn);
}

void cmd_auctionbid(player *p, char *data) 
{
	int auction_id, bid, i;
	auction *a;
	char *str = (char *)malloc(20);

	for(i = 0; isdigit(data[i]); i++)
		str[i] = data[i];
	str[i] = '\0';

	auction_id = atoi(str);
	free(str);
	bid = atoi(data+i+1);
	
	if(!(a = game_find_auction(p->game, auction_id))) {
		player_io_error(p, "No such auctionid %d.", auction_id);
		return;
	}
	
	if(p->money < bid) {
		player_io_error(p, "You don't have $%d.", bid);
		return;
	}
	
	if(bid <= a->highbid) {
		player_io_error(p, "Minimum bid is %d.", a->highbid+1);
		return;
	}
	
	auction_set_bid(a, p, bid);
}

void cmd_game_new(player *p, char *data) 
{
	p->game = new_game(p, "city");
	game_add_player(p->game, p);
	game_set_master(p->game, p);
}

void cmd_game_list(player *p) 
{
	game *g, *games = p->sock->server->games;
	
	player_io_write(p, "<monopd><updategamelist type=\"full\">");
	for(g = games; g; g = g->next)
		player_io_write(p, "<game id=\"%d\" players=\"%d\" description=\"%s\"/>", g->id, g->numplayers, g->description);
	player_io_write(p, "</updategamelist></monopd>\n");
}

void cmd_game_desc(player *p, char *data)
{
	game_set_description(p->game, data);
}

void cmd_game_start(player *p) 
{
	if(p->game->status != GAME_CONFIG)
		player_io_error(p, "The game has already started.");
	else if(p->game->master != p)
		player_io_error(p, "You're not the game master!");
	else 
		game_start(p->game);
}

void cmd_game_join(player *p, char *data) 
{
	game *g, *games = p->sock->server->games;
	int game_id = atoi(data);

	for(g = games; g; g = g->next)
		if(g->id == game_id) {
			game_add_player(g, p);
			break;
		}
}

void cmd_trade_new(player *p, char *data)
{
	player *p_target;
	int player_id = atoi(data);
	if(!(p_target = game_find_player(p->game, player_id))) {
		player_io_error(p, "No such playerid: %d.", player_id);
		return;
	}
	new_trade(p, p_target);
}

void cmd_trade_estate(player *p, char *data) 
{
	int trade_id, estate_id;
	int i;
	char str[20];
	trade *t;
	estate *e;

	for(i = 0; isdigit(data[i]); i++)
		str[i] = data[i];
	str[i] = '\0';
	
	trade_id = atoi(str);
	estate_id = atoi(data+i+1);
	
	if(!(t = game_find_trade(p->game, trade_id))) {
		player_io_error(p, "No such tradeid %d.", trade_id);
		return;
	}
	
	if(!(e = game_find_estate(p->game, estate_id))) {
		player_io_error(p, "No such estateid %d.", estate_id);
		return;
	}
	
	if(estate_group_has_buildings(e)) {
		player_io_error(p, "Group contains buildings.");
		return;
	}

	trade_toggle_estate(t, e, p);
}

void cmd_trade_money(player *p, char *data) 
{
	trade *t;
	
	int trade_id, money, i;
	char str[20];
	
	for(i = 0; isdigit(data[i]); i++)
		str[i] = data[i];
	str[i] = '\0';
	
	trade_id = atoi(str);
	money = atoi(data+i+1);
	
	if(!(t = game_find_trade(p->game, trade_id))) {
		player_io_error(p, "No such tradeid %d.", trade_id);
		return;
	}
	
	if(p->money < money) {
		player_io_error(p, "You don't have $%d to offer.", money);
		return;
	}
	
	trade_money(t, p, money);
}

void cmd_trade_accept(player *p, char *data) 
{
	trade *t;
	int trade_id = atoi(data);

	if(!(t = game_find_trade(p->game, trade_id))) {
		player_io_error(p, "no such tradeid: %d.", trade_id);
		return;
	}
	
	if(p == t->to || p == t->from)
		trade_toggle_accept(t, p);
	if(t->to_accept && t->from_accept)
		trade_complete(t);
}

void cmd_trade_reject(player *p, char *data) 
{
	trade *t;
	int trade_id = atoi(data);
	if(!(t = game_find_trade(p->game, trade_id))) {
		player_io_error(p, "no such tradeid: %d.", trade_id);
		return;
	}
	
	if(p == t->to || p == t->from)
		trade_reject(t, p);
}

void cmd_house_buy(player *p, char *data) 
{
	estate *e = p->game->estates[atoi(data)];

	if(p->game->houses <= 0) {
		player_io_error(p, "There aren't any houses left!");
		return;
	}

	if(e->house_price > p->money) {
		player_io_error(p, "You don't have $%d.", e->house_price);
		return;
	}
	
	if(e->houses >= 5) {
		player_io_error(p, "This estate is full!");
		return;
	}

	if(!estate_can_buy_houses(e, p)) {
		player_io_error(p, "You cannot buy houses on this estate.");
		return;
	}

	game_io_info(p->game, "%s buys a house for '%s'.", p->name, e->name);
	player_pay(p, NULL, e->house_price, FALSE);
	e->houses++;
	game_io_estateupdate(p->game, e);
}
       
void cmd_house_sell(player *p, char *data) 
{
	estate *e = p->game->estates[atoi(data)];
	if(e->houses <= 0) {
		player_io_error(p, "This estate has no houses!");
		return;
	}

	if(!estate_can_sell_houses(e, p)) {
		player_io_error(p, "You cannot sell houses on this estate.");
		return;
	}
	
	game_io_info(p->game, "%s sells a house on '%s'.", p->name, e->name);
	player_pay(p, NULL, -(e->sell_house_price), FALSE);
	e->houses--;
	game_io_estateupdate(p->game, e);
}

void cmd_mortgage(player *p, char *data) 
{
	estate *e = p->game->estates[atoi(data)];
	
	if(!estate_can_be_mortgaged(e, p)) {
		player_io_error(p, "You cannot mortgage this estate.");
		return;
	}

	game_io_info(p->game, "%s mortgages '%s'.", p->name, e->name);
	player_pay(p, NULL, -(e->mortgage_price), FALSE);
	e->mortgaged = 1;
	game_io_estateupdate(p->game, e);
}

void cmd_unmortgage(player *p, char *data) 
{
	estate *e = p->game->estates[atoi(data)];
	
	if(!estate_can_be_unmortgaged(e, p)) {
		player_io_error(p, "You cannot unmortgage this estate.");
		return;
	}
	if(p->money < e->unmortgage_price) {
		player_io_error(p, "You do not have enough money.");
		return;
	}

	game_io_info(p->game, "%s unmortgages '%s'.", p->name, e->name);
	player_pay(p, NULL, e->unmortgage_price, FALSE);
	e->mortgaged = 0;
	game_io_estateupdate(p->game, e);
}

void cmd_estate_buy(player *p) 
{
	estate *e = p->game->estates[p->location];

	if(e->price <= p->money) {
		player_pay(p, NULL, e->price, TRUE);
		e->owner = p;
		game_io_info(p->game, "%s buys '%s'.", p->name, e->name);
		game_io_estateupdate(p->game, e);
		player_remove_status(p, PLAYER_CANBUY);
		game_newturn(p->game);
	} else
		player_io_error(p, "'%s' costs $%d, but you only have $%d.",
				e->name, e->price, p->money);
}

void cmd_estate_auction(player *p) 
{
	player_remove_status(p, PLAYER_CANBUY);	
	new_auction(p->game->estates[p->location]);
}

void cmd_jail_roll(player *p) 
{
	int dice[2];
	struct timeval tv;
	
	gettimeofday(&tv, NULL);
	srand(tv.tv_usec);
	dice[0] = 1 + (int) (6.0f * rand()/(RAND_MAX + 1.0));
	srand(tv.tv_sec);
	dice[1] = 1 + (int) (6.0f * rand()/(RAND_MAX + 1.0));

	if(dice[0] == dice[1]) {
	        game_io_info(p->game, "%s rolled doubles, leaving jail!",
			     p->name);
		player_remove_status(p, PLAYER_JAILED);
	} else game_io_info(p->game,
			    "%s didn't roll doubles, staying in jail.",
			    p->name);
	game_newturn(p->game);
}

void cmd_jail_pay(player *p) 
{
	if(p->money >= p->game->jail_price) {
		player_pay(p, NULL, p->game->jail_price, FALSE);
		game_io_info(p->game, "%s paid to leave jail.", p->name);
		player_remove_status(p, PLAYER_JAILED);
	} else player_io_error(p, "You don't have enough money!");
	game_newturn(p->game);
}

void cmd_jail_card(player *p) 
{
	card *c;
	if((c = player_has_card(p, CARD_JAILCARD))) {
		player_remove_card(p, c);
		game_io_info(p->game, "%s used a 'Get out of jail free' card.",
			     p->name);
		player_remove_status(p, PLAYER_JAILED);
		game_newturn(p->game);
	} else player_io_error(p, "You don't have a jail  card.");
}
	
void cmd_paydebt(player *p) 
{
	if(p->money >= p->debt->amount) {
		if(p->debt->e_to)
			player_pay_to_location(p, p->debt->e_to,
					       p->debt->amount);
		else
			player_pay(p, p->debt->p_to, p->debt->amount, TRUE);
		delete_debt(p->debt);
		game_newturn(p->game);
	} else player_io_error(p, "You need $%d to pay off your $%d debt.",
			       p->debt->amount - p->money, p->debt->amount);
}

void cmd_declarebankruptcy(player *p) 
{
	player *p_to = p->debt->p_to;
	game *g = p->game;
	int i;
	
	for(i = 0; i < 40; i++)
		if(g->estates[i]->owner == p) {
			g->estates[i]->owner == p_to;
			game_io_estateupdate(g, g->estates[i]);
		}
	game_io_info(p->game,
		     "%s declares bankruptcy and leaves all property to %s!",
		     p->name, (p_to ? p_to->name : "the bank"));
	player_pay(p, p_to, p->money, TRUE);
	
	player_give_status(p, PLAYER_BANKRUPT);
	game_newturn(p->game);
}
