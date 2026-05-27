#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <pthread.h>
#include "../include/telemetry.h"

static FILE *log_file = NULL;
static pthread_mutex_t log_mutex = PTHREAD_MUTEX_INITIALIZER;

// Utility function to map structural state enums to clear string traces
static const char* state_to_string(TaskState state) {
    switch(state) {
        case STATE_INGESTED:   return "INGESTED";
        case STATE_DISPATCHED: return "DISPATCHED";
        case STATE_PROCESSING: return "PROCESSING";
        case STATE_COMPLETED:  return "COMPLETED";
        case STATE_FAILED:     return "FAILED";
        default:               return "UNKNOWN";
    }
}

// Open the tracking log channel safely
void telemetry_init(const char *log_path) {
    pthread_mutex_lock(&log_mutex);
    log_file = fopen(log_path, "a"); // Open in append-only mode
    if (!log_file) {
        perror("[-] Failed to initialize telemetry log file");
    }
    pthread_mutex_unlock(&log_mutex);
}

// Thread-safe event ingestion to disk with explicit flushing
void telemetry_log_event(TelemetryEvent *event) {
    pthread_mutex_lock(&log_mutex);
    if (log_file) {
        time_t now = time(NULL);
        char time_str[26];
        struct tm *tm_info = localtime(&now);
        
        // Format readable timestamp structures
        if (tm_info) {
            strftime(time_str, sizeof(time_str), "%Y-%m-%d %H:%M:%S", tm_info);
        } else {
            strncpy(time_str, "TIMESTAMP_ERROR", sizeof(time_str));
        }

        fprintf(log_file, "[%s] [COLUMN_ID: %d] [TYPE: %s] [VOLATILITY: %.4f] [STATE: %s] [WORKER: %u] [DURATION: %.2fms]\n",
                time_str,
                event->column_id,
                event->data_type,
                event->volatility_index,
                state_to_string(event->current_state),
                event->worker_thread_id,
                event->execution_duration_ms);
                
        // Force the OS buffer cache to flush straight to disk immediately
        fflush(log_file); 
    }
    pthread_mutex_unlock(&log_mutex);
}

// Safely close file interfaces and tear down tracking locks
void telemetry_shutdown(void) {
    pthread_mutex_lock(&log_mutex);
    if (log_file) {
        fclose(log_file);
        log_file = NULL;
    }
    pthread_mutex_unlock(&log_mutex);
    pthread_mutex_destroy(&log_mutex);
}
