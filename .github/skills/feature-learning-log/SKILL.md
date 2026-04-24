---
name: feature-learning-log
description: "Use when: a feature is completed and you need to record a structured development learning log in docs/devlog.md for later study and review"
---

# Feature Learning Log

Use this skill after finishing a feature, bug fix, or optimization to keep a high-quality learning record.

## Goal

Create reusable learning material, not just task notes. Keep entries consistent and searchable.

## Output Location

- Primary log file: docs/devlog.md

## Required Entry Sections

1. Date and short title
2. Completed work checklist
3. Problem background
4. Root cause analysis
5. Implementation details (with key code snippets only)
6. Validation and test evidence
7. Pitfalls and lessons learned
8. Next improvement items

## Workflow

1. Collect final change summary (what was changed and why).
2. Extract at most 2-3 key code snippets that explain the core idea.
3. Record objective verification results (build/test/runtime logs).
4. Add a concise "Lessons Learned" section with actionable patterns.
5. Append the new entry near the top of docs/devlog.md in the same style.

## Guardrails

- Do not include secrets, tokens, or private credentials.
- Do not paste excessive raw logs; keep only useful evidence.
- Keep each entry focused on one feature or one fix.
- Prefer concrete facts over generic reflection.
