# Pebble Ride Log

A Pebble app (and eventually a companion watchface) for logging ride forces at amusement parks.

This repository currently contains an early app skeleton with a basic menu and vibration feedback. The goal is to evolve it into a fast, ride-day workflow with park-aware ride start/stop logging and at-a-glance daily stats.

## Product Vision (Target Experience)

1. You arrive at a park and open the app.
2. The app uses your location to suggest the nearest park, then asks you to confirm.
3. The app loads that park's coaster list.
4. You start a new ride log in a few clicks.
5. You stop after the ride.
6. The app shows a quick summary (max +G, min -G, airtime estimate, acceleration highlights).
7. A watchface shows "today so far" stats throughout the day.

---

## Incremental Roadmap

### Phase 0 — Foundation cleanup (current sprint)
**Goal:** Make the app codebase easier to extend before adding new features.

- [ ] Split `main.c` into focused modules:
  - `ui_menu.c` / `ui_menu.h`
  - `ride_session.c` / `ride_session.h`
  - `sensors.c` / `sensors.h`
  - `storage.c` / `storage.h`
- [ ] Define shared models:
  - `Park`
  - `Ride`
  - `RideSession`
  - `DailyStats`
- [ ] Add a lightweight app state machine:
  - `STATE_IDLE`
  - `STATE_PARK_SELECTED`
  - `STATE_RECORDING`
  - `STATE_SUMMARY`

### Phase 1 — Ride recording MVP
**Goal:** Capture meaningful force data for one ride with clear start/stop UX.

- [ ] Start/stop ride recording from menu.
- [ ] Sample accelerometer at a fixed interval.
- [ ] Compute per-ride core metrics:
  - peak positive G
  - peak negative G
  - duration
  - simple roughness/jerk proxy (optional)
- [ ] Show a post-ride summary screen.
- [ ] Persist latest session to local storage.

### Phase 2 — Park mode and ride catalog
**Goal:** Fast ride selection in a specific park.

- [ ] Add a static local park + ride catalog (small seed dataset first).
- [ ] Add "Set Current Park" flow.
- [ ] In park mode, "New Ride" should open rides for that park.
- [ ] Add "Quick Start" behavior:
  - remember last ride
  - one-click restart for repeat laps

### Phase 3 — Location-assisted park suggestion
**Goal:** Suggest the nearest park and confirm with user.

- [ ] Add phone companion JS messaging for location lookup.
- [ ] Compute nearest park from park coordinates.
- [ ] Prompt: "Use <Park Name>?"
- [ ] Fallback cleanly when phone/location unavailable.

### Phase 4 — Daily stats + watchface MVP
**Goal:** Show live "today's best" ride stats on a watchface.

- [ ] Create a simple watchface target (time + 2 to 4 stats).
- [ ] Aggregate day-level metrics from ride sessions:
  - highest +G
  - lowest -G
  - ride with most airtime
  - ride with highest acceleration rate
- [ ] Refresh watchface after each completed ride.
- [ ] Keep visuals intentionally minimal for v1.

### Phase 5 — Polish and reliability
**Goal:** Improve trust, battery behavior, and usability.

- [ ] Data validation and guardrails (bad samples, accidental stops).
- [ ] Better summary visuals and labels.
- [ ] Export/share options (later: phone sync/cloud backup).
- [ ] Battery/performance tuning and sampling strategy validation.

---

## Suggested First Build Slices

If you want to move quickly without overbuilding, implement these slices in order:

1. **Slice A:** Record one unnamed ride and show summary.
2. **Slice B:** Add named ride selection from one hardcoded park.
3. **Slice C:** Persist multiple sessions and compute "today" aggregate stats.
4. **Slice D:** Build simple stats watchface reading daily aggregate.
5. **Slice E:** Add location-based park suggestion.

This keeps each milestone testable in the Pebble emulator and on-device.

---

## Data Model Sketch

These structs can evolve, but this shape should support app + watchface goals:

- `RideSession`
  - `park_id`
  - `ride_id`
  - `start_time`
  - `end_time`
  - `max_pos_g`
  - `max_neg_g`
  - `airtime_ms_estimate`
  - `max_accel_rate`
- `DailyStats`
  - `date_key`
  - `best_pos_g`
  - `best_neg_g`
  - `ride_id_most_airtime`
  - `ride_id_highest_accel`

---

## Immediate Next Task (recommended)

Implement **Phase 1 (Ride recording MVP)** before park/location logic.

Why:
- It validates sensor pipeline and metrics first.
- It gives real data to drive watchface design.
- It avoids building selection/location UX on top of unproven recording.

---

## Development Notes

- Target language: C (Pebble SDK).
- Use Pebble Cloud for emulator iteration and debugging.
- Keep interactions short and button-driven for in-queue usage.
- Prefer conservative defaults and obvious confirmations while recording.
