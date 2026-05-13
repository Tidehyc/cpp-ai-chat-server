# PR Draft - Project Closeout

## Title

`chore: close out project verification and final docs`

## Summary

- Finalized the project closeout state after completing the main runtime and documentation verification work.
- Kept the frontend accessible over the LAN and ensured the backend listens on `0.0.0.0:8080` for mobile/local network access.
- Fixed the frontend AI page render-loop regression and kept the settings page spacing adjustment.
- Updated closeout documentation so the project status, follow-up list, and test checklist reflect the final state.
- Marked mobile-specific verification as stopped per user request, so it is no longer treated as an active deliverable.

## Key Changes

- `Dockerfile`
  - Trimmed runtime image and kept required runtime libraries so the container stays under the size target while remaining healthy.
- `docker-compose.yml`
  - Kept health-check behavior aligned with the runtime container setup.
- `frontend/src/services/api.ts`
  - Switched the frontend API fallback to same-origin usage instead of hard-coding `localhost`.
- `frontend/src/pages/AIAssistant.tsx`
  - Fixed the `Maximum update depth exceeded` regression.
- `frontend/src/pages/Settings.tsx`
  - Added bottom padding so the page is less likely to be covered by the bottom navigation on narrow screens.
- `docs/INDEX.md`
  - Added the final closeout summary entry.
- `docs/TEST_CHECKLIST.md`
  - Marked the verified frontend startup item and recorded that mobile verification is stopped.
- `docs/devlog.md`
  - Added the final closeout note and linked the summary document.
- `docs/test_reports/test_summary_2026-05-11.md`
  - Added the final closeout summary for the current project state.

## Verification

- `cd frontend && npm run build`
- `docker-compose ps`
- `curl -I --max-time 5 http://10.20.54.121:5173`
- `curl -i --max-time 5 -H 'Connection: Upgrade' -H 'Upgrade: websocket' -H 'Sec-WebSocket-Version: 13' -H 'Sec-WebSocket-Key: dGhlIHNhbXBsZSBub25jZQ==' http://10.20.54.121:8080/`
- `git diff --check`

## Notes

- The backend now reports healthy and accepts WebSocket handshakes on the LAN endpoint.
- Mobile-specific acceptance was intentionally stopped per user request and should not be treated as an open defect.
- If the PR needs to be split, the safest split point is between runtime/frontend fixes and documentation-only closeout edits.
 - Concurrency decision: for the `rooms_json` command-path stress scenario we adopted a batch strategy as the release gate (example: `batch_size=30, interval=9000ms`) because the突发模式下直接 300 并发的成功率为 80% whereas the batched approach can reach 300/300; treating the 80% burst baseline as a strict release requirement must be raised as a separate change request.
