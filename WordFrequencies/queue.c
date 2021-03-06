#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <pthread.h>

#ifndef QSIZE
#define QSIZE 8
#endif

typedef struct QNode{
    char* key;
    struct QNode* next;
}QNode;

typedef struct queue_t{
	QNode *front;
	QNode *rear;
	int activeThreads;
	unsigned count;
	pthread_mutex_t lock;
	pthread_cond_t read_ready;
	//pthread_cond_t write_ready;
} queue_t;

QNode* newNode(char* k){
    QNode* temp = malloc(sizeof(QNode));
	temp->key = malloc(strlen(k) + 1);
	strcpy(temp->key, k);
    temp->next = NULL;
    return temp;
}

int init(queue_t *Q)
{
    Q->front = NULL;
    Q->rear = NULL;
	Q->count = 0;
	pthread_mutex_init(&Q->lock, NULL);
	pthread_cond_init(&Q->read_ready, NULL);
	
	return 0;
}

int setThreads(queue_t *Q, int threads){
	Q->activeThreads = threads;
}

int destroy(queue_t *Q)
{
	pthread_mutex_destroy(&Q->lock);
	pthread_cond_destroy(&Q->read_ready);
	//pthread_cond_destroy(&Q->write_ready);

	return 0;
}
int qclose(queue_t *Q)
{
	pthread_mutex_lock(&Q->lock);
	pthread_cond_broadcast(&Q->read_ready);
	pthread_mutex_unlock(&Q->lock);	

	return 0;
}

// add item to end of queue
// if the queue is full, block until space becomes available
int enqueue(queue_t *Q, char* item)
{
    pthread_mutex_lock(&Q->lock);
	
    QNode* temp = newNode(item);
    if(Q->rear == NULL){
        Q->front = Q->rear = temp;
    }
    else{
        Q->rear->next = temp;
        Q->rear = temp;
    }
	++Q->count;
	
	pthread_cond_signal(&Q->read_ready);
	pthread_mutex_unlock(&Q->lock);
	
	return 0;
}

char* dequeue(queue_t *Q)
{
	pthread_mutex_lock(&Q->lock);
	
	if(Q->count == 0){
		Q->activeThreads--;
		if(Q->activeThreads == 0){
			pthread_mutex_unlock(&Q->lock);
			pthread_cond_broadcast(&Q->read_ready);
			return NULL;
		}
		
		while(Q->count == 0 && Q->activeThreads != 0){
			pthread_cond_wait(&Q->read_ready, &Q->lock);
		}
		
		if(Q->count == 0){
			pthread_mutex_unlock(&Q->lock);
			return NULL;
		}
		Q->activeThreads++;
	}

    QNode* temp = Q->front;
	
	char* tempData;
	tempData = malloc(strlen(temp->key) + 1);
	strcpy(tempData, temp->key);
	Q->front = Q->front->next;
	
	if(Q->front == NULL){
		Q->rear = NULL;
	}

	free(temp->key);
	free(temp);
	--Q->count;

	//pthread_cond_signal(&Q->write_ready);
	
	pthread_mutex_unlock(&Q->lock);

	return tempData;
}