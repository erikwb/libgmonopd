#ifndef AUCTION_H
#define AUCTION_H

struct player;
struct estate;
struct event;
struct game;

typedef struct auction 
{
	int id;
	int lastbid;
	int highbid;
	int going;

	struct player *lastbidder;
	struct player *highbidder;
	struct estate *estate;
	struct game *game;
	struct event *event;

	struct auction *next;
} auction;

extern auction *new_auction(struct estate *e);
extern void delete_auction(auction *a);
extern void auction_set_going(auction *a, int going);
extern void auction_complete(auction *a);
extern void auction_cancel_bid(auction *a);

#endif
