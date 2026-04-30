Style consistency scan report

Scan date: 2026-04-30
Scope: frontend/src/**

Summary:
- Tokens searched: bg-white, text-slate-, border-slate-, bg-gray-50, bg-slate-100
- Total matches found: 99

Top affected files (examples):
- src/pages/Chat.tsx (multiple occurrences: inputs, cards, message bubbles)
- src/pages/Contact.tsx
- src/pages/AIAssistant.tsx
- src/pages/Settings.tsx
- src/pages/Login.tsx (auth panel — already converted partially)
- src/components/ChatList.tsx
- src/components/ContextModal.tsx
- src/components/SearchModal.tsx
- src/components/HistoryModal.tsx
- src/components/ExportModal.tsx

Notes and suggested replacement mappings (proposal):
- Replace `bg-white` / `bg-white/XX` -> `theme-surface` (use semantic surface class)
- Replace `text-slate-900`, `text-slate-800`, `text-slate-700` -> `text-inherit` or semantic headings (e.g., keep bold/weight and use `text-inherit`)
- Replace `text-slate-500`, `text-slate-600`, `text-slate-400` -> `theme-muted`
- Replace `border-slate-200*/*` -> `theme-divider`
- Replace `bg-gray-50`, `bg-slate-100` -> `theme-surface` or `theme-card` (context dependent)

Risk notes:
- Tailwind opacity variants like `bg-white/85` cannot be replaced by a simple token substitution without adjusting markup or creating utility classes. Bulk replacement may change visuals; recommend performing replacements per-file or adding compatibility helper classes in CSS.
- Large-scale automatic replacements can introduce regressions. I can:
  1) Apply a conservative automated pass replacing `text-slate-5xx` -> `theme-muted` and `border-slate-200` -> `theme-divider` in `frontend/src/pages` and `frontend/src/components` only.
  2) Or generate per-file patches for manual review.

Next actions (choose):
- [A] Proceed with conservative automated replacements in `src/pages` and `src/components`.
- [B] Generate per-file patches and show diffs for your review before applying.
- [C] Create CSS compatibility helpers and then apply replacements progressively.

I can start with option A and run build/test afterwards, or follow your preferred option.
