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

/* trade.c: allocates and manipulates trades */

#include <stdlib.h>
#include <stdarg.h>

#include "stub.h"
#include "game.h"
#include "estate.h"
#include "player.h"
#include "trade.h"

/* external function bodies */
trade *new_trade(player *from, player *to)
{
	int i;
	trade *t = (trade *)malloc(sizeof(trade));

	t->from = from;
	t->to = to;
	t->from_money = 0;
	t->to_money = 0;
	t->from_accept = 0;
	t->to_accept = 0;
	t->game = from->game;
	t->id = t->game->tradecounter++;
	for(i = 0; i < 40; i++)	t->has_estate[i] = 0;

	t->next = t->game->trades;
	t->game->trades = t;

	trade_io_write(t, "<monopd><tradeupdate type=\"new\" tradeid=\"%d\" actor=\"%d\"><tradeplayer playerid=\"%d\"/><tradeplayer playerid=\"%d\"/></tradeupdate></monopd>\n", t->id, t->from->id, t->from->id, t->to->id);
	
	return t;
}

void trade_complete(trade *t) 
{
	int i;
	game *g = t->game;

	for(i = 0; i < 40; i++)
		if(t->has_estate[i]) {
			g->estates[i]->owner = 
				((g->estates[i]->owner == t->to) ?
				 t->from :
				 t->to);
			game_io_estateupdate(g, g->estates[i]);
		}
	
	player_pay(t->to, t->from, t->to_money, FALSE);
	player_pay(t->from, t->to, t->from_money, FALSE);
	
	trade_io_write(t, "<monopd><tradeupdate type=\"completed\" tradeid=\"%d\"/></monopd>\n", t->id);

	delete_trade(t);
}

void trade_reject(trade *t, player *p)
{
	trade_io_write(t, "<monopd><tradeupdate type=\"rejected\" actor=\"%d\" tradeid=\"%d\"/></monopd>\n", p->id, t->id);
	delete_trade(t);
}

void delete_trade(trade *t) 
{
	game *g = t->game;
	trade *t_tmp, *t_trailer;
	
	for(t_trailer = NULL, t_tmp = g->trades;
	    t_tmp != t;
	    t_trailer = t_tmp, t_tmp = t_tmp->next);
	
	if(t_trailer == NULL) /* first one */
		g->trades = t->next;
	else
		t_trailer->next = t->next;
	
	free(t);
}

void trade_toggle_accept(trade *t, player *p) 
{
	int val;
	if(p == t->to)
		val = t->to_accept = !(t->to_accept);
	else if(p == t->from)
		val = t->from_accept = !(t->from_accept);
	trade_io_write(t, "<monopd><tradeupdate tradeid=\"%d\" type=\"edit\"><tradeplayer playerid=\"%d\" accept=\"%d\"/></tradeupdate></monopd>\n", t->id, p->id, val);
}

void trade_money(trade *t, player *p, int money) 
{
	player *p_other = 0;

	if(p == t->to) {
		t->to_money = money;
		p_other = t->from;
	} else if(p == t->from) {
		t->from_money = money;
		p_other = t->to;
	}
	
	trade_io_write(t, "<monopd><tradeupdate tradeid=\"%d\" type=\"edit\" actor=\"%d\"><tradeplayer playerid=\"%d\" money=\"%d\" accept=\"0\"/><tradeplayer playerid=\"%d\" accept=\"0\"/></tradeupdate></monopd>\n", t->id, p->id, p->id, money, p_other->id);
}

void trade_toggle_estate(trade *t, estate *e, player *p) 
{
	if(t->has_estate[e->id])
		t->has_estate[e->id] = 0;
	else t->has_estate[e->id] = 1;

	trade_io_write(t, "<monopd><tradeupdate type=\"edit\" tradeid=\"%d\" actor=\"%d\"><estate estateid=\"%d\" included=\"%d\"/><tradeplayer playerid=\"%d\" accept=\"0\"/><tradeplayer playerid=\"%d\" accept=\"0\"/></tradeupdate></monopd>\n", t->id, p->id, e->id, t->has_estate[e->id], t->to->id, t->from->id);
}

void trade_io_write(trade *t, char *data, ...)
{
	va_list arg;
	char buf[BUFSIZE];
	
	va_start(arg, data);
	vsprintf(buf, data, arg);
	va_end(arg);
	
	player_io_write(t->to, buf);
	player_io_write(t->from, buf);
}
