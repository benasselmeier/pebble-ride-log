# Pebble Ride Logger

Pebble Ride Logger is a native Pebble smartwatch application for recording roller coaster and amusement ride sessions. The watch app samples the accelerometer throughout a ride, calculates peak and average G-forces, and stores a history of multiple ride summaries directly on the watch. When the watch reconnects with the phone companion, the data is synchronised automatically using AppMessage so it can be exported by a companion app or a cloud service.

## Features

- Start/stop ride logging directly on the watch using the Select button.
- Real-time display of elapsed time, sample count, and live/max/min G-force values during a ride.
- History browser (Up/Down buttons) that shows start/end times, duration, G-force stats, and the strongest readings on each axis.
- Automatic and manual (long-press Select) synchronisation of unsent ride summaries once a phone connection is available.
- Persistent on-watch storage for up to eight recent rides so stats remain available offline.

## Controls

| Control | Action |
| --- | --- |
| Select | Start or stop a ride recording |
| Select (long press) | Retry syncing pending rides |
| Up / Down | Browse the stored ride summaries |

While logging, the screen shows the live G-force snapshot together with the current peak axis readings. Completed rides are marked with an asterisk in the history until they successfully sync.

## Sync protocol

Ride summaries are sent over AppMessage with the keys defined in `package.json`. Each message contains:

- `StartTime`/`EndTime` (UNIX timestamps)
- `Duration` (seconds)
- `SampleCount`
- `MaxG`, `MinG`, `AvgG` (values in milli-g)
- `PeakX`, `PeakY`, `PeakZ` (axis peaks in milli-g)

When an outbox send succeeds the watch marks the ride as synced, so a phone-side app only needs to acknowledge receipt by allowing the message to reach the inbox.

## Building

The project follows the standard Pebble SDK layout and can be built with the Pebble command line tools:

```sh
pebble build
pebble install --phone <phone_ip>
```

This repository does not include the Pebble SDK; install it separately according to the official documentation.

## Future enhancements

- Phone companion app that captures the AppMessage payloads and forwards them to a logging service.
- Graphing of G-force trends on the phone based on the samples collected during each ride.
- Configurable sampling rate and ride metadata entry (ride name, park, seat).
