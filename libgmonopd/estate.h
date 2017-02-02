#ifndef ESTATE_H
#define ESTATE_H

enum estatetype {
	ESTATE_STREET, ESTATE_RR, ESTATE_AIRPORT, ESTATE_UTILITY,
	ESTATE_COMMUNITYCHEST, ESTATE_CHANCE, ESTATE_FREEPARKING,
	ESTATE_TOJAIL, ESTATE_TAX, ESTATE_GO, ESTATE_JAIL, ESTATE_OTHER 
};

struct player;
struct game;
struct card;

typedef struct estate 
{
	char *name;
	int group;
	int id;
	int mortgaged, houses;
	int price, house_price, sell_house_price;
	int mortgage_price, unmortgage_price;
	int tax, tax_percentage;
	int pay_location;
	int fp_money, go_money, rent[6];
	enum estatetype type;

	struct player *owner;
	struct game *game;
} estate;

extern estate *new_estate(struct game *g, int id, char *name);
extern struct card *estate_get_card(estate *e, struct player *p);
extern int estate_get_rent(estate *e);
extern int estate_group_has_buildings(estate *e);
extern int estate_can_be_mortgaged(estate *e, struct player *p);
extern int estate_can_be_unmortgaged(estate *e, struct player *p);
extern int estate_can_buy_houses(estate *e, struct player *p);
extern int estate_can_sell_houses(estate *e, struct player *p);
extern int estate_can_be_owned(estate *e);
extern int estate_max_houses(estate *e);
extern int estate_min_houses(estate *e);
extern int estate_group_is_monopoly(estate *e);
extern int estate_group_size(estate *e);
extern int estate_group_has_mortgages(estate *e);
extern char *estate_type_to_string(estate *e);
extern int estate_type_to_int(char *data);

#endif
