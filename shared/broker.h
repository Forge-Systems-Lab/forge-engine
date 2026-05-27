#ifndef BROKER_H
#define BROKER_H

#include <pthread.h>
#include <stddef.h>

#define MAX_TYPE_LEN 16
#define MAX_QUEUE_SIZE 4096
#define MAX_PATH_LEN 256

typedef struct {
    int column_id;
    char data_type[MAX_TYPE_LEN];
    double volatility_index;
} BrokerTask;

typedef struct {
    BrokerTask tasks[MAX_QUEUE_SIZE];
    int head;
    int tail;
    int count;
    pthread_mutex_t lock;
    pthread_cond_t not_empty;
    pthread_cond_t not_full;
} BrokerQueue;

void queue_init(BrokerQueue *q);
void queue_push(BrokerQueue *q, BrokerTask task);
BrokerTask queue_pop(BrokerQueue *q);

// Updated interface to enforce live column metric filtering
int ingest_intelligence_payload(const char *json_path, BrokerQueue *q, double min_volatility);

#endif // BROKER_H
