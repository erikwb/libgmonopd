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

/* player.c: allocates and manipulates players */

#include <stdarg.h>

#include "stub.h"
#include "network.h"
#include "game.h"
#include "debt.h"
#include "card.h"
#include "trade.h"
#include "estate.h"
#include "player.h"
#include "auction.h"
#include "libgmonopd.h"

player *new_player(sock *s) 
{
	player *p = (player *)malloc(sizeof(player));

	p->name = (char *)malloc(strlen("Abner") + 1);
	strcpy(p->name, "Abner");

	p->host = (char *)malloc(strlen("0.0.0.0") + 1);
	strcpy(p->host, "0.0.0.0");

	p->status = 0;
	p->commands = 0;
	p->money = 1500;
	p->doublecount = 0;
	p->jailcount = 0;
	p->id = -1;
	
	p->game = NULL;
	p->debt = NULL;
	p->timeout = NULL;
	p->sock = s;
	p->cards = NULL;
	p->next = NULL;
	
	return p;
}

void delete_player(player *p) 
{
	player *p_trailer, *p_tmp;
	card *c_tmp;
	trade *t_tmp;
	auction *a_tmp;
	int i;

	if(p->game) {
		for(p_trailer = NULL, p_tmp = p->game->players;
		    p_tmp != p;
		    p_trailer = p_tmp, p_tmp = p_tmp->next);
		
		if(p_trailer == NULL) /* first one */
			p->game->players = p->next;
		else
			p_trailer->next = p->next;
		
		free(p->name);
		free(p->host);
		
		/* cancel all trades */
		for(t_tmp = p->game->trades; t_tmp; t_tmp = t_tmp->next)
			if((t_tmp->to == p) || (t_tmp->from == p))
				trade_reject(t_tmp, p);
		
		/* cancel all auction bids where p is the high bidder */
		for(a_tmp = p->game->auctions; a_tmp; a_tmp = a_tmp->next)
			if(a_tmp->highbidder == p)
				auction_cancel_bid(a_tmp);
		
		/* give back all cards */
		for(c_tmp = p->cards; c_tmp; c_tmp = c_tmp->next)
			player_remove_card(p, c_tmp);
		
		/* give back all estates */
		for(i = 0; i < 40; i++)
			if(p->game->estates[i]->owner == p)
				p->game->estates[i]->owner = NULL;
		
		if(p->game->players == NULL) /* no more players */
			delete_game(p->game);
	}
	
	p->sock->status = SOCK_GONE;
	p->sock->player = NULL;
	free(p);
	sock_handler(p->sock, NULL);
}

void player_pay_per_house(player *p, int amount) 
{
	int i, total = 0;
	for(i = 0; i < 40; i++)
		if((p->game->estates[i]->owner == p) &&
		   (p->game->estates[i]->houses < 5) &&
		   (p->game->estates[i]->houses > 0))
			total += (p->game->estates[i]->houses * amount);
	player_pay(p, NULL, total, TRUE);
}

void player_pay_per_hotel(player *p, int amount) 
{
	int i, total = 0;
	for(i = 0; i < 40; i++)
		if((p->game->estates[i]->owner == p) &&
		   (p->game->estates[i]->houses == 5))
			total += (p->game->estates[i]->houses * amount);
	player_pay(p, NULL, total, TRUE);
}

void player_pay_to_location(player *p, estate *e, int amount) 
{
	if(player_pay(p, NULL, amount, FALSE)) {
		e->fp_money += amount;
		game_io_info(p->game, "%s %s $%d %s '%s'.", p->name,
			     amount > 0 ? "puts" : "gets", abs(amount), 
			     amount > 0 ? "on" : "from", e->name);
		game_io_write(p->game, "<monopd><estateupdate estateid=\"%d\" money=\"%d\"/></monopd>\n", e->id, e->fp_money);
	} else p->debt->e_to = e;
}

void player_pay_each(player *p, int amount) 
{
	player *p_tmp;
	for(p_tmp = p->game->players; p_tmp; p_tmp = p_tmp->next)
		if(p_tmp != p) player_pay(p, p_tmp, amount, TRUE);
}

int player_pay(player *p_from, player *p_to, int amount, int io)
{
	if(amount == 0) return 1;
	
	if(p_from->money >= amount) {
		p_from->money -= amount;
		game_io_write(p_from->game, "<monopd><playerupdate playerid=\"%d\" money=\"%d\"/>", p_from->id, p_from->money);
		if(p_to) {
			p_to->money += amount;
			if(io) game_io_write(p_from->game, "<msg type=\"info\" value=\"%s %s %s $%d.\"/>", p_from->name, (amount > 0 ? "pays" : "receives from"), p_to->name, abs(amount));
			game_io_write(p_from->game, "<playerupdate playerid=\"%d\" money=\"%d\"/>", p_to->id, p_to->money);
		} else
			if(io) game_io_write(p_from->game, "<msg type=\"info\" value=\"%s %s $%d.\"/>", p_from->name, (amount > 0 ? "pays" : "receives"), abs(amount));
		game_io_write(p_from->game, "</monopd>\n");
	} else {
		new_debt(p_from, p_to, amount);
		game_io_info(p_from->game, "Game paused, %s owes %s $%d but is not solvent.  Player needs to raise $%d first.", p_from->name, (p_to ? p_to->name : "the bank"), amount, amount - p_from->money);
		return 0;
	}
	return 1;
}

