#!/bin/bash

# Define relative path architecture boundaries
CORE_DIR="../forge-core"
BROKER_DIR="."

echo "================================================================="
echo " 🏛️  LAUNCHING INTEGRATED FORGE ECOSYSTEM DATA ENGINE PIPELINE  "
echo "================================================================="

# Step 1: Verify the ingestion core binary exists
if [ ! -f "$CORE_DIR/forge-core" ]; then
    echo "[-] Error: forge-core execution binary not found in $CORE_DIR"
    echo "[*] Please compile forge-core before running the pipeline."
    exit 1
fi

# Step 2: Trigger the High-Speed Ingestion Plane
echo "[+] Step 1: Invoking forge-core low-level ingestion engine..."
cd "$CORE_DIR" || exit 1
./forge-core .
cd - > /dev/null || exit 1

# Step 3: Verify payload generation and route it to the Broker interface
if [ ! -f "$CORE_DIR/intelligence.json" ]; then
    echo "[-] Error: Data plane failed to export intelligence.json"
    exit 1
fi

echo "[+] Step 2: Transferring structured payload data block to Control Plane..."
cp "$CORE_DIR/intelligence.json" "$BROKER_DIR/intelligence.json"

# Step 4: Execute the Hardened Orchestration Matrix
echo "[+] Step 3: Initializing forge-broker multi-threaded dispatcher..."
echo "-----------------------------------------------------------------"
./forge-broker
echo "-----------------------------------------------------------------"

echo "[+] Pipeline execution pass complete. System standing down."
