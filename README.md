# DSE — Daily Satta Exchange

A custom NSE-equivalent exchange written in C++17 for Linux. Speaks the NNF protocol so existing NNF clients (login, order entry, modify, cancel) work against it. Publishes every order-book event onto a shared-memory queue for a separate TBT broadcaster process.

## Architecture

```
                        ┌────────────────────────────────────────────┐
NNF client ─── TCP ─────► GR server (port 5000)                      │
                        │   ↓                                        │
                        │   issues BoxID + IP+port (gateway)         │
                        │                                            │
NNF client ─── TCP ─────► Gateway server (port 9090)                 │
                        │   ↓                                        │
                        │   Box reg (MD5) → Box signon → MS_SIGNON   │
                        │   → MS_OE_REQUEST (new/modify/cancel)      │
                        │                                            │
                        │   ┌─────────────────────────────┐          │
                        │   │ matching engine             │          │
                        │   │   intrusive DLL per level   │          │
                        │   │   map<token, map<price,L>>  │          │
                        │   │   onNew / onModify /        │          │
                        │   │   onCancel / onTrade        │          │
                        │   └────────┬────────────────────┘          │
                        │            ▼                               │
                        │   SPSC SHM ring `/dev/shm/tbt_shm`         │
                        │   64 MB, 1M × 64-byte slots                │
                        └────────────┬───────────────────────────────┘
                                     ▼
                              [ DSE-TBT broadcaster — separate repo ]
```

## Layout

| File | Role |
|---|---|
| `src/TcpHandler.cpp` | Multi-listener TCP server. Handles GR (login) on port 5000 and the gateway on 9090. Dispatches NNF messages by transaction code. |
| `src/matching_engine.cpp` | Price-time priority order book. Intrusive doubly-linked list per price level, O(1) cancel/modify. Publishes every event into the SPSC SHM queue. |
| `src/spsc_queue.cpp` | Lock-free single-producer single-consumer ring buffer over POSIX shared memory. Producer side. |
| `src/logging_object.cpp` | Async quill logger pinned to a non-trading core. |
| `include/nse_fo_structs.hpp` | NNF protocol structs (login + order entry + TBT messages), `pragma pack(1)`, big-endian on the wire. |
| `include/bswap.hpp` | Host↔big-endian conversion helpers. |
| `tools/send_gr_request.py` | Test client: full login (GR → secure box reg → box sign-on → MS_SIGNON) + new/modify/cancel order chain. |

## Build & run

```bash
cmake -S . -B build -DCMAKE_BUILD_TYPE=Release
cmake --build build -j
./build/dse
# in another terminal:
python3 tools/send_gr_request.py
```

The exchange opens `/dev/shm/tbt_shm` and writes every order-book event into it. Run [DSE-TBT](../DSE-TBT) alongside to consume the feed.

## Status

- ✅ NNF login flow (GR → box reg → box signon → MS_SIGNON) over plaintext TCP
- ✅ Order entry: new / modify / cancel parsed and dispatched into the matcher
- ✅ Price-time matching with intrusive list per price level
- ✅ TBT publish to shared memory (OrderMessage / TradeMessage)
- 🚧 AES-256-GCM session encryption (placeholder in protocol structs; not wired)
- 🚧 TLS 1.3 on the GR connection
- 🚧 Clean shutdown / signal handling
