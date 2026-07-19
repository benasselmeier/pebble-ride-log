// Configuration interface for Forces Recorder
// This creates the settings page that opens in the Pebble mobile app

module.exports = [
  {
    "type": "heading",
    "defaultValue": "Forces Recorder Settings",
    "size": 1
  },
  {
    "type": "text",
    "defaultValue": "Configure your roller coaster force recording preferences and view your ride history."
  },
  {
    "type": "section",
    "items": [
      {
        "type": "heading",
        "defaultValue": "Recording Settings",
        "size": 3
      },
      {
        "type": "select",
        "messageKey": "RECORDING_MODE",
        "defaultValue": "auto",
        "label": "Recording Mode",
        "description": "Choose how recordings are triggered",
        "options": [
          { 
            "label": "Manual Start/Stop", 
            "value": "manual" 
          },
          { 
            "label": "Auto-detect Motion", 
            "value": "auto" 
          },
          { 
            "label": "Quick Start (Immediate)", 
            "value": "quick" 
          }
        ]
      },
      {
        "type": "slider",
        "messageKey": "G_FORCE_THRESHOLD",
        "defaultValue": 150,
        "label": "G-Force Alert Threshold",
        "description": "Alert when G-forces exceed this value (in hundredths, 150 = 1.50g)",
        "min": 100,
        "max": 500,
        "step": 10
      },
      {
        "type": "toggle",
        "messageKey": "VIBRATE_ON_PEAKS",
        "defaultValue": true,
        "label": "Vibrate on G-Force Peaks",
        "description": "Vibrate when hitting max/min G-forces during recording"
      },
      {
        "type": "toggle",
        "messageKey": "SOUND_ALERTS",
        "defaultValue": false,
        "label": "Sound Alerts",
        "description": "Play sounds for recording events (if supported)"
      }
    ]
  },
  {
    "type": "section",
    "items": [
      {
        "type": "heading",
        "defaultValue": "Display Settings",
        "size": 3
      },
      {
        "type": "select",
        "messageKey": "TIME_FORMAT",
        "defaultValue": "mmss",
        "label": "Timer Display Format",
        "description": "How to display elapsed time during recording",
        "options": [
          { 
            "label": "MM:SS (Minutes:Seconds)", 
            "value": "mmss" 
          },
          { 
            "label": "MM:SS.CC (With Centiseconds)", 
            "value": "mmsscc" 
          },
          { 
            "label": "SS.CC (Seconds.Centiseconds)", 
            "value": "sscc" 
          }
        ]
      },
      {
        "type": "select",
        "messageKey": "G_FORCE_UNITS",
        "defaultValue": "g",
        "label": "G-Force Units",
        "description": "Display units for acceleration forces",
        "options": [
          { 
            "label": "G-forces (g)", 
            "value": "g" 
          },
          { 
            "label": "Meters/sec² (m/s²)", 
            "value": "ms2" 
          },
          { 
            "label": "Feet/sec² (ft/s²)", 
            "value": "fts2" 
          }
        ]
      },
      {
        "type": "toggle",
        "messageKey": "SHOW_CURRENT_G",
        "defaultValue": true,
        "label": "Show Current G-Force",
        "description": "Display real-time G-force during recording"
      }
    ]
  },
  {
    "type": "section",
    "items": [
      {
        "type": "heading",
        "defaultValue": "Data Management",
        "size": 3
      },
      {
        "type": "slider",
        "messageKey": "MAX_STORED_RIDES",
        "defaultValue": 20,
        "label": "Maximum Stored Rides",
        "description": "Number of ride recordings to keep in memory",
        "min": 5,
        "max": 50,
        "step": 5
      },
      {
        "type": "toggle",
        "messageKey": "AUTO_SAVE",
        "defaultValue": true,
        "label": "Auto-Save Recordings",
        "description": "Automatically save completed ride recordings"
      },
      {
        "type": "button",
        "primary": false,
        "defaultValue": "View Ride History",
        "description": "Open detailed view of your recorded rides",
        "messageKey": "VIEW_HISTORY"
      },
      {
        "type": "button",
        "primary": false,
        "defaultValue": "Export Data",
        "description": "Export your ride data as CSV or JSON",
        "messageKey": "EXPORT_DATA"
      },
      {
        "type": "button",
        "primary": true,
        "defaultValue": "Clear All Data",
        "description": "⚠️ Delete all stored ride recordings",
        "messageKey": "CLEAR_DATA"
      }
    ]
  },
  {
    "type": "section",
    "items": [
      {
        "type": "heading",
        "defaultValue": "Coaster Management",
        "size": 3
      },
      {
        "type": "text",
        "defaultValue": "Add custom coasters to parks for more accurate ride tracking. Your additions will be saved to your device and synced with the watch."
      },
      {
        "type": "select",
        "messageKey": "SELECTED_PARK",
        "defaultValue": "",
        "label": "Select Park",
        "description": "Choose a park to add coasters to",
        "options": [
          { "label": "-- Select a Park --", "value": "" },
          { "label": "Six Flags St. Louis", "value": "sixflags_stlouis" },
          { "label": "Six Flags Magic Mountain", "value": "sixflags_mm" },
          { "label": "Six Flags Great Adventure", "value": "sixflags_ga" },
          { "label": "Six Flags Fiesta Texas", "value": "sixflags_fiesta" },
          { "label": "Six Flags Over Texas", "value": "sixflags_texas" },
          { "label": "Cedar Point", "value": "cedar_point" },
          { "label": "Kings Island", "value": "kings_island" },
          { "label": "Canada's Wonderland", "value": "canadas_wonderland" },
          { "label": "Carowinds", "value": "carowinds" },
          { "label": "Knott's Berry Farm", "value": "knotts" },
          { "label": "Silver Dollar City", "value": "silver_dollar" },
          { "label": "Dollywood", "value": "dollywood" },
          { "label": "Magic Kingdom", "value": "magic_kingdom" },
          { "label": "EPCOT", "value": "epcot" },
          { "label": "Hollywood Studios", "value": "hollywood_studios" },
          { "label": "Animal Kingdom", "value": "animal_kingdom" },
          { "label": "Disneyland", "value": "disneyland" },
          { "label": "Disney California Adventure", "value": "dca" },
          { "label": "Universal Studios Florida", "value": "universal_florida" },
          { "label": "Islands of Adventure", "value": "islands_adventure" },
          { "label": "Universal Studios Hollywood", "value": "universal_hollywood" },
          { "label": "Busch Gardens Williamsburg", "value": "busch_williamsburg" },
          { "label": "Busch Gardens Tampa", "value": "busch_tampa" },
          { "label": "SeaWorld Orlando", "value": "seaworld_orlando" },
          { "label": "Hersheypark", "value": "hersheypark" },
          { "label": "Kennywood", "value": "kennywood" },
          { "label": "Local/Other Park", "value": "other_park" }
        ]
      },
      {
        "type": "input",
        "messageKey": "CUSTOM_PARK_NAME",
        "defaultValue": "",
        "label": "Custom Park Name",
        "description": "If you selected 'Local/Other Park', enter the park name here",
        "attributes": {
          "placeholder": "e.g., Local Carnival, State Fair, Regional Park",
          "maxlength": 50
        }
      },
      {
        "type": "input",
        "messageKey": "NEW_COASTER_NAME",
        "defaultValue": "",
        "label": "New Coaster Name",
        "description": "Enter the name of the coaster you want to add",
        "attributes": {
          "placeholder": "e.g., Steel Vengeance, The Beast, Space Mountain",
          "maxlength": 50
        }
      },
      {
        "type": "select",
        "messageKey": "NEW_COASTER_TYPE",
        "defaultValue": "steel",
        "label": "Coaster Type",
        "description": "Select the type/material of the coaster",
        "options": [
          { "label": "Steel Coaster", "value": "steel" },
          { "label": "Wood Coaster", "value": "wood" },
          { "label": "Hybrid (Steel track, Wood structure)", "value": "hybrid" },
          { "label": "Launched Steel", "value": "launched_steel" },
          { "label": "Inverted Steel", "value": "inverted" },
          { "label": "Flying Coaster", "value": "flying" },
          { "label": "Spinning Coaster", "value": "spinning" },
          { "label": "Mine Train", "value": "mine_train" },
          { "label": "Bobsled/Alpine Coaster", "value": "bobsled" },
          { "label": "Water Coaster", "value": "water" },
          { "label": "Dark Ride Coaster", "value": "dark_ride" },
          { "label": "Other/Unknown", "value": "other" }
        ]
      },
      {
        "type": "button",
        "primary": true,
        "defaultValue": "Add Coaster",
        "description": "Add this coaster to the selected park",
        "messageKey": "ADD_COASTER"
      },
      {
        "type": "button",
        "primary": false,
        "defaultValue": "View My Custom Coasters",
        "description": "See all coasters you've added to parks",
        "messageKey": "VIEW_PARK_COASTERS"
      },
      {
        "type": "button",
        "primary": false,
        "defaultValue": "Clear Custom Coasters",
        "description": "⚠️ Remove all custom coasters you've added",
        "messageKey": "CLEAR_CUSTOM_COASTERS"
      }
    ]
  },
  {
    "type": "section",
    "items": [
      {
        "type": "heading",
        "defaultValue": "About",
        "size": 3
      },
      {
        "type": "text",
        "defaultValue": "Forces Recorder v1.0.0"
      },
      {
        "type": "text",
        "defaultValue": "Record and analyze G-forces during roller coaster rides. Compatible with all Pebble models."
      },
      {
        "type": "text",
        "defaultValue": "Created by Ben Asselmeier • © 2025"
      }
    ]
  },
  {
    "type": "submit",
    "defaultValue": "Save Settings"
  }
];
