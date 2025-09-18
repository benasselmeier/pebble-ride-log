# Pebble Forces Recorder

A lightweight application for Pebble smartwatches to record amusement ride forces, similar to LogRide or RideForces apps.

## Features

- **Real-time force measurement** using the Pebble's accelerometer
- **Maximum force tracking** during rides
- **Recording duration** tracking
- **Offline functionality** - works without phone connection
- **Simple menu interface** for easy operation during rides
- **Vibration feedback** for start/stop recording

## How to Use

1. **Start Recording**: Select "Start Recording" from the main menu before getting on a ride
2. **During the Ride**: The watch will continuously measure and record forces
3. **Stop Recording**: Select "Stop Recording" after the ride ends
4. **View Results**: Check "Current Force" and "Max Force" in the menu

## Controls

- **UP/DOWN buttons**: Navigate menu
- **SELECT button**: Choose menu option
- **BACK button**: Exit app

## Menu Options

- **Start/Stop Recording**: Begin or end force measurement
- **Current Force**: Shows real-time acceleration in g-forces
- **Max Force**: Shows maximum force recorded during current session
```
pebble-forces-recorder
├── src
│   ├── c
│   │   ├── main.c          # Entry point of the application
│   │   ├── accelerometer.c  # Accelerometer data handling
│   │   ├── data_storage.c   # Data storage management
│   │   └── ui.c            # User interface management
│   └── js
│       └── app.js          # JavaScript for additional functionality
├── resources
│   ├── fonts               # Font files for UI
│   └── images              # Image files for icons and backgrounds
├── appinfo.json           # Application metadata
├── package.json            # npm configuration
├── wscript                 # Build script
└── README.md               # Project documentation
```

## Setup Instructions
1. Clone the repository:
   ```
   git clone https://github.com/yourusername/pebble-forces-recorder.git
   ```
2. Navigate to the project directory:
   ```
   cd pebble-forces-recorder
   ```
3. Install dependencies:
   ```
   npm install
   ```
4. Build the project:
   ```
   wscript build
   ```

## Usage
- Launch the application on your Pebble smartwatch.
- Use the UI to start recording forces during your amusement rides.
- Review recorded data directly on the watch.

## Contributing
Contributions are welcome! Please submit a pull request or open an issue for any suggestions or improvements.

## License
This project is licensed under the MIT License. See the LICENSE file for details.