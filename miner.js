const { ethers } = require('ethers');
const dotenv = require('dotenv');
const path = require('path');
const fs = require('fs');
const { Worker } = require('worker_threads');
const os = require('os');
const crypto = require('crypto');

dotenv.config();

const args = {};
const raw = process.argv.slice(2);
for (let i = 0; i < raw.length; i++) {
  const key = raw[i].replace(/^--/, '');
  if (raw[i].startsWith('--')) {
    if (i + 1 < raw.length && !raw[i + 1].startsWith('--')) {
      args[key] = raw[++i];
    } else {
      args[key] = true;
    }
  }
}

const RPC_URL = args.rpc || process.env.RPC_URL;
const CONTRACT_ADDRESS = args.contract || process.env.CONTRACT_ADDRESS;
const PRIVATE_KEY = args['private-key'] || process.env.PRIVATE_KEY;
const ENGINE = args.engine || 'gpu';
const WORKERS = parseInt(args.workers, 10) || Math.max(1, os.cpus().length - 1);
const GAS_LIMIT = args['gas-limit'] ? parseInt(args['gas-limit'], 10) : null;
const MAX_MINTS = parseInt(args['max-mints'], 10) || 0;
const LOG_INTERVAL = parseInt(args['log-interval'], 10) || 5000;
const MAX_FEE_GWEI = args['max-fee-gwei'] ? parseFloat(args['max-fee-gwei']) : null;
const PRIORITY_FEE_GWEI = args['priority-fee-gwei'] ? parseFloat(args['priority-fee-gwei']) : null;
const FAST_SUBMIT = args['fast-submit'] || false;
const DRY_RUN = args['dry-run'] || false;
const KEEP_GOING = args['keep-going'] || false;
const COOLDOWN = parseInt(args.cooldown, 10) || 0;
const REFRESH_BLOCKS = parseInt(args['refresh-blocks'], 10) || 2;
const BLOCKCAP_RETRIES = parseInt(args['blockcap-retries'], 10) || 8;
const BLOCKCAP_POLL = parseFloat(args['blockcap-poll-interval']) || 1.0;
const RECEIPT_TIMEOUT = parseInt(args['receipt-timeout'], 10) || 180;
const GPU_BLOCKS = parseInt(args['gpu-blocks'], 10) || 16384;
const GPU_THREADS = parseInt(args['gpu-threads'], 10) || 64;
const GPU_ROUNDS = parseInt(args['gpu-rounds'], 10) || 16;
const GPU_MAX_SECONDS = parseFloat(args['gpu-max-seconds']) || 600;
const GPU_DEVICE = parseInt(args['gpu-device'], 10) || 0;
const GPU_SECONDS_PER_BLOCK = parseFloat(args['gpu-seconds-per-block']) || 12;

if (!RPC_URL || !CONTRACT_ADDRESS || !PRIVATE_KEY) {
  console.error('Missing required config. Provide RPC_URL, CONTRACT_ADDRESS, and PRIVATE_KEY via .env or --flags.');
  process.exit(1);
}

const ABI = JSON.parse(fs.readFileSync(path.join(__dirname, 'abi.json'), 'utf8'));

const provider = new ethers.JsonRpcProvider(RPC_URL);
const wallet = new ethers.Wallet(PRIVATE_KEY, provider);
const contract = new ethers.Contract(CONTRACT_ADDRESS, ABI, wallet);

const ZERO_192 = Buffer.alloc(24, 0);

function now() {
  return new Date().toISOString();
}

function log(level, msg, extra) {
  const suffix = extra ? ' ' + Object.entries(extra).map(([k, v]) => `${k}=${v}`).join(' ') : '';
  console.log(`${now()} ${level.toUpperCase().padEnd(7)} ${msg}${suffix}`);
}

function formatHashrate(h) {
  if (h >= 1e9) return (h / 1e9).toFixed(2) + ' GH/s';
  if (h >= 1e6) return (h / 1e6).toFixed(2) + ' MH/s';
  if (h >= 1e3) return (h / 1e3).toFixed(2) + ' KH/s';
  return h.toFixed(0) + ' H/s';
}

function formatToken(wei) {
  const whole = Number(wei) / 1e18;
  return whole.toFixed(whole < 1 ? 6 : 2);
}

function shortHex(hex) {
  return hex.length > 18 ? hex.slice(0, 10) + '...' + hex.slice(-6) : hex;
}

function randomUint256() {
  return BigInt('0x' + crypto.randomBytes(32).toString('hex'));
}

// Encode uint64 nonce as full uint256 (big-endian, right-justified)
function nonceToUint256Buf(nonce) {
  const buf = Buffer.alloc(32, 0);
  buf.writeBigUInt64BE(nonce, 24);
  return buf;
}

const { keccak256: jsKeccak256 } = require('js-sha3');

function proofHash(challenge, nonce) {
  const input = Buffer.concat([challenge, nonceToUint256Buf(nonce)]);
  return Buffer.from(jsKeccak256(input), 'hex');
}

function cpuMine(challengeHex, difficultyHex, numWorkers) {
  return new Promise((resolve, reject) => {
    const workers = [];
    let found = false;
    const cpuStarted = Date.now();
    const baseNonce = randomUint256();

    for (let i = 0; i < numWorkers; i++) {
      const worker = new Worker(`
        const { parentPort } = require('worker_threads');
        const { keccak256 } = require('js-sha3');

        parentPort.on('message', (msg) => {
          const { challengeHex, difficultyHex, startNonceStr, stepStr } = msg;
          const challenge = Buffer.from(challengeHex.replace('0x', '').padStart(64, '0'), 'hex');
          const difficulty = Buffer.from(difficultyHex.replace('0x', '').padStart(64, '0'), 'hex');
          const zero24 = Buffer.alloc(24, 0);
          let nonce = BigInt(startNonceStr);
          const step = BigInt(stepStr);
          let hashes = 0n;

          while (true) {
            const nonceBuf = Buffer.concat([zero24, (() => {
              const b = Buffer.alloc(8);
              b.writeBigUInt64BE(nonce);
              return b;
            })()]);
            const hash = Buffer.from(keccak256(Buffer.concat([challenge, nonceBuf])), 'hex');

            let less = false;
            for (let i = 0; i < 32; i++) {
              if (hash[i] < difficulty[i]) { less = true; break; }
              else if (hash[i] > difficulty[i]) break;
            }

            if (less) {
              parentPort.postMessage({ found: true, nonce: nonce.toString(), hashes: hashes.toString() });
              return;
            }

            nonce += step;
            hashes += 1n;
          }
        });
      `, { eval: true });

      const step = BigInt(numWorkers);
      const startNonce = baseNonce + BigInt(i);

      worker.on('message', (msg) => {
        if (msg.found && !found) {
          found = true;
          const elapsed = (Date.now() - cpuStarted) / 1000;
          log('info', 'CPU solution found', { nonce: msg.nonce, hashes: msg.hashes, elapsed: elapsed.toFixed(1) + 's' });
          for (const w of workers) { try { w.terminate(); } catch {} }
          resolve(msg.nonce);
        }
      });
      worker.on('error', reject);
      workers.push(worker);
      worker.postMessage({ challengeHex, difficultyHex, startNonceStr: startNonce.toString(), stepStr: step.toString() });
    }
  });
}