void player_advance(player *p, int distance) 
{
	p->location += distance;
	if(p->location >= 40) {
		p->location -= 40;
		player_give_status(p, PLAYER_PASSEDGO);
	}
	game_io_write(p->game, "<monopd><playerupdate playerid=\"%d\" location=\"%d\" directmove=\"0\"/></monopd>\n", p->id, p->location);
}

void player_advance_to(player *p, int location) 
{
	p->location = location;
	game_io_write(p->game, "<monopd><playerupdate playerid=\"%d\" location=\"%d\" directmove=\"1\"/></monopd>\n", p->id, p->location);
}

void player_advance_to_with_go(player *p, int location) 
{
	int loc = p->location;
	p->location = location;
	if(loc >= p->location)
		player_pay(p, NULL, -200, TRUE);
	game_io_write(p->game, "<monopd><playerupdate playerid=\"%d\" location=\"%d\" directmove=\"0\"/></monopd>\n", p->id, p->location);
}

void player_advance_next(player *p, int estatetype) 
{
	int i, loc = p->location;
	for(i = loc+1; i < 40; i++)
		if(p->game->estates[i]->type == estatetype) {
			player_advance_to_with_go(p, i);
			return;
		}
	for(i = 0; i < loc; i++)
		if(p->game->estates[i]->type == estatetype) {
			player_advance_to_with_go(p, i);
			return;
		}
}

void player_give_card(player *p, card *c) 
{
	c->next = p->cards;
	p->cards = c;
}

card *player_has_card(player *p, int cardtype) 
{
	card *c_tmp;
	for(c_tmp = p->cards; c_tmp; c_tmp = c_tmp->next)
		if(card_has_action(c_tmp, cardtype)) return c_tmp;
	return NULL;
}

void player_remove_card(player *p, card *c) 
{
	card *c_tmp, *c_trailer;

	for(c_trailer = NULL, c_tmp = p->cards; c_tmp != c; 
	    c_trailer = c_tmp, c_tmp = c_tmp->next);
	if(c_trailer == NULL) p->cards = p->cards->next;
	else c_trailer->next = c_tmp->next;
	
	/* put it back in the deck */
	c_tmp = (c->type == CARD_CHANCE) ?
		(p->game->cards_chance) :
		(p->game->cards_communitychest);
	for(c_trailer = NULL;
	    c_tmp;
	    c_trailer = c_tmp, c_tmp = c_tmp->next);
	c_trailer->next = c;
	c->next = NULL;
}

int player_has_trade(player *p) 
{
	trade *t_tmp;
	if(!p->game) return;
	for(t_tmp = p->game->trades; t_tmp; t_tmp = t_tmp->next) {
		if(t_tmp->to == p) return TRUE;
		if(t_tmp->from == p) return TRUE;
	}
	return FALSE;
}

/* put the player on an estate, called after all clients confirm movement */
void player_land(player *p) 
{
	estate *e;
	card *c;

	if(p->timeout) {
		delete_event(p->timeout);
		p->timeout = NULL;
	}
	
	if(player_has_status(p, PLAYER_PASSEDGO)) {
		player_remove_status(p, PLAYER_PASSEDGO);
		game_io_info(p->game, "%s passes Go.", p->name);
		player_pay(p, NULL, -(p->game->go->go_money), TRUE);
	}

	e = p->game->estates[p->location];
	if(e->fp_money)
		player_pay_to_location(p, e, -e->fp_money);

	switch(e->type) {
	case ESTATE_STREET:
	case ESTATE_UTILITY:		
	case ESTATE_RR:
		if(e->owner == NULL) {
			game_io_info(p->game,
				     "%s lands on '%s' and it is unowned.",
				     p->name, e->name);
			player_give_status(p, PLAYER_CANBUY);
			player_io_commandlist(p);
		} else if(e->owner != p) {
			if(e->mortgaged)
				game_io_info(p->game, "%s lands on '%s', but it is mortgaged.", p->name, e->name);
			else {
				game_io_info(p->game, "%s lands on '%s', which is owned by %s.", p->name, e->name, e->owner->name);
				player_pay(p, e->owner, estate_get_rent(e),
					   TRUE);
			}
		} else
			game_io_info(p->game, 
				     "%s lands on '%s', but already owns it.",
				     p->name, e->name);
		break;
	case ESTATE_CHANCE:
	case ESTATE_COMMUNITYCHEST:
		game_io_info(p->game, "%s lands on '%s' and gets a card.",
			     p->name, e->name);
		if((c = estate_get_card(e, p)))
			card_do_action(c, p);
		break;
	case ESTATE_TOJAIL:
		game_io_info(p->game, "%s lands on '%s'.", p->name, e->name);
		player_tojail(p);
		break;
	case ESTATE_TAX: /* question the user too */
		player_pay_to_location(p, p->game->estates[e->pay_location],
				       e->tax);
	}

	if(!player_has_status(p, PLAYER_CANBUY))
		game_newturn(p->game);
}

