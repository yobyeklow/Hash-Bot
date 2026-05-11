#!/usr/bin/env bash
# Source this file to load env vars, or run it to start mining.
#   source run.sh          # load env vars into shell
#   ./run.sh               # start mining (GPU)
#   ./run.sh --engine cpu  # start mining with CPU

SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
cd "$SCRIPT_DIR"

# Load env vars from .env
if [ -f .env ]; then
    set -a
    source .env
    set +a
else
    echo "[!] Missing .env file. Copy .env.example to .env and edit it."
    exit 1
fi

# If sourced, just load env vars and return
if [[ "${BASH_SOURCE[0]}" != "${0}" ]]; then
    echo "[*] Environment loaded for this shell"
    return 0
fi

# Install Node.js dependencies
if [ ! -d node_modules ]; then
    echo "[*] Installing Node.js dependencies..."
    npm install
fi

# Build GPU miner if binary doesn't exist
if [ ! -f gpu-miner ] && command -v gcc &>/dev/null; then
    echo "[*] Building GPU miner..."
    if ! command -v pkg-config &>/dev/null || ! pkg-config --exists OpenCL &>/dev/null; then
        echo "[!] OpenCL not found. Install: sudo apt install opencl-headers ocl-icd-libopencl1"
        echo "    Falling back to CPU engine..."
        export ENGINE=cpu
    else
        make clean 2>/dev/null || true
        make
    fi
elif [ ! -f gpu-miner ]; then
    echo "[!] No gcc found. Install: sudo apt install build-essential opencl-headers ocl-icd-libopencl1"
fi

# Run miner.js
echo "[*] Starting miner (engine: ${ENGINE:-gpu})..."
exec node miner.js --engine "${ENGINE:-gpu}" "$@"
