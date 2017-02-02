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

/* debt.c: allocates and manipulates debts */

#include <stdlib.h>
#include "stub.h"
#include "player.h"
#include "estate.h"
#include "debt.h"

debt *new_debt(player *p_from, player *p_to, int amount)
{
	debt *d = (debt *)malloc(sizeof(debt));

	d->p_from = p_from;
	d->p_to = p_to;
	d->e_to = (estate *)NULL;
	d->amount = amount;
	p_from->debt = d;

	return d;
}

void delete_debt(debt *d) 
{
	d->p_from->debt = NULL;
	free(d);
}

		
		
