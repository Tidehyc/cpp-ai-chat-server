---
name: cpp-cmake-test-fix
description: "Use when: fixing C++ build failures, CMake errors, test regressions, or failing GoogleTest cases in this repository"
---

# C++ CMake Test Fix

Use this skill to diagnose and fix build or test issues in the C++ backend.

## Steps

1. Verify current build state with CMake Tools build.
2. If build fails, collect diagnostics from Problems and identify root cause first.
3. Apply minimal code changes in src/ or tests/ without broad refactors.
4. Rebuild and rerun tests to confirm behavior.
5. Summarize root cause, fix, and residual risks.

## Repository Notes

- CMake entry: CMakeLists.txt
- Main tests target: chat_server_test
- Core source roots: src/, tests/

## Guardrails

- Prefer smallest possible fix for the failing scenario.
- Do not change public behavior unless required by the bug fix.
- Keep existing coding style and naming conventions.
