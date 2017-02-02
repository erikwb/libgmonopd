#ifndef TRADE_H
#define TRADE_H

struct player;
struct game;
struct estate;

typedef struct trade 
{
	struct game *game;
	struct player *from, *to;
	int from_money, to_money, from_accept, to_accept;
	int id;
	int has_estate[40];

	struct trade *next;
} trade;

trade *new_trade(struct player *from, struct player *to);
void trade_complete(trade *t);
void trade_reject(trade *t, struct player *p);
void delete_trade(trade *t);
void trade_toggle_accept(trade *t, struct player *p);
void trade_money(trade *t, struct player *p, int money);
void trade_toggle_estate(trade *t, struct estate *e, struct player *p);
void trade_io_write(trade *t, char *data, ...);

#endif
