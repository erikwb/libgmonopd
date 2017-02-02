#ifndef EVENT_H
#define EVENT_H

struct game;
struct gmonopd;

typedef struct event 
{
	int permanent;
	long launchtime;
	int type;
	void (*action)(struct event *);
	void *parent;

	struct gmonopd *server;
	struct event *next;
} event;

extern event *new_event(struct gmonopd *, void *, int, void (*)(struct event *));
extern void delete_event(event *ev);
extern void event_tokentimeout(event *ev);
extern void event_auctiontimeout(event *ev);

#endif
