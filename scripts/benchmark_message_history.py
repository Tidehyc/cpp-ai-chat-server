#!/usr/bin/env python3
"""Simple SQLite benchmark for message history search (indexed vs no-index)."""

import sqlite3
import time
from pathlib import Path

DB_PATH = Path("/tmp/chat_history_benchmark.db")
ROW_COUNT = 50000
ROUNDS = 100


def reset_db() -> sqlite3.Connection:
    if DB_PATH.exists():
        DB_PATH.unlink()

    conn = sqlite3.connect(str(DB_PATH))
    conn.execute("""
        CREATE TABLE messages (
            id INTEGER PRIMARY KEY AUTOINCREMENT,
            from_user TEXT NOT NULL,
            to_user TEXT NOT NULL,
            room_name TEXT,
            content TEXT NOT NULL,
            message_type TEXT DEFAULT 'text',
            is_read INTEGER DEFAULT 0,
            timestamp INTEGER NOT NULL
        )
    """)

    conn.execute("CREATE INDEX idx_timestamp ON messages(timestamp)")
    conn.execute("CREATE INDEX idx_room_name_timestamp ON messages(room_name, timestamp DESC)")

    base_ts = int(time.time()) - ROW_COUNT
    rows = []
    for i in range(ROW_COUNT):
        from_user = "user1" if i % 3 == 0 else ("user2" if i % 3 == 1 else "user3")
        to_user = "AI" if i % 5 == 0 else "general"
        room_name = "general" if i % 2 == 0 else "code"
        content = f"perf_message_{i}"
        rows.append((from_user, to_user, room_name, content, "text", 0, base_ts + i))

    conn.executemany(
        """
        INSERT INTO messages (from_user, to_user, room_name, content, message_type, is_read, timestamp)
        VALUES (?, ?, ?, ?, ?, ?, ?)
        """,
        rows,
    )
    conn.commit()
    return conn


def run_query_rounds(conn: sqlite3.Connection) -> float:
    sql = """
    SELECT id, from_user, to_user, room_name, content, message_type, timestamp
    FROM messages
    WHERE room_name = ? AND timestamp >= ? AND timestamp <= ?
    ORDER BY timestamp DESC
    LIMIT ?
    """

    start_time = int(time.time()) - ROW_COUNT
    end_time = int(time.time()) + 60

    t0 = time.perf_counter()
    for _ in range(ROUNDS):
        rows = conn.execute(sql, ("general", start_time, end_time, 50)).fetchall()
        if not rows:
            raise RuntimeError("query returned no rows")
    t1 = time.perf_counter()
    return (t1 - t0) * 1000.0


def main() -> None:
    conn = reset_db()

    indexed_ms = run_query_rounds(conn)

    conn.execute("DROP INDEX IF EXISTS idx_room_name_timestamp")
    conn.execute("DROP INDEX IF EXISTS idx_timestamp")
    conn.commit()

    no_index_ms = run_query_rounds(conn)

    print("=== Message History Benchmark ===")
    print(f"rows={ROW_COUNT}, rounds={ROUNDS}")
    print(f"indexed_total_ms={indexed_ms:.2f}")
    print(f"indexed_avg_ms={indexed_ms / ROUNDS:.4f}")
    print(f"no_index_total_ms={no_index_ms:.2f}")
    print(f"no_index_avg_ms={no_index_ms / ROUNDS:.4f}")
    if no_index_ms > 0:
        print(f"speedup={no_index_ms / indexed_ms:.2f}x")

    conn.close()


if __name__ == "__main__":
    main()
