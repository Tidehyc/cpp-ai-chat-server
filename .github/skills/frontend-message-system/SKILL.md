---
name: frontend-message-system
description: "Use when: implementing frontend chat message flow, WebSocket message rendering, reconnect handling, or AI mention interactions"
---

# Frontend Message System

Use this skill to implement or stabilize message behavior in the React frontend.

## Steps

1. Identify message state flow from socket input to UI output.
2. Normalize message shape and rendering responsibilities.
3. Implement robust send/receive flow and reconnect-safe buffering.
4. Add focused UI updates for timestamps, sender identity, and AI mentions.
5. Verify behavior with manual scenarios and existing frontend checks.

## Repository Notes

- Frontend root: frontend/
- Expected areas: frontend/src/pages/, frontend/src/components/, frontend/src/store/

## Guardrails

- Preserve current routing and state architecture.
- Prefer incremental UI changes over full page rewrites.
- Ensure both desktop and mobile layouts remain usable.
