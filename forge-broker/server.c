#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <pthread.h>
#include "../shared/server.h"
#include "../shared/broker.h"

static void compile_http_json_response(char *http_response, size_t max_len) {
    FILE *log = fopen("forge_state.log", "r");
    int total_ingested = 0, total_processed = 0, total_filtered = 0;
    double total_duration_ms = 0.0, max_volatility = 0.0;

    if (log) {
        char line[1024];
        while (fgets(line, sizeof(line), log)) {
            if (strstr(line, "[STATE: INGESTED]")) total_ingested++;
            if (strstr(line, "[STATE: COMPLETED]")) total_processed++;
            if (strstr(line, "[STATE: FAILED]")) total_filtered++;

            char *vol_ptr = strstr(line, "[VOLATILITY: ");
            if (vol_ptr) {
                double vol = 0.0;
                if (sscanf(vol_ptr, "[VOLATILITY: %lf]", &vol) == 1 && vol > max_volatility) max_volatility = vol;
            }
            char *dur_ptr = strstr(line, "[DURATION: ");
            if (dur_ptr) {
                double dur = 0.0;
                if (sscanf(dur_ptr, "[DURATION: %lfms]", &dur) == 1) total_duration_ms += dur;
            }
        }
        fclose(log);
    }

    double avg_duration = (total_processed > 0) ? (total_duration_ms / total_processed) : 0.0;

    char json_body[1024];
    snprintf(json_body, sizeof(json_body),
             "{\n"
             "  \"status\": \"success\",\n"
             "  \"engine\": \"FORGE-BROKER v5.0\",\n"
             "  \"metrics\": {\n"
             "    \"total_discovered\": %d,\n"
             "    \"dropped_by_filters\": %d,\n"
             "    \"peak_volatility\": %.4f,\n"
             "    \"avg_latency_ms\": %.2f\n"
             "  }\n"
             "}\n",
             total_ingested, total_filtered, max_volatility, avg_duration);

    snprintf(http_response, max_len,
             "HTTP/1.1 200 OK\r\n"
             "Content-Type: application/json\r\n"
             "Content-Length: %zu\r\n"
             "Access-Control-Allow-Origin: *\r\n"
             "Connection: close\r\n"
             "\r\n"
             "%s",
             strlen(json_body), json_body);
}

static void serve_html_dashboard(int client_fd) {
    const char *html_body = 
        "<!DOCTYPE html>\n<html>\n<head>\n"
        "<title>FORGE Control Panel</title>\n"
        "<style>\n"
        "  body { font-family: -apple-system, sans-serif; background: #0f1115; color: #e4e6eb; padding: 40px; }\n"
        "  .card { background: #1a1d24; padding: 24px; border-radius: 8px; border: 1px solid #2e3440; max-width: 600px; margin: auto; }\n"
        "  h1 { color: #88c0d0; margin-top: 0; font-size: 24px; border-bottom: 2px solid #2e3440; padding-bottom: 10px; }\n"
        "  .metric { display: flex; justify-content: space-between; padding: 12px 0; border-bottom: 1px solid #2e3440; }\n"
        "  .value { font-family: monospace; color: #a3be8c; font-weight: bold; font-size: 16px; }\n"
        "  button { background: #88c0d0; color: #2e3440; border: none; padding: 12px 20px; font-weight: bold; border-radius: 4px; cursor: pointer; width: 100%; margin-top: 20px; font-size: 14px; }\n"
        "  button:hover { background: #8fbcbb; }\n"
        "</style>\n"
        "<script>\n"
        "  async function updateMetrics() {\n"
        "    const res = await fetch('/metrics');\n"
        "    const data = await res.json();\n"
        "    document.getElementById('disc').innerText = data.metrics.total_discovered;\n"
        "    document.getElementById('drop').innerText = data.metrics.dropped_by_filters;\n"
        "    document.getElementById('peak').innerText = data.metrics.peak_volatility.toFixed(4);\n"
        "    document.getElementById('lat').innerText = data.metrics.avg_latency_ms.toFixed(2) + ' ms';\n"
        "  }\n"
        "  async function triggerRun() {\n"
        "    await fetch('/run', { method: 'POST' });\n"
        "    setTimeout(updateMetrics, 200);\n"
        "  }\n"
        "  setInterval(updateMetrics, 1000);\n"
        "  window.onload = updateMetrics;\n"
        "</script>\n"
        "</head>\n<body>\n"
        "<div class='card'>\n"
        "  <h1>🏛️ FORGE PLATFORM ENGINE DASHBOARD</h1>\n"
        "  <div class='metric'><span>Total Columns Discovered</span><span id='disc' class='value'>-</span></div>\n"
        "  <div class='metric'><span>Dropped by Filters</span><span id='drop' class='value'>-</span></div>\n"
        "  <div class='metric'><span>Peak Dataset Volatility</span><span id='peak' class='value'>-</span></div>\n"
        "  <div class='metric'><span>Avg Thread Latency</span><span id='lat' class='value'>-</span></div>\n"
        "  <button onclick='triggerRun()'>⚡ TRIGGER PIPELINE RUN</button>\n"
        "</div>\n"
        "</body>\n</html>\n";

    char response[4096];
    snprintf(response, sizeof(response),
             "HTTP/1.1 200 OK\r\n"
             "Content-Type: text/html\r\n"
             "Content-Length: %zu\r\n"
             "Connection: close\r\n"
             "\r\n"
             "%s",
             strlen(html_body), html_body);
             
    send(client_fd, response, strlen(response), 0);
}

