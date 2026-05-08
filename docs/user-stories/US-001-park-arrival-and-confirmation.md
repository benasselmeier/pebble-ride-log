# US-001: Park Arrival & Park Confirmation

## User Story
As a rider arriving at a park,
I want the app to suggest my nearest park and ask for confirmation,
so I can start logging rides with minimal setup.

## Acceptance Criteria
- When the app opens, it requests current location (via phone companion).
- The app computes the nearest known park from local park coordinates.
- The app prompts: `Use <Park Name>?`
- User can Confirm or Choose Another Park.
- If location is unavailable, app falls back to manual park selection.

## Out of Scope (v1)
- Automatic background park switching.
- Geofencing notifications.

## Technical Notes
- Park catalog remains local/static for MVP.
- Location retrieval runs through Pebble JS companion.
- Cache last confirmed park for quick startup.
