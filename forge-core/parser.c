#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include "../include/broker.h"
#include "../include/telemetry.h"

int ingest_intelligence_payload(const char *json_path, BrokerQueue *q, double min_volatility) {
    FILE *file = fopen(json_path, "r");
    if (!file) return -1;

    char line[1024];
    int tasks_ingested = 0;
    
    int current_column = -1;
    char current_type[MAX_TYPE_LEN] = "UNKNOWN";
    double current_volatility = 0.0;
    
    int has_column = 0;
    int has_type = 0;

    while (fgets(line, sizeof(line), file)) {
        char *col_key = strstr(line, "\"column\"");
        char *type_key = strstr(line, "\"type\"");
        char *vol_key = strstr(line, "\"volatility_index\"");

        if (col_key) {
            char *colon = strchr(col_key, ':');
            if (colon) {
                current_column = atoi(colon + 1);
                has_column = 1;
            }
        }

        if (type_key) {
            char *colon = strchr(type_key, ':');
            if (colon) {
                char *start = strchr(colon, 34);
                if (start) {
                    start++;
                    char *end = strchr(start, 34);
                    if (end) {
                        size_t len = end - start;
                        if (len >= MAX_TYPE_LEN) len = MAX_TYPE_LEN - 1;
                        strncpy(current_type, start, len);
                        current_type[len] = '\0';
                        has_type = 1;
                    }
                }
            }
        }

        if (vol_key) {
            char *colon = strchr(vol_key, ':');
            if (colon) {
                current_volatility = strtod(colon + 1, NULL);
            }
        }

        if (strstr(line, "}") && has_column && has_type) {
            // Live Metric Filtering Validation Pass
            if (current_volatility < min_volatility) {
                printf("[Parser Boundary] Dropped Column %d: Volatility %.4f is below minimum threshold %.4f\n", 
                       current_column, current_volatility, min_volatility);
                
                // Track the skipped column in your telemetry ledger for complete auditing
                TelemetryEvent ev = {
                    .column_id = current_column,
                    .volatility_index = current_volatility,
                    .current_state = STATE_FAILED, // Flagged as filtered/failed ingest
                    .worker_thread_id = 0,
                    .execution_duration_ms = 0.0
                };
                strncpy(ev.data_type, current_type, 16);
                telemetry_log_event(&ev);
            } 
            else {
                BrokerTask task;
                task.column_id = current_column;
                strncpy(task.data_type, current_type, MAX_TYPE_LEN);
                task.volatility_index = current_volatility;

                TelemetryEvent ev = {
                    .column_id = task.column_id,
                    .volatility_index = task.volatility_index,
                    .current_state = STATE_INGESTED,
                    .worker_thread_id = 0,
                    .execution_duration_ms = 0.0
                };
                strncpy(ev.data_type, task.data_type, 16);
                telemetry_log_event(&ev);

                queue_push(q, task);
                tasks_ingested++;

                ev.current_state = STATE_DISPATCHED;
                telemetry_log_event(&ev);
            }

            has_column = 0;
            has_type = 0;
            current_volatility = 0.0;
            strcpy(current_type, "UNKNOWN");
        }
    }

    fclose(file);
    return tasks_ingested;
}
