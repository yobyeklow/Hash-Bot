# Ethereum PoW GPU Miner

A CLI application that mines a specific Ethereum smart contract using Proof-of-Work (Keccak-256) with GPU (OpenCL) acceleration or CPU fallback.

## Prerequisites

- **Node.js** 18+ (for the orchestrator)
- **C compiler** with OpenCL support (for building the GPU miner, optional if using CPU mode):
  - Windows: Visual Studio with "Desktop development with C++" workload
  - Linux: `gcc` + `opencl-headers` + `ocl-icd-libopencl1`

## Installation

```bash
# Install Node.js dependencies
npm install

# Copy and configure environment
cp .env.example .env
# Edit .env with your RPC URL, contract address, and private key

# Build the GPU miner (optional — requires C compiler + OpenCL)
# Windows:
.\build.ps1
# Linux:
make
```

## Usage

```bash
# With GPU (default)
node miner.js

# With CPU fallback
node miner.js --engine cpu

# All options
node miner.js --rpc <URL> --contract <ADDRESS> --private-key <0xKEY> \
  --engine gpu|cpu --workers <n> --gas-limit <n> \
  --max-mints <n> --log-interval <ms>
```

### Command-line arguments

| Argument          | Description                                    | Default               |
|-------------------|------------------------------------------------|-----------------------|
| `--rpc`           | Ethereum JSON-RPC URL                          | From `.env`           |
| `--contract`      | Contract address                               | From `.env`           |
| `--private-key`   | Wallet private key (never logged)              | From `.env`           |
| `--engine`        | Mining engine: `gpu` or `cpu`                  | `gpu`                 |
| `--workers`       | Number of CPU worker threads                    | CPU core count        |
| `--gas-limit`     | Gas limit for `mine()` transaction             | `300000`              |
| `--max-mints`     | Maximum number of mints (0 = unlimited)        | `0`                   |
| `--log-interval`  | Status log interval in ms                      | `5000`                |

## How it works

1. Connects to an Ethereum node via JSON-RPC.
2. Fetches a unique PoW challenge (`getChallenge`) and current difficulty (`currentDifficulty`) from the contract.
3. GPU mode spawns `gpu-miner.exe` (OpenCL binary), CPU mode spawns worker threads — both search for a nonce where `keccak256(challenge || nonce) < difficulty`.
4. The winning nonce is submitted via `contract.mine(nonce)`.
5. Repeats automatically, refreshing the challenge every ~30 seconds or when the epoch changes.

## File structure

```
miner/
├── .env.example        # Example environment variables
├── package.json        # Node.js dependencies
├── miner.js            # Main CLI entry point
├── gpu-miner.c         # OpenCL GPU miner source
├── build.ps1           # Windows build script (MSVC)
├── Makefile            # Linux build script (gcc)
└── README.md           # This file
```

## Security

- Private keys are read from `.env` or command-line arguments only.
- Private keys are **never** logged, printed, or committed to version control.
- Add `.env` to `.gitignore` to prevent accidental commits.