void player_tojail(player *p) 
{
	p->location = p->game->jail_position;
	player_give_status(p, PLAYER_JAILED);
	player_remove_status(p, PLAYER_DOUBLES);
	p->jailcount = 0;
	game_io_write(p->game, "<monopd><playerupdate playerid=\"%d\" location=\"%d\" jailed=\"%d\"/></monopd>\n", p->id, p->location, player_has_status(p, PLAYER_JAILED));
}

void player_set_name(player *p, char *data) 
{
	free(p->name);
	p->name = escape_xml(data);

	if(p->game) {
		if(p->game->status == GAME_CONFIG)
			game_io_write(p->game, "<monopd><updateplayerlist type=\"edit\"><player playerid=\"%d\" name=\"%s\"/></updateplayerlist></monopd>\n", p->id, p->name);
		else
			game_io_write(p->game, "<monopd><playerupdate playerid=\"%d\" name=\"%s\"/></monopd>\n", p->id, p->name);
	}
}

void player_io_write(player *p, char *data, ...)
{
	va_list arg;
	char buf[BUFSIZE];
	
	va_start(arg, data);
	vsprintf(buf, data, arg);
	va_end(arg);
	
	sock_io_write(p->sock, buf);
}

void player_io_error(player *p, char *data, ...)
{
	va_list arg;
	char buf[BUFSIZE];

	va_start(arg, data);
	vsprintf(buf, data, arg);
	va_end(arg);
	
	sock_io_write(p->sock,
		      "<monopd><msg type=\"error\" value=\"%s\"/></monopd>\n",
		      buf);
}

void player_io_info(player *p, char *data, ...)
{
	va_list arg;
	char buf[BUFSIZE];
	
	va_start(arg, data);
	vsprintf(buf, data, arg);
	va_end(arg);
	
	sock_io_write(p->sock, 
		      "<monopd><msg type=\"info\" value=\"%s\"/></monopd>\n",
		      buf);
}

void player_io_commandlist(player *p)
{
	player_set_commands(p);
	
	player_io_write(p, "<monopd><commandlist type=\"full\">");
	player_io_write(p, "<command id=\"setname\" syntax=\".n$1\" description=\"Set name to $1.\"/>");
	
	if(player_has_command(p, COMMAND_GAME_NEW))
		player_io_write(p, "<command id=\"newgame\" syntax=\".gn\" description=\"Start a new game\"/>");
	if(player_has_command(p, COMMAND_GAME_LIST))
		player_io_write(p, "<command id=\"listgames\" syntax=\".gl\" description=\"Fetch gamelist\"/>");
	if(player_has_command(p, COMMAND_GAME_JOIN))
		player_io_write(p, "<command id=\"joingame\" syntax=\".gj$1\" description=\"Join game $1\"/>");
	if(player_has_command(p, COMMAND_GAME_START))
		player_io_write(p, "<command id=\"startgame\" syntax=\".gs\" description=\"End configuration and start game\"/>");
	if(player_has_command(p, COMMAND_AUCTION_BID))
		player_io_write(p, "<command id=\"bid\" syntax=\".ab$1:$2\" description=\"Bid $2 dollars on auction $1\"/>");
	if(player_has_command(p, COMMAND_PAYDEBT))
		player_io_write(p, "<command id=\"paydebt\" syntax=\".p\" description=\"Pay off debt\"/>");
	if(player_has_command(p, COMMAND_DECLARE))
		player_io_write(p, "<command id=\"declarebankrupt\" syntax=\".D\" description=\"Declare bankruptcy\"/>");
	if(player_has_command(p, COMMAND_ROLL))
		player_io_write(p, "<command id=\"roll\" syntax=\".r\" description=\"Roll dice\"/>");
	if(player_has_command(p, COMMAND_ESTATE_BUY))
		player_io_write(p, "<command id=\"buy\" syntax=\".b\" description=\"Buy current property\"/>");
	if(player_has_command(p, COMMAND_ESTATE_AUCTION))
		player_io_write(p, "<command id=\"auction\" syntax=\".ea\" description=\"Auction current property\"/>");
	if(player_has_command(p, COMMAND_JAIL_PAY))
		player_io_write(p, "<command id=\"payjail\" syntax=\".p\" description=\"Pay to leave jail\"/>");
	if(player_has_command(p, COMMAND_JAIL_ROLL))
		player_io_write(p, "<command id=\"jailroll\" syntax=\".jr\" description=\"Roll dice\"/>");
	if(player_has_command(p, COMMAND_JAIL_CARD))
		player_io_write(p, "<command id=\"usecard\" syntax=\".c\" description=\"Use get-out-of-jail-free card\"/>");
	if(player_has_command(p, COMMAND_MORTGAGE))
		player_io_write(p, "<command id=\"mortgage\" syntax=\".m $1\" description=\"Mortgage property $1\"/>");
	if(player_has_command(p, COMMAND_UNMORTGAGE))
		player_io_write(p, "<command id=\"unmortgage\" syntax=\".u $1\" description=\"Unmortgage property $1\"/>");
	
	player_io_write(p, "</commandlist></monopd>\n");
}

