# US-005: Daily Stats Watchface MVP

## User Story
As a rider spending the day at a park,
I want a simple watchface that shows my top ride-force stats,
so I can check progress at a glance between rides.

## Acceptance Criteria
- Watchface displays time plus day stats from completed sessions.
- Day stats include:
  - highest positive G
  - lowest negative G
  - ride with most airtime (or placeholder until airtime metric is implemented)
  - ride with highest acceleration rate (or placeholder until metric is implemented)
- Stats refresh after each completed ride session.
- If no rides logged today, watchface shows a clean empty state.

## Out of Scope (v1)
- Complex UI widgets/animations.
- Historical multi-day analytics on face.

## Technical Notes
- Maintain a simple `DailyStats` aggregate keyed by date.
- Keep watchface rendering lightweight to protect battery life.