function gpuMine(challengeHex, difficultyHex) {
  return new Promise((resolve, reject) => {
    const gpuBinary = process.platform === 'win32' ? 'gpu-miner.exe' : './gpu-miner';
    const gpuPath = path.join(__dirname, gpuBinary);

    if (!fs.existsSync(gpuPath)) {
      reject(new Error(`GPU miner not built: ${gpuPath}`));
      return;
    }

    const startNonce = BigInt.asUintN(64, randomUint256());
    const started = Date.now();
    const child = require('child_process').spawn(gpuPath, [
      '--challenge', challengeHex.replace('0x', '').padStart(64, '0'),
      '--difficulty', difficultyHex.replace('0x', '').padStart(64, '0'),
      '--start', String(startNonce),
      '--batch-size', String(1 << 20),
      '--progress-ms', String(LOG_INTERVAL),
      '--cutoff-ms', String(Date.now() + GPU_MAX_SECONDS * 1000)
    ]);

    let stderr = '';
    let buffer = '';

    child.stdout.on('data', (chunk) => {
      buffer += chunk.toString('utf8');
      while (buffer.includes('\n')) {
        const idx = buffer.indexOf('\n');
        const line = buffer.slice(0, idx).trim();
        buffer = buffer.slice(idx + 1);
        if (!line) continue;
        let msg;
        try { msg = JSON.parse(line); } catch { continue; }
        if (msg.type === 'found') {
          const elapsed = (Date.now() - started) / 1000;
          log('success', 'GPU solution found', { nonce: msg.solution_nonce, hashes: msg.hashes, elapsed: elapsed.toFixed(1) + 's' });
          child.stdout.removeAllListeners('data');
          resolve(BigInt(msg.solution_nonce).toString(16));
        } else if (msg.type === 'progress') {
          const elapsed = (Date.now() - started) / 1000;
          const hashes = BigInt(msg.hashes);
          const rate = elapsed > 0 ? Number(hashes) / elapsed : 0;
          log('info', 'GPU searching', { rate: formatHashrate(rate), hashes: msg.hashes, nonce: msg.nonce, device: msg.device || '' });
        } else if (msg.type === 'expired') {
          log('warn', 'GPU search expired');
          const err = new Error('GPU miner: search expired');
          err.code = 'EXPIRED';
          reject(err);
        }
      }
    });

    child.stderr.on('data', (chunk) => { stderr += chunk.toString(); });
    child.on('close', (code) => {
      if (code !== 0 && code !== null) {
        reject(new Error(`GPU miner exited with code ${code}: ${stderr}`));
      }
    });
    child.on('error', reject);
  });
}

async function waitForNextBlock(w3, pollInterval) {
  const startBlock = await w3.getBlockNumber();
  while (true) {
    await new Promise(r => setTimeout(r, pollInterval * 1000));
    const current = await w3.getBlockNumber();
    if (current > startBlock) return current;
  }
}

function isBlockCapError(err) {
  const text = typeof err === 'string' ? err : (err.message || String(err));
  return text.includes('BlockCapReached') || text.includes('BLOCK_CAP_REACHED') || text.includes('0x4992976a');
}

async function buildTx(w3, contractInstance, account, nonceValue, chainId) {
  const tx = {
    from: account,
    nonce: await w3.getTransactionCount(account, 'pending'),
    chainId: chainId,
  };

  if (GAS_LIMIT) {
    tx.gasLimit = GAS_LIMIT;
  } else {
    try {
      const estimate = await contractInstance.mine.estimateGas(nonceValue, { from: account });
      tx.gasLimit = Math.max(100000, Number(estimate * 150n / 100n));
    } catch {
      tx.gasLimit = 300000;
    }
  }

  if (MAX_FEE_GWEI !== null && PRIORITY_FEE_GWEI !== null) {
    tx.maxPriorityFeePerGas = ethers.parseUnits(String(PRIORITY_FEE_GWEI), 'gwei');
    tx.maxFeePerGas = ethers.parseUnits(String(MAX_FEE_GWEI), 'gwei');
  } else {
    const feeData = await w3.getFeeData();
    if (feeData.maxFeePerGas && feeData.maxPriorityFeePerGas) {
      tx.maxPriorityFeePerGas = feeData.maxPriorityFeePerGas;
      tx.maxFeePerGas = feeData.maxFeePerGas;
    } else {
      tx.gasPrice = feeData.gasPrice;
    }
  }

  return contractInstance.mine.populateTransaction(nonceValue, tx);
}

