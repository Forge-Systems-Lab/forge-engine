# 🏛️ forge-engine

A high-performance, low-latency task orchestration and asynchronous data ingestion runtime engine written in pure C. Built around an isolated condition-variable-backed ring buffer and a non-blocking control plane, `forge-engine` transitions traditional pipeline execution into a native, scalable network service layer.

## 📂 Ecosystem Architecture

This repository is organized as a modular monorepo, decoupling low-level data ingestion from runtime scheduling and IPC execution:

```text
forge-engine/
├── forge-broker/  # Persistent multi-threaded orchestration core & scheduler
├── forge-cli/     # Lightweight native TCP control client network proxy
├── forge-core/    # Low-latency bounded-memory stream ingestion layer
├── telemetry/     # Atomic, append-only observability tracking engine
└── shared/        # Centralized, unified header matrix definitions
```

## ⚡ Primitives & Core Capabilities

* **Asynchronous Isolation Engine:** Utilizes an `AF_INET` TCP network socket gateway to accept pipeline requests instantly, immediately delegating workloads to detached background ingestion worker threads without locking the listener loop.
* **Thread-Safe Work Distribution:** Operates a synchronized, custom ring-buffer queue backed by `pthread_mutex` and condition variables (`pthread_cond_t`) protecting against consumer/producer thread collisions and out-of-memory back-pressure.
* **Low-latency Stream Parser:** Designed to process high-throughput JSON metric sets with a bounded-memory footprint, filtering target metrics dynamically before routing to computation layers.
* **Append-Only Telemetry DB:** Features a thread-safe atomic monitoring logger that formats telemetry profiles into live structured text and JSON event ledgers.

## 🌐 HTTP Control Gateway API

The embedded C server exposes native web protocol routes over port `8080` to enable universal platform integration:

| Method | Endpoint | Description | Response Format |
| :--- | :--- | :--- | :--- |
| **`GET`** | `/` | Serves the built-in dark-mode HTML developer panel | `text/html` |
| **`GET`** | `/metrics` | Computes on-demand system performance states | `application/json` |
| **`POST`** | `/run` | Spun up background async data processing run | `text/plain` |

## 🚀 Quick Start (Development Simulation)

### Build Components
```bash
gcc forge-broker/main.c forge-broker/queue.c forge-broker/server.c telemetry/telemetry.c forge-core/parser.c -lpthread -o forge-broker-daemon
```

### Spin Up the Persistent Server
```bash
./forge-broker-daemon
```

### Query System Metrics Over Network Channels
```bash
curl http://localhost:8080/metrics
```
