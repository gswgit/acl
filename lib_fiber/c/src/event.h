#ifndef EVENT_INCLUDE_H
#define EVENT_INCLUDE_H

#include "define.h"
#include "common/gettimeofday.h"

#ifdef	HAS_EPOLL
#include <sys/epoll.h>
#endif
#include "fiber/lib_fiber.h"

#define SET_TIME(x) do { \
    struct timeval _tv; \
    gettimeofday(&_tv, NULL); \
    (x) = ((long long) _tv.tv_sec) * 1000 + ((long long) _tv.tv_usec)/ 1000; \
} while (0)

typedef struct FILE_EVENT   FILE_EVENT;
typedef struct EVENT        EVENT;

#ifdef HAS_POLL
typedef struct POLLFD       POLLFD;
typedef struct POLL_CTX     POLL_CTX;
typedef struct POLL_EVENT   POLL_EVENT;
#endif

#ifdef HAS_EPOLL
typedef struct EPOLL_CTX    EPOLL_CTX;
typedef struct EPOLL_EVENT  EPOLL_EVENT;
#endif

typedef int  event_oper(EVENT *ev, FILE_EVENT *fe);
typedef void event_proc(EVENT *ev, FILE_EVENT *fe);

#ifdef HAS_POLL
typedef void poll_proc(EVENT *ev, POLL_EVENT *pe);
#endif

#ifdef HAS_EPOLL
typedef void epoll_proc(EVENT *ev, EPOLL_EVENT *ee);
#endif

/**
 * for each connection fd
 */
struct FILE_EVENT {
	RING       me;
	ACL_FIBER *fiber;
	socket_t   fd;
	int id;
	int type;
#define	TYPE_NONE		0
#define	TYPE_SOCK		1
#define	TYPE_NOSOCK		2

	unsigned oper;
#define	EVENT_ADD_READ		(unsigned) (1 << 0)
#define	EVENT_DEL_READ		(unsigned) (1 << 1)
#define	EVENT_ADD_WRITE		(unsigned) (1 << 2)
#define	EVENT_DEL_WRITE		(unsigned) (1 << 3)
#define EVENT_ADD_CONNECT	(unsigned) (1 << 4)
#define EVENT_DEL_CONNECT	(unsigned) (1 << 5)  // not used yet!

	unsigned mask;
#define	EVENT_NONE		0
#define	EVENT_READ		(unsigned) (1 << 0)
#define	EVENT_WRITE		(unsigned) (1 << 1)
#define	EVENT_ERROR		(unsigned) (1 << 2)

	event_proc   *r_proc;
	event_proc   *w_proc;
#ifdef HAS_POLL
	POLLFD       *pfd;
#endif
#ifdef HAS_EPOLL
	EPOLL_CTX    *epx;
#endif
};

#ifdef HAS_POLL
struct POLLFD {
	FILE_EVENT *fe;
	POLL_EVENT *pe;
	struct pollfd *pfd;
};

struct POLL_EVENT {
	RING       me;
	ACL_FIBER *fiber;
	poll_proc *proc;
	int        nready;
	int        nfds;
	POLLFD    *fds;
};
#endif

#ifdef	HAS_EPOLL
struct EPOLL_CTX {
	int  fd;
	int  op;
	int  mask;
	int  rmask;
	FILE_EVENT  *fe;
	EPOLL_EVENT *ee;
	epoll_data_t data;
};

struct EPOLL_EVENT {
	RING        me;
	ACL_FIBER  *fiber;
	epoll_proc *proc;
	size_t      nfds;
	EPOLL_CTX **fds;
	int         epfd;

	struct epoll_event *events;
	int maxevents;
	int nready;
};
#endif

struct EVENT {
	RING events;
	int  timeout;
	socket_t setsize;
	socket_t maxfd;

#ifdef HAS_POLL
	RING   poll_list;
#endif
#ifdef HAS_EPOLL
	RING   epoll_list;
#endif

	const char *(*name)(void);
	int  (*handle)(EVENT *);
	void (*free)(EVENT *);

	int  (*event_wait)(EVENT *, int);

	event_oper *add_read;
	event_oper *add_write;
	event_oper *del_read;
	event_oper *del_write;
};

/* file_event.c */
void file_event_init(FILE_EVENT *fe, int fd);
FILE_EVENT *file_event_alloc(int fd);
void file_event_free(FILE_EVENT *fe);

/* event.c */
void event_set(int event_mode);
EVENT *event_create(int size);
const char *event_name(EVENT *ev);
int  event_handle(EVENT *ev);
int  event_size(EVENT *ev);
void event_free(EVENT *ev);
void event_close(EVENT *ev, FILE_EVENT *fe);

int  event_add_read(EVENT *ev, FILE_EVENT *fe, event_proc *proc);
int  event_add_write(EVENT *ev, FILE_EVENT *fe, event_proc *proc);
void event_del_read(EVENT *ev, FILE_EVENT *fe);
void event_del_write(EVENT *ev, FILE_EVENT *fe);
int  event_process(EVENT *ev, int left);

#endif