async function ensureSolutionCurrent(contractInstance, account, nonceValue, expectedChallenge, expectedEpoch, expectedDifficulty) {
  const challenge = await contractInstance.getChallenge(account);
  if (challenge !== expectedChallenge) throw new Error('solution became stale: challenge changed');
  const { epoch } = await contractInstance.miningState();
  if (epoch !== expectedEpoch) throw new Error('solution became stale: epoch changed');
  const hash = proofHash(Buffer.from(expectedChallenge.replace('0x', ''), 'hex'), BigInt(nonceValue));
  const hashBI = BigInt('0x' + hash.toString('hex'));
  if (hashBI >= BigInt(expectedDifficulty)) throw new Error('solution became stale: no longer valid');
}

async function submitSolution(w3, contractInstance, solutionNonce, expectedChallenge, expectedEpoch, expectedDifficulty, account) {
  const submitStarted = Date.now();
  let attempts = 0;

  while (true) {
    let txData = null;
    try {
      if (!FAST_SUBMIT) {
        await ensureSolutionCurrent(contractInstance, account, solutionNonce, expectedChallenge, expectedEpoch, expectedDifficulty);
      }
      txData = await buildTx(w3, contractInstance, account, solutionNonce, await w3.getChainId());
    } catch (err) {
      if (!isBlockCapError(err) || attempts >= BLOCKCAP_RETRIES) throw err;
      attempts++;
      log('warn', 'block mint cap reached; waiting for next block', { attempt: attempts });
      await waitForNextBlock(w3, BLOCKCAP_POLL);
      continue;
    }

    if (DRY_RUN) {
      log('info', 'dry-run: transaction built', { nonce: String(solutionNonce) });
      return 'dry-run';
    }

    try {
      const tx = await contractInstance.mine(solutionNonce, {
        gasLimit: txData.gasLimit || 300000,
        ...(txData.maxFeePerGas ? { maxFeePerGas: txData.maxFeePerGas } : {}),
        ...(txData.maxPriorityFeePerGas ? { maxPriorityFeePerGas: txData.maxPriorityFeePerGas } : {}),
        ...(txData.gasPrice ? { gasPrice: txData.gasPrice } : {}),
      });
      log('info', 'submitted', { tx: tx.hash, nonce: String(solutionNonce), attempt: attempts, latency: `${(Date.now() - submitStarted) / 1000}s` });

      const receipt = await tx.wait(RECEIPT_TIMEOUT);
      if (receipt.status === 1) {
        log('success', 'mine accepted', { tx: tx.hash, block: receipt.blockNumber, gasUsed: String(receipt.gasUsed) });
        return tx.hash;
      }

      if (attempts < BLOCKCAP_RETRIES && isBlockCapError(receipt.revertReason || '')) {
        attempts++;
        log('warn', 'block mint cap in receipt; retrying', { tx: tx.hash, attempt: attempts });
        await waitForNextBlock(w3, BLOCKCAP_POLL);
        continue;
      }

      throw new Error(`transaction reverted: ${tx.hash}`);
    } catch (err) {
      if (!isBlockCapError(err) || attempts >= BLOCKCAP_RETRIES) throw err;
      attempts++;
      log('warn', 'block mint cap during submit; retrying', { attempt: attempts });
      await waitForNextBlock(w3, BLOCKCAP_POLL);
    }
  }
}

