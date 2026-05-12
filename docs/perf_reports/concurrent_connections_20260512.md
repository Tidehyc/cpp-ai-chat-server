# Concurrent Connections Report (300 clients) - 2026-05-12

## Scope

- Target: `ws://127.0.0.1:8080`
- Server status during test: `docker-compose ps` showed `cpp-ai-chat-server Up (healthy)`
- Tool: `scripts/test_concurrent_connections.py`
- Goal: validate 300 concurrent connections and basic command-path behavior.

## Commands

```bash
python3 scripts/test_concurrent_connections.py --uri ws://127.0.0.1:8080 --clients 300 --waves 1 --timeout 8 --mode connect_only
python3 scripts/test_concurrent_connections.py --uri ws://127.0.0.1:8080 --clients 300 --waves 1 --timeout 8 --mode rooms_json
```

## Raw Outputs

- `docs/perf_reports/concurrent_connections_20260512_raw.txt`
- `docs/perf_reports/concurrent_rooms_json_20260512_raw.txt`

## Results

1. `connect_only` (connection capacity baseline)
   - Result: `ok=300/300`
   - Summary: `total=300, failed=0, success_rate=100.00%`

2. `rooms_json` (connection + command path)
   - Result: `ok=240/300`
   - Summary: `total=300, failed=60, success_rate=80.00%`
   - Typical failure: `unexpected response type: system`

## Interpretation

- The server can sustain 300 concurrent WebSocket connections in `connect_only` mode.
- Under command-path load (`rooms_json`), 300 concurrent clients currently produce partial success (80%).
- Failure pattern indicates some clients receive an additional/earlier `system` message when the test expects only `room_list_update` next.

## Recommendation

- Keep `300 connect_only = pass` as current connection-capacity baseline.
- Treat `rooms_json` at 300 as a stress scenario requiring follow-up (protocol sequencing or client-side message filtering in the test harness).
