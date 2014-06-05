#include <pthread.h>
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <errno.h>

typedef struct ns {
        pthread_t data;
        struct ns *next;
} node;

typedef struct  {
	int readersActive;
	int writersActive;
	int readersWaiting;
	int writersWaiting;
	int priorityValue;
	pthread_mutex_t ReadWriteLock;
	pthread_cond_t readCondition;
	pthread_cond_t writeCondition;
	pthread_t lockingThread;
	node *ReaderThreadIDs;
} rwmutex_t;



rwmutex_t *create_rwmutex(int N);
void write_lock(rwmutex_t *);
void read_lock(rwmutex_t *);
void read_unlock(rwmutex_t *);
void write_unlock(rwmutex_t *);

/* Added to handle linked list */
node *list_add(node **p, pthread_t i);
void list_remove(node **p);
node **list_search(node **n, pthread_t i);

