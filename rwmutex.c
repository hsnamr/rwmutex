// Last Updated: Fri Jan 04, 2008 @ 10:35 pm

#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <errno.h>
#include "rwmutex.h"

rwmutex_t *create_rwmutex(int N)
{
	if(N < 1)
	{
		printf("Error, lock value must be greater than 0\n");
		exit(-1);
	}

	rwmutex_t *ReadWriteMutex = (rwmutex_t *)malloc(sizeof(rwmutex_t));

	ReadWriteMutex->readersActive = 0;
	ReadWriteMutex->writersActive = 0;
	ReadWriteMutex->readersWaiting = 0;
	ReadWriteMutex->writersWaiting = 0;
	ReadWriteMutex->priorityValue = N;
	ReadWriteMutex->ReaderThreadIDs = NULL;

	if(pthread_mutex_init(&ReadWriteMutex->ReadWriteLock, NULL) != 0) {
	    perror("Could not initialize mutex");
	    exit(2);
	}

	if(pthread_cond_init(&ReadWriteMutex->readCondition, NULL) != 0) {
	    perror("Could not initialize read Condition");
	    exit(2);
	}

	if(pthread_cond_init(&ReadWriteMutex->writeCondition, NULL) != 0) {
	    perror("Could not initialize write Condition");
	    exit(2);
	}

	return ReadWriteMutex;
}

void read_lock(rwmutex_t *ReadWriteMutex)
{
	list_search(&n, pthread_self());
	pause();

	if(!pthread_equal(n->data, pthread_self()))
	{
		pthread_mutex_lock(&ReadWriteMutex->ReadWriteLock);

		#ifdef LOG
		printf("Locking read\n");
		#endif

		if(ReadWriteMutex->writersActive > 0 || ReadWriteMutex->writersWaiting > 0)
		{
			ReadWriteMutex->readersWaiting++;
			#ifdef LOG
			printf("readers waiting %d\n",ReadWriteMutex->readersWaiting);
			#endif
			
			while (ReadWriteMutex-> writersActive > 0)
				pthread_cond_wait(&ReadWriteMutex->readCondition, &ReadWriteMutex->ReadWriteLock);
			ReadWriteMutex->readersWaiting--;
			
			#ifdef LOG
			printf("readers waiting %d\n",ReadWriteMutex->readersWaiting);
			#endif
		}

		ReadWriteMutex->readersActive++;

		list_add(&n, pthread_self());
		list_print(&n);	
		
		#ifdef LOG
		printf("readers active %d\n",ReadWriteMutex->readersActive);
		#endif
		
		pthread_mutex_unlock(&ReadWriteMutex->ReadWriteLock);
	}
	return;
}

void read_unlock(rwmutex_t *ReadWriteMutex)
{
	if(ReadWriteMutex->readersActive > 0)
	{
		list_search(&n, pthread_self());

		if(pthread_equal(n->data, pthread_self()))
		{
			list_remove(&n, list_search(&n, pthread_self()));
			pthread_mutex_lock(&ReadWriteMutex->ReadWriteLock);

			ReadWriteMutex->readersActive--;
			#ifdef LOG
			printf("Unlocking read\n");
			#endif

			if(ReadWriteMutex->readersActive == 0 && ReadWriteMutex->writersWaiting>0)
				pthread_cond_signal(&ReadWriteMutex->writeCondition);

			else
				pthread_cond_broadcast(&ReadWriteMutex->readCondition);

			pthread_mutex_unlock(&ReadWriteMutex->ReadWriteLock);
		}
	}
	return;
}

void write_lock(rwmutex_t *ReadWriteMutex)
{
	if(!pthread_equal(ReadWriteMutex->lockingThread, pthread_self()))
	{
		pthread_mutex_lock(&ReadWriteMutex->ReadWriteLock);

		#ifdef LOG
		printf("Locking Write\n");
		#endif

		if(ReadWriteMutex->writersActive > 0 || ReadWriteMutex->readersActive > 0)
		{
			ReadWriteMutex->writersWaiting++;
			#ifdef LOG
			printf("writers waiting %d\n",ReadWriteMutex->writersWaiting);
			#endif
			
			while(ReadWriteMutex->writersActive > 0 || ReadWriteMutex->readersActive > 0)
				pthread_cond_wait(&ReadWriteMutex->writeCondition,
					&ReadWriteMutex->ReadWriteLock);
			ReadWriteMutex->writersWaiting--;
		}

		#ifdef LOG
		printf("Write Locked\n");
		#endif

		ReadWriteMutex->writersActive = 1;

		ReadWriteMutex->lockingThread = pthread_self();

		pthread_mutex_unlock(&ReadWriteMutex->ReadWriteLock);
	}

	return;
}

void write_unlock(rwmutex_t *ReadWriteMutex)
{
	if(ReadWriteMutex->writersActive > 0)
	{
		if(pthread_equal(ReadWriteMutex->lockingThread, pthread_self()))
		{

			pthread_mutex_lock(&ReadWriteMutex->ReadWriteLock);

			#ifdef LOG
			printf("Unlocking Write\n");
			#endif

			ReadWriteMutex->writersActive = 0;
			ReadWriteMutex->lockingThread = 0;

			if(ReadWriteMutex->readersWaiting < ReadWriteMutex->priorityValue)
				if(pthread_cond_signal(&ReadWriteMutex->writeCondition)!= 0) {
				 	perror("Error while signaling");
					exit(2);
				}

			else if(ReadWriteMutex->readersWaiting >= ReadWriteMutex->priorityValue)
				if(pthread_cond_broadcast(&ReadWriteMutex->readCondition) != 0) {
				 	perror("Error while broadcasting");
					exit(2);
				}

			#ifdef LOG
			printf("Write Unlocked\n");
			#endif
			
			pthread_mutex_unlock(&ReadWriteMutex->ReadWriteLock);

		}
	}
	return;
}

/* Linked List Implementation copied from http://en.wikipedia.org/wiki/Linked_list with modifications*/
/* All text is available under the terms of the GNU Free Documentation License */
 
node *list_add(node **p, pthread_t i) {
    /* some compilers don't require a cast of return value for malloc */
    node *n = (node *)malloc(sizeof(node));
    if (n == NULL)
        return NULL;
    n->next = *p;                                                                            
    *p = n;
    n->data = i;
    return n;
}
 
void list_remove(node **p) { /* remove head */
    if (*p != NULL) {
        node *n = *p;
        *p = (*p)->next;
        free(n);
    }
}
 
node **list_search(node **n, pthread_t i) {
    while (*n != NULL) {
        if (pthread_equal((*n)->data,i)) {
            return n;
        }
        n = &(*n)->next;
    }
    return NULL;
}

