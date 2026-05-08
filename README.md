# Pebble Ride Log

A Pebble app (and eventually companion watchface) for logging roller coaster and amusement ride forces.

## Planning

Product planning is now documented as user stories in [`docs/user-stories`](./docs/user-stories/00-overview.md).

Start there for:
- story order
- acceptance criteria
- MVP scope boundaries
- implementation notes

## Current App State

The codebase currently contains an early Pebble app skeleton with a menu and placeholder interactions in `src/c/main.c`.

## Development Notes

- Primary target: Pebble C app.
- Companion JS is expected for location lookup and park suggestion.
- Keep interactions short and button-driven for ride queue usage.
