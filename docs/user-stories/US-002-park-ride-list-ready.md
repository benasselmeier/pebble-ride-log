# US-002: Park Ride List Ready

## User Story
As a rider who selected a park,
I want the app to show rides for that park,
so I can quickly choose what I am about to ride.

## Acceptance Criteria
- App stores a `current_park_id` after user selection/confirmation.
- Selecting `New Ride` opens a list containing only rides for the current park.
- Rides are shown with clear, short names for small screens.
- If no park is set, app routes user to park selection flow.

## Out of Scope (v1)
- Live ride status (open/closed/wait times).
- Dynamic ride list downloads.

## Technical Notes
- Seed with a small static dataset first.
- Add `Quick Start` shortcut to preselect last ridden coaster.
