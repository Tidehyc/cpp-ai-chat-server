---
name: database-query-optimizer
description: "Use when: optimizing SQLite query performance, adding indexes, reducing slow message history or search operations"
---

# Database Query Optimizer

Use this skill for performance tuning in SQLite-backed features.

## Steps

1. Locate hot SQL paths in storage and history modules.
2. Inspect query patterns for filter, sort, and pagination usage.
3. Propose targeted index changes with clear expected benefit.
4. Validate correctness of query results after optimization.
5. Record before/after latency for critical operations.

## Repository Notes

- DB layer: src/storage/database.cpp
- History/search layer: src/history/message_history.cpp
- Test file to extend: tests/test_message_history.cpp

## Guardrails

- Do not break existing SQL schema migrations.
- Avoid speculative indexes with no query consumer.
- Prefer measured improvements over broad rewrites.
