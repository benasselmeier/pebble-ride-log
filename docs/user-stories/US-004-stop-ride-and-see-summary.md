# US-004: Stop Ride and See Summary

## User Story
As a rider exiting a ride,
I want to stop logging and immediately see a summary,
so I can understand how intense that ride was.

## Acceptance Criteria
- User can stop recording from recording screen/menu.
- App captures end timestamp and session duration.
- Summary view shows at minimum:
  - max positive G
  - max negative G
  - duration
- Session is persisted locally.
- App returns to park mode for rapid next-ride logging.

## Out of Scope (v1)
- Detailed graph rendering.
- Cloud sync/export.

## Technical Notes
- Store a session record suitable for day aggregation.
- Add guards to reduce accidental stop events.
