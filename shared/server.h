#ifndef SERVER_H
#define SERVER_H

#include "broker.h"

#define PORT 8080
#define BUFFER_SIZE 2048

typedef struct {
    char target_source[256];
    BrokerQueue *queue;
    double min_volatility;
} IngestionArgs;

void* socket_server_routine(void *arg);

#endif // SERVER_H
