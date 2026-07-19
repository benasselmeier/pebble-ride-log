# US-003: Start Ride Logging Quickly

## User Story
As a rider boarding a coaster,
I want to start logging with just a few button presses,
so I do not miss the beginning of the ride.

## Acceptance Criteria
- User can start a ride session from selected ride in <= 3 button actions.
- App gives immediate tactile confirmation when recording starts.
- App transitions to `RECORDING` state and begins accelerometer sampling.
- App records start timestamp and selected ride/park identifiers.

## Out of Scope (v1)
- Auto-start based on motion detection.
- Custom sampling profiles per ride type.

## Technical Notes
- Prefer fixed sample interval for predictable battery/performance.
- Keep UI minimal while recording.
