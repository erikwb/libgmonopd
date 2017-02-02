#ifndef DEBT_H
#define DEBT_H

struct player;
struct estate;

typedef struct debt 
{
	struct player *p_from, *p_to;
	struct estate *e_to;
	int amount;
} debt;

debt *new_debt(struct player *p_from, struct player *p_to, int amount);
void delet_debt(debt *d);

#endif
