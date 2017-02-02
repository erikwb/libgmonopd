#ifndef CARD_H
#define CARD_H

#define CARD_CHANCE         1
#define CARD_COMMUNITYCHEST 2

#define CARD_TOJAIL       0x001
#define CARD_JAILCARD     0x002
#define CARD_PAY          0x004
#define CARD_PAY_EACH     0x008
#define CARD_PAY_HOUSE    0x010
#define CARD_PAY_HOTEL    0x020
#define CARD_ADVANCE      0x040
#define CARD_ADVANCE_TO   0x080
#define CARD_ADVANCE_NEXT 0x100

struct game;
struct player;

typedef struct card 
{
	char *title, *description;
	int pay_location, pay, pay_house, pay_hotel, pay_each;
	int actions, distance, location;
	int type, advancetype;

	struct game *game;
	struct card *next;
} card;

extern card *new_card(struct game *g, int type, char *text);
extern void card_do_action(card *c, struct player *p);
extern int card_has_action(card *c, int actionbit);
extern void card_give_action(card *c, int actionbit);
extern void card_remove_action(card *c, int actionbit);

#endif