static void* async_ingestion_handler(void *arg) {
    IngestionArgs *args = (IngestionArgs*)arg;
    printf("[Async Ingest] Background pipeline parsing launched for: %s\n", args->target_source);
    int tasks = ingest_intelligence_payload(args->target_source, args->queue, args->min_volatility);
    printf("[Async Ingest] Complete. Sequenced %d valid tasks straight to workers.\n", tasks);
    free(args);
    return NULL;
}

void* socket_server_routine(void *arg) {
    BrokerQueue *queue = (BrokerQueue*)arg;
    int server_fd, client_fd;
    struct sockaddr_in address;
    int opt = 1;
    char buffer[BUFFER_SIZE];

    server_fd = socket(AF_INET, SOCK_STREAM, 0);
    if (server_fd < 0) return NULL;

    setsockopt(server_fd, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt));

    address.sin_family = AF_INET;
    address.sin_addr.s_addr = INADDR_ANY;
    address.sin_port = htons(PORT);

    if (bind(server_fd, (struct sockaddr *)&address, sizeof(address)) < 0) {
        perror("[-] Network port binding failed");
        close(server_fd);
        return NULL;
    }

    if (listen(server_fd, 10) < 0) {
        close(server_fd);
        return NULL;
    }

    printf("[API Server] HTTP Control Gateway actively live on: http://localhost:%d\n", PORT);

    while (1) {
        client_fd = accept(server_fd, NULL, NULL);
        if (client_fd < 0) break;

        memset(buffer, 0, BUFFER_SIZE);
        ssize_t bytes_received = recv(client_fd, buffer, BUFFER_SIZE - 1, 0);

        if (bytes_received > 0) {
            if (strncmp(buffer, "GET / ", 6) == 0 || strncmp(buffer, "GET /index.html", 15) == 0) {
                serve_html_dashboard(client_fd);
            }
            else if (strncmp(buffer, "GET /metrics", 12) == 0) {
                char response[2048] = {0};
                compile_http_json_response(response, sizeof(response));
                send(client_fd, response, strlen(response), 0);
            }
            else if (strncmp(buffer, "POST /run", 9) == 0) {
                IngestionArgs *i_args = malloc(sizeof(IngestionArgs));
                strcpy(i_args->target_source, "intelligence.json");
                i_args->queue = queue;
                i_args->min_volatility = 0.0;

                char *vol_param = strstr(buffer, "min_volatility=");
                if (vol_param) {
                    sscanf(vol_param, "min_volatility=%lf", &i_args->min_volatility);
                }

                pthread_t ingest_thread;
                pthread_create(&ingest_thread, NULL, async_ingestion_handler, i_args);
                pthread_detach(ingest_thread);

                const char *body = "STATUS: ASYNC_PIPELINE_RUN_INITIATED\n";
                char http_ack[256];
                snprintf(http_ack, sizeof(http_ack),
                         "HTTP/1.1 202 Accepted\r\n"
                         "Content-Type: text/plain\r\n"
                         "Content-Length: %zu\r\n"
                         "Connection: close\r\n"
                         "\r\n"
                         "%s", strlen(body), body);

                send(client_fd, http_ack, strlen(http_ack), 0);
            }
            else {
                const char *http_404 = 
                    "HTTP/1.1 404 Not Found\r\n"
                    "Content-Length: 0\r\n"
                    "Connection: close\r\n"
                    "\r\n";
                send(client_fd, http_404, strlen(http_404), 0);
            }
        }
        close(client_fd);
    }

    close(server_fd);
    return NULL;
}
