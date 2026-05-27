#ifndef TELEMETRY_H
#define TELEMETRY_H

#include <time.h>

// Definitive operational lifecycle states for system tracking
typedef enum {
    STATE_INGESTED,
    STATE_DISPATCHED,
    STATE_PROCESSING,
    STATE_COMPLETED,
    STATE_FAILED
} TaskState;

// Detailed tracking telemetry structural block
typedef struct {
    int column_id;
    char data_type[16];
    double volatility_index;
    TaskState current_state;
    unsigned int worker_thread_id;
    double execution_duration_ms;
} TelemetryEvent;

// Thread-safe core logging entry points
void telemetry_init(const char *log_path);
void telemetry_log_event(TelemetryEvent *event);
void telemetry_shutdown(void);

#endif // TELEMETRY_H
