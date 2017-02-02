#ifndef PLAYER_H
#define PLAYER_H

#include "network.h"

#define MAXNAME 1024
#define PLAYER_HASTURN  0x001
#define PLAYER_CANBUY   0x002
#define PLAYER_JAILED   0x004
#define PLAYER_CANROLL  0x008
#define PLAYER_BANKRUPT 0x010
#define PLAYER_DOUBLES  0x020
#define PLAYER_MOVING   0x040
#define PLAYER_MAINTAIN 0x080
#define PLAYER_PASSEDGO 0x100

#define COMMAND_GAME_NEW       0x0000001
#define COMMAND_GAME_LIST      0x0000002
#define COMMAND_GAME_JOIN      0x0000004
#define COMMAND_GAME_START     0x0000008
#define COMMAND_GAME_DESC      0x0000010
#define COMMAND_PAYDEBT        0x0000020
#define COMMAND_DECLARE        0x0000040
#define COMMAND_ROLL           0x0000080
#define COMMAND_ESTATE_BUY     0x0000100
#define COMMAND_ESTATE_AUCTION 0x0000200
#define COMMAND_JAIL_CARD      0x0000400
#define COMMAND_JAIL_PAY       0x0000800
#define COMMAND_JAIL_ROLL      0x0001000
#define COMMAND_MORTGAGE       0x0002000
#define COMMAND_UNMORTGAGE     0x0004000
#define COMMAND_ENDTURN        0x0008000
#define COMMAND_TRADE_NEW      0x0010000
#define COMMAND_TRADE_ESTATE   0x0020000
#define COMMAND_TRADE_MONEY    0x0040000
#define COMMAND_TRADE_ACCEPT   0x0080000
#define COMMAND_TRADE_REJECT   0x0100000
#define COMMAND_SETNAME        0x0200000
#define COMMAND_HOUSE_BUY      0x0400000
#define COMMAND_HOUSE_SELL     0x0800000
#define COMMAND_TOKENMOVE      0x1000000
#define COMMAND_AUCTION_BID    0x2000000

struct game;
struct debt;
struct sock;
struct estate;
struct card;

typedef struct player 
{
	char *name;
	char *host;
	int status;
	int commands;
	int location;
	int money;
	int doublecount, jailcount;
	int id;
	
	struct sock *sock;
	struct game *game;
	struct debt *debt;
	struct card *cards;

	struct event *timeout;	
	struct player *next;
} player;

extern player *new_player(sock *s);

extern void player_io_intro(player *p);
extern void player_io_gamelist(player *p);
extern void player_io_commandlist(player *p);
extern void player_io_write(player *p, char *data, ...);
extern void player_io_error(player *p, char *data, ...);
extern void player_io_info(player *p, char *data, ...);

extern int player_pay(player *p_from, player *p_to, int amount, int io);
extern void player_pay_each(player *p, int amount);
extern void player_pay_to_location(player *p, struct estate *e, int amount);
extern void player_pay_per_house(player *p, int amount);
extern void player_pay_per_hotel(player *p, int amount);
extern void player_set_commands(player *p);
extern void player_set_name(player *p, char *data);
extern void player_remove_status(player *p, int statbit);
extern void player_give_status(player *p, int statbit);
extern int player_has_status(player *p, int statbit);
extern void player_give_command(player *p, int commbit);
extern int player_has_command(player *p, int commbit);
extern void player_give_card(player *p, struct card *c);
extern struct card *player_has_card(player *p, int cardtype);
extern void player_remove_card(player *p, struct card *c);
extern int player_has_trade(player *p);
extern void player_advance(player *p, int distance);
extern void player_advance_to(player *p, int location);
extern void player_advance_to_with_go(player *p, int location);
extern void player_advance_next(player *p, int estatetype);
extern void player_tojail(player *p);
extern void delete_player(player *p);

#endif
