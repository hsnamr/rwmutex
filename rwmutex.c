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
	printf("Just entered read_lock\n");
	if(list_search(&ReadWriteMutex->ReaderThreadIDs, pthread_self()) == NULL)
	{
		pthread_mutex_lock(&ReadWriteMutex->ReadWriteLock);
		printf("Writers Active %d\n", ReadWriteMutex->writersActive);
		printf("Readers Active %d\n", ReadWriteMutex->readersActive);
		printf("Locking Read\n");

		if(ReadWriteMutex->writersActive > 0 || ReadWriteMutex->writersWaiting > 0)
		{
			ReadWriteMutex->readersWaiting++;
			printf("Waiting for writers to finish\n");
			while (ReadWriteMutex-> writersActive > 0)
				pthread_cond_wait(&ReadWriteMutex->readCondition, &ReadWriteMutex->ReadWriteLock);
			ReadWriteMutex->readersWaiting--;
		}

		printf("Adding an element\n");
		list_add(&ReadWriteMutex->ReaderThreadIDs, pthread_self());
		printf("Element Added\n");
		ReadWriteMutex->readersActive++;

		printf("Read Locked\n");
		pthread_mutex_unlock(&ReadWriteMutex->ReadWriteLock);
	}
	printf("Exiting read_lock\n");
	return;
}

void read_unlock(rwmutex_t *ReadWriteMutex)
{
	printf("Just entered read_unlock\n");
	if(ReadWriteMutex->readersActive > 0)
	{
		if(list_search(&ReadWriteMutex->ReaderThreadIDs ,pthread_self())!= NULL)
		{
			pthread_mutex_lock(&ReadWriteMutex->ReadWriteLock);
			printf("Unlocking Read\n");

			if(ReadWriteMutex->readersActive > 0)
				ReadWriteMutex->readersActive--;

			if(ReadWriteMutex->readersActive == 0 && ReadWriteMutex->writersWaiting>0)
				pthread_cond_signal(&ReadWriteMutex->writeCondition);

			else
				pthread_cond_broadcast(&ReadWriteMutex->readCondition);

			printf("Removing an element\n");
			list_remove(list_search(&ReadWriteMutex->ReaderThreadIDs, pthread_self()));
			printf("Elemenet removed\n");

			printf("Read Unlocked\n");
			pthread_mutex_unlock(&ReadWriteMutex->ReadWriteLock);
		}
	}
	printf("Exiting read_unlock\n");
	return;
}

void write_lock(rwmutex_t *ReadWriteMutex)
{
	printf("Just entered write_lock\n");
	if(!pthread_equal(ReadWriteMutex->lockingThread, pthread_self()))
	{
		pthread_mutex_lock(&ReadWriteMutex->ReadWriteLock);
		printf("Locking Write\n");

		if(ReadWriteMutex->writersActive > 0 || ReadWriteMutex->readersActive > 0)
		{
			ReadWriteMutex->writersWaiting++;
			printf("writers waiting %d\n",ReadWriteMutex->writersWaiting);
			printf("Waiting for readers/writers to finish\n");
			while(ReadWriteMutex->writersActive > 0 || ReadWriteMutex->readersActive > 0)
				pthread_cond_wait(&ReadWriteMutex->writeCondition,
					&ReadWriteMutex->ReadWriteLock);
			ReadWriteMutex->writersWaiting--;
		}

		ReadWriteMutex->writersActive = 1;
		ReadWriteMutex->lockingThread = pthread_self();

		printf("Write Locked\n");
		pthread_mutex_unlock(&ReadWriteMutex->ReadWriteLock);
	}
	printf("Exiting write_lock\n");
	return;
}

void write_unlock(rwmutex_t *ReadWriteMutex)
{
	printf("Just entered write_unlock\n");
	if(ReadWriteMutex->writersActive > 0)
	{
		if(pthread_equal(ReadWriteMutex->lockingThread, pthread_self()))
		{
			pthread_mutex_lock(&ReadWriteMutex->ReadWriteLock);
			printf("Unlocking Write\n");

			ReadWriteMutex->writersActive = 0;
			ReadWriteMutex->lockingThread = 0;

			if(ReadWriteMutex->readersWaiting < ReadWriteMutex->priorityValue)
				pthread_cond_signal(&ReadWriteMutex->writeCondition);

			else if(ReadWriteMutex->readersWaiting >= ReadWriteMutex->priorityValue)
				pthread_cond_broadcast(&ReadWriteMutex->readCondition);

			printf("Write Unlocked\n");
			pthread_mutex_unlock(&ReadWriteMutex->ReadWriteLock);
		}
	}
	printf("Exiting write_unlock\n");
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

