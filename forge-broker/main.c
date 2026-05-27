#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <pthread.h>
#include <string.h>
#include "../shared/broker.h"
#include "../shared/telemetry.h"
#include "../shared/server.h"

#define NUM_WORKERS 4

static void execute_numerical_analytics(BrokerTask *task, unsigned int worker_id) {
    printf("[Worker %u] [ANALYTICS] Processing Column ID: %d | Weight: %.4f\n", 
           worker_id, task->column_id, task->volatility_index);
    if (task->volatility_index > 0.5) {
        usleep(60000); 
    } else {
        usleep(20000); 
    }
}

static void execute_text_normalization(BrokerTask *task, unsigned int worker_id) {
    printf("[Worker %u] [TEXT] Normalizing Column ID: %d\n", worker_id, task->column_id);
    usleep(35000);
}

void* worker_routine(void *arg) {
    BrokerQueue *queue = (BrokerQueue*)arg;
    unsigned int thread_id = (unsigned int)pthread_self();
    
    while (1) {
        BrokerTask task = queue_pop(queue);
        if (task.column_id == -1) {
            queue_push(queue, task); 
            break;
        }

        TelemetryEvent ev = {
            .column_id = task.column_id,
            .volatility_index = task.volatility_index,
            .current_state = STATE_PROCESSING,
            .worker_thread_id = thread_id,
            .execution_duration_ms = 0.0
        };
        strncpy(ev.data_type, task.data_type, 16);
        telemetry_log_event(&ev);

        struct timespec start_time, end_time;
        clock_gettime(CLOCK_MONOTONIC, &start_time);

        if (strcmp(task.data_type, "INT") == 0 || strcmp(task.data_type, "FLOAT") == 0) {
            execute_numerical_analytics(&task, thread_id);
        } else if (strcmp(task.data_type, "STR") == 0) {
            execute_text_normalization(&task, thread_id);
        } else {
            usleep(10000);
        }

        clock_gettime(CLOCK_MONOTONIC, &end_time);
        double duration_ms = (end_time.tv_sec - start_time.tv_sec) * 1000.0 + 
                             (end_time.tv_nsec - start_time.tv_nsec) / 1000000.0;

        ev.current_state = STATE_COMPLETED;
        ev.execution_duration_ms = duration_ms;
        telemetry_log_event(&ev);
    }
    return NULL;
}

int main() {
    printf("🏛️ [FORGE-BROKER] PERSISTENT NETWORK ENGINE ACTIVE\n");
    
    const char *log_destination = "forge_state.log";
    telemetry_init(log_destination);
    
    BrokerQueue queue;
    queue_init(&queue);
    
    pthread_t workers[NUM_WORKERS];
    for (int i = 0; i < NUM_WORKERS; i++) {
        pthread_create(&workers[i], NULL, worker_routine, &queue);
    }
    
    pthread_t api_server_thread;
    pthread_create(&api_server_thread, NULL, socket_server_routine, &queue);
    
    pthread_join(api_server_thread, NULL);
    
    telemetry_shutdown();
    return 0;
}
