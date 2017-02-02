#ifndef GAME_H
#define GAME_H

#include "const.h"

#define GAME_CONFIG  0x001
#define GAME_RUN 0x002

struct player;
struct estate;
struct card;
struct trade;
struct auction;
struct gmonopd;

typedef struct game 
{
	char *description;
	int id;
	int status;
	int dice[2];
	int numplayers;
	int houses, hotels;
	int playercounter, tradecounter, auctioncounter;
	int start_position;
	int jail_position, jail_price;
	
	struct gmonopd *server;
	struct player *players, *player_current, *master;
	struct estate *go;
	struct estate *estates[40];
	struct trade *trades;
	struct auction *auctions;
	struct card *cards_chance;
	struct card *cards_communitychest;

	struct game *next;
} game;

extern void game_io_estateupdate(game *g, struct estate *e);
extern void game_io_write(game *g, char *data, ...);
extern void game_io_error(game *g, char *data, ...);
extern void game_io_info(game *g, char *data, ...);

extern game *new_game(struct player *p, char *theme);
extern void delete_game(game *g);
extern void game_init_estates(game *g, char *theme);
extern void game_init_cards(game *g, int type, char *theme);
extern void game_newturn(game *g);
extern void game_start(game *g);
extern void game_set_description(game *g, char *data);
extern int game_has_debt(game *g);
extern void game_set_players_moving(game *g, struct player *p);
extern int game_has_players_moving(game *g);
extern struct auction *game_find_auction(game *g, int id);
extern struct estate *game_find_estate(game *g, int id);
extern struct trade *game_find_trade(game *g, int id);
extern struct player *game_find_player(game *g, int id);

extern char *escape_xml(char *str);

#endif
