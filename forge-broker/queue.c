#include <pthread.h>
#include "../shared/broker.h"

void queue_init(BrokerQueue *q) {
    q->head = 0;
    q->tail = 0;
    q->count = 0;
    pthread_mutex_init(&q->lock, NULL);
    pthread_cond_init(&q->not_empty, NULL);
    pthread_cond_init(&q->not_full, NULL);
}

void queue_push(BrokerQueue *q, BrokerTask task) {
    pthread_mutex_lock(&q->lock);
    
    // Protect against queue overflow bounds conditions
    while (q->count == MAX_QUEUE_SIZE) {
        pthread_cond_wait(&q->not_full, &q->lock);
    }
    
    q->tasks[q->tail] = task;
    q->tail = (q->tail + 1) % MAX_QUEUE_SIZE;
    q->count++;
    
    // Broadcast context wake to idle data worker threads
    pthread_cond_signal(&q->not_empty);
    pthread_mutex_unlock(&q->lock);
}

BrokerTask queue_pop(BrokerQueue *q) {
    pthread_mutex_lock(&q->lock);
    
    // Block consumer threads safely if data queues are empty
    while (q->count == 0) {
        pthread_cond_wait(&q->not_empty, &q->lock);
    }
    
    BrokerTask task = q->tasks[q->head];
    q->head = (q->head + 1) % MAX_QUEUE_SIZE;
    q->count--;
    
    // Signal back-pressure release to producer bounds loops
    pthread_cond_signal(&q->not_full);
    pthread_mutex_unlock(&q->lock);
    
    return task;
}