void player_io_intro(player *p) 
{
	player_io_write(p, "<monopd><server name=\"%s\" version=\"%s\"/><client clientid=\"%d\" cookie=\"?\"/></monopd>\n", SERVER_NAME, SERVER_VERSION, 0);
	player_io_gamelist(p);
	player_io_commandlist(p);
}

void player_io_gamelist(player *p) 
{
	game *g, *games = p->sock->server->games;
	player_io_write(p, "<monopd><updategamelist type=\"full\">");

	for(g = games; g; g = g->next)
		if(g->status == GAME_CONFIG)
			player_io_write(p, "<game id=\"%d\" players=\"%d\" description=\"%s\"/>", g->id, g->playercounter, g->description);

	player_io_write(p, "</updategamelist></monopd>\n");
}

void player_set_commands(player *p) 
{
	p->commands = 0; /* zero commands */
	player_give_command(p, COMMAND_SETNAME);
	
	if(!p->game) {
		player_give_command(p, COMMAND_GAME_NEW);
		player_give_command(p, COMMAND_GAME_LIST);
		player_give_command(p, COMMAND_GAME_JOIN);
	} else {
		if((p->game->status == GAME_CONFIG) &&
		   (p == p->game->master)) {
			player_give_command(p, COMMAND_GAME_DESC);
			if(p->game->playercounter >= 2)
				player_give_command(p, COMMAND_GAME_START);
			return;
		}

		player_give_command(p, COMMAND_HOUSE_BUY);
		player_give_command(p, COMMAND_HOUSE_SELL);
		player_give_command(p, COMMAND_MORTGAGE);
		player_give_command(p, COMMAND_UNMORTGAGE);
		
		player_give_command(p, COMMAND_TOKENMOVE);
		player_give_command(p, COMMAND_TRADE_NEW);

		if(player_has_trade(p)) {
			player_give_command(p, COMMAND_TRADE_ESTATE);
			player_give_command(p, COMMAND_TRADE_MONEY);
			player_give_command(p, COMMAND_TRADE_ACCEPT);
			player_give_command(p, COMMAND_TRADE_REJECT);
		}
			
		if(game_has_auction(p->game))
			player_give_command(p, COMMAND_AUCTION_BID);
		if(p->debt) {
			player_give_command(p, COMMAND_PAYDEBT);
			player_give_command(p, COMMAND_DECLARE);
		}
		
		if(!game_has_debt(p->game) && !game_has_auction(p->game)) {
			if(player_has_status(p, PLAYER_CANROLL))
				player_give_command(p, COMMAND_ROLL);
			
			if(player_has_status(p, PLAYER_CANBUY)) {
				player_give_command(p, COMMAND_ESTATE_BUY);
				player_give_command(p, COMMAND_ESTATE_AUCTION);
			}
			if(player_has_status(p, PLAYER_HASTURN) &&
			   player_has_status(p, PLAYER_JAILED))	{
				player_give_command(p, COMMAND_JAIL_PAY);
				player_give_command(p, COMMAND_JAIL_ROLL);
				if(player_has_card(p, CARD_JAILCARD))
				   player_give_command(p, COMMAND_JAIL_CARD);
			}
		}
	}
}

int player_has_command(player *p, int commbit) 
{
	return p->commands & commbit;
}
	
void player_give_command(player *p, int commbit) 
{
	p->commands |= commbit;
}

int player_has_status(player *p, int statbit) 
{
	return p->status & statbit;
}

void player_give_status(player *p, int statbit)
{
	p->status |= statbit;
}

void player_remove_status(player *p, int statbit)
{
	p->status &= ~statbit;
}
