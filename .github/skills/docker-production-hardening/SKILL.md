---
name: docker-production-hardening
description: "Use when: preparing Docker deployment for production, adding TLS/WSS, tightening runtime security, or improving compose reliability"
---

# Docker Production Hardening

Use this skill for secure and stable container deployment improvements.

## Steps

1. Review Dockerfile and docker-compose service boundaries.
2. Identify security gaps: secrets handling, network exposure, user permissions.
3. Add concrete hardening changes with minimal operational complexity.
4. Validate startup, health, and connectivity paths.
5. Document deployment changes in docs/ for repeatable rollout.

## Repository Notes

- Build config: Dockerfile
- Runtime orchestration: docker-compose.yml
- Docs baseline: docs/DOCKER_GUIDE.md and docs/DEPLOYMENT.md

## Guardrails

- Do not hardcode secrets into images or compose files.
- Keep local development flow usable.
- Prefer explicit, testable changes over assumptions.