async function mineLoop() {
  const network = await provider.getNetwork();
  const chainId = network.chainId;
  const account = wallet.address;

  log('info', 'miner started', {
    network: network.name,
    chainId,
    account,
    contract: CONTRACT_ADDRESS,
    engine: ENGINE,
    workers: WORKERS,
    maxMints: MAX_MINTS || 'unlimited',
  });

  let mintCount = 0;
  let lastEpoch = null;
  let stopRequested = false;

  process.on('SIGINT', () => { stopRequested = true; log('warn', 'interrupted'); });
  process.on('SIGTERM', () => { stopRequested = true; log('warn', 'terminated'); });

  // Print initial status
  const initialState = await contract.miningState();
  log('info', 'status', {
    era: Number(initialState.era) + 1,
    reward: formatToken(initialState.reward),
    difficulty: '0x' + initialState.difficulty.toString(16).padStart(64, '0'),
    epoch: Number(initialState.epoch),
    epochBlocksLeft: Number(initialState.epochBlocksLeft_),
    minted: formatToken(initialState.minted),
    remaining: formatToken(initialState.remaining),
    balance: formatToken(await contract.balanceOf(account)),
  });

  while (!stopRequested && (MAX_MINTS === 0 || mintCount < MAX_MINTS)) {
    try {
      const genesisComplete = await contract.genesisComplete();
      if (!genesisComplete) {
        log('warn', 'genesis not complete; waiting 30s');
        await new Promise(r => setTimeout(r, 30000));
        continue;
      }

      const state = await contract.miningState();
      if (Number(state.remaining) <= 0 || Number(state.reward) <= 0) {
        log('warn', 'mining supply exhausted');
        return;
      }

      if (lastEpoch !== null && Number(state.epoch) !== lastEpoch) {
        log('info', 'epoch changed', { from: lastEpoch, to: Number(state.epoch) });
      }
      lastEpoch = Number(state.epoch);

      const epochBlocksLeft = Number(state.epochBlocksLeft_);
      if (epochBlocksLeft <= REFRESH_BLOCKS) {
        log('warn', 'epoch too close; waiting for refresh', { epochBlocksLeft, refreshBlocks: REFRESH_BLOCKS });
        await new Promise(r => setTimeout(r, 30000));
        continue;
      }

      const challengeHex = await contract.getChallenge(account);
      const diffHex = state.difficulty.toString(16).padStart(64, '0');

      log('info', 'mining', {
        challenge: shortHex(challengeHex),
        difficulty: '0x' + diffHex.slice(0, 16),
        epoch: lastEpoch,
        epochBlocksLeft,
      });

      const nonceHex = ENGINE === 'gpu'
        ? await gpuMine(challengeHex, diffHex)
        : await cpuMine(challengeHex, diffHex, WORKERS);

      const nonceBI = BigInt('0x' + nonceHex);

      await submitSolution(provider, contract, nonceBI, challengeHex, state.epoch, state.difficulty.toString(), account);

      mintCount++;
      log('success', `mint #${mintCount} completed`, { reward: formatToken(state.reward) });

      if (MAX_MINTS > 0 && mintCount >= MAX_MINTS) {
        log('info', `reached target (${MAX_MINTS} mints)`);
        process.exit(0);
      }

      if (COOLDOWN > 0) {
        await new Promise(r => setTimeout(r, COOLDOWN * 1000));
      }

    } catch (err) {
      const msg = err.message || String(err);
      if (msg.includes('ProofAlreadyUsed') || msg.includes('PROOF_ALREADY_USED')) {
        log('warn', 'nonce already used, refreshing...');
      } else if (msg.includes('InsufficientWork') || msg.includes('INSUFFICIENT_WORK')) {
        log('warn', 'nonce rejected (difficulty increased), retrying...');
      } else if (msg.includes('SupplyExhausted')) {
        log('info', 'supply exhausted! mining complete.');
        process.exit(0);
      } else if (msg.includes('TooSoon') || msg.includes('TOO_SOON')) {
        const wait = 3000 + Math.floor(Math.random() * 3000);
        log('warn', `too soon, waiting ${wait}ms...`);
        await new Promise(r => setTimeout(r, wait));
      } else if (msg.includes('TxCapExceeded') || msg.includes('TX_CAP_EXCEEDED')) {
        log('warn', 'tx cap exceeded, retrying...');
      } else if (err.code === 'EXPIRED') {
        log('warn', 'search expired, refreshing...');
      } else {
        log('error', 'unexpected error', { error: msg });
        if (!KEEP_GOING) throw err;
      }

      log('info', 'retrying in 5s...');
      await new Promise(r => setTimeout(r, 5000));
    }
  }
}

mineLoop().catch((err) => {
  log('error', 'fatal', { error: err.message });
  process.exit(1);
});
