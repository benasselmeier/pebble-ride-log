// PebbleKit JS for Forces Recorder
// Handles configuration interface and data communication

var Clay = require('pebble-clay');
var clayConfig = require('./config');
var clay = new Clay(clayConfig);

// Initialize the app
Pebble.addEventListener('ready', function() {
  console.log('Forces Recorder PebbleKit JS ready!');
  
  // Send initial settings to watch if they exist
  var settings = JSON.parse(localStorage.getItem('clay-settings')) || {};
  if (Object.keys(settings).length > 0) {
    sendSettingsToWatch(settings);
  }
});

// Handle messages from the watch
Pebble.addEventListener('appmessage', function(e) {
  console.log('Received message from watch:', e.payload);
  
  var payload = e.payload;
  
  // Handle different message types
  if (payload.REQUEST_SETTINGS) {
    // Watch is requesting current settings
    var settings = JSON.parse(localStorage.getItem('clay-settings')) || {};
    sendSettingsToWatch(settings);
  }
  
  if (payload.REQUEST_EXPORT) {
    // Watch is requesting data export
    handleDataExport(payload);
  }
  
  if (payload.RIDE_DATA) {
    // Watch is sending ride data for web storage/export
    handleRideData(payload);
  }
});

// Handle configuration changes
Pebble.addEventListener('webviewclosed', function(e) {
  if (e && !e.response) {
    return;
  }
  
  try {
    var claySettings = clay.getSettings(e.response);
    console.log('New settings:', claySettings);
    
    // Save settings locally
    localStorage.setItem('clay-settings', JSON.stringify(claySettings));
    
    // Handle special button actions
    handleSpecialActions(claySettings);
    
    // Send settings to watch
    sendSettingsToWatch(claySettings);
    
  } catch (error) {
    console.error('Error processing settings:', error);
  }
});

// Send settings to the watch
function sendSettingsToWatch(settings) {
  var message = {
    'SETTINGS_DATA': 1,
    'RECORDING_MODE': encodeSettingValue(settings.RECORDING_MODE, 'manual'),
    'G_FORCE_THRESHOLD': parseInt(settings.G_FORCE_THRESHOLD) || 150,
    'VIBRATE_ON_PEAKS': settings.VIBRATE_ON_PEAKS ? 1 : 0,
    'SOUND_ALERTS': settings.SOUND_ALERTS ? 1 : 0,
    'TIME_FORMAT': encodeTimeFormat(settings.TIME_FORMAT),
    'G_FORCE_UNITS': encodeUnits(settings.G_FORCE_UNITS),
    'SHOW_CURRENT_G': settings.SHOW_CURRENT_G ? 1 : 0,
    'MAX_STORED_RIDES': parseInt(settings.MAX_STORED_RIDES) || 20,
    'AUTO_SAVE': settings.AUTO_SAVE ? 1 : 0
  };
  
  console.log('Sending settings to watch:', message);
  
  Pebble.sendAppMessage(message, 
    function() {
      console.log('Settings sent successfully');
    },
    function(error) {
      console.log('Error sending settings:', error);
    }
  );
}

// Handle special button actions
function handleSpecialActions(settings) {
  if (settings.VIEW_HISTORY) {
    openRideHistoryPage();
  }
  
  if (settings.EXPORT_DATA) {
    exportRideData();
  }
  
  if (settings.CLEAR_DATA) {
    if (confirm('Are you sure you want to delete all ride recordings? This cannot be undone.')) {
      clearAllData();
    }
  }
  
  if (settings.ADD_COASTER) {
    addCoasterToPark(settings);
  }
  
  if (settings.VIEW_PARK_COASTERS) {
    viewAllCustomCoasters();
  }
  
  if (settings.CLEAR_CUSTOM_COASTERS) {
    if (confirm('Are you sure you want to delete all custom coasters? This cannot be undone.')) {
      clearCustomCoasters();
    }
  }
  
  if (settings.CUSTOM_PARK_NAME && settings.CUSTOM_PARK_NAME.trim() && settings.SELECTED_PARK === 'other_park') {
    // Handle custom park name only when "Local/Other Park" is selected
    addCustomPark(settings.CUSTOM_PARK_NAME.trim(), settings);
  }
}

// Encode setting values for transmission
function encodeSettingValue(value, defaultValue) {
  var encodings = {
    'manual': 0,
    'auto': 1,
    'quick': 2
  };
  return encodings[value] !== undefined ? encodings[value] : encodings[defaultValue];
}

function encodeTimeFormat(value) {
  var encodings = {
    'mmss': 0,
    'mmsscc': 1,
    'sscc': 2
  };
  return encodings[value] || 0;
}

function encodeUnits(value) {
  var encodings = {
    'g': 0,
    'ms2': 1,
    'fts2': 2
  };
  return encodings[value] || 0;
}

// Handle ride data from watch
function handleRideData(payload) {
  console.log('Received ride data:', payload);
  
  // Store ride data in local storage for web interface
  var rideHistory = JSON.parse(localStorage.getItem('ride-history')) || [];
  
  var rideRecord = {
    timestamp: Date.now(),
    park_name: payload.PARK_NAME || 'Unknown Park',
    coaster_name: payload.COASTER_NAME || 'Unknown Coaster',
    duration_seconds: payload.DURATION || 0,
    max_g_force: (payload.MAX_G_FORCE || 100) / 100.0,
    min_g_force: (payload.MIN_G_FORCE || 100) / 100.0,
    device_type: payload.DEVICE_TYPE || 'Unknown'
  };
  
  rideHistory.push(rideRecord);
  
  // Limit stored rides
  var maxRides = 100; // Store more on phone than on watch
  if (rideHistory.length > maxRides) {
    rideHistory = rideHistory.slice(-maxRides);
  }
  
  localStorage.setItem('ride-history', JSON.stringify(rideHistory));
}

// Open ride history page
function openRideHistoryPage() {
  var rideHistory = JSON.parse(localStorage.getItem('ride-history')) || [];
  
  if (rideHistory.length === 0) {
    alert('No ride recordings found. Start recording some rides first!');
    return;
  }
  
  // Create a simple HTML page showing ride history
  var historyHtml = generateRideHistoryHtml(rideHistory);
  var historyWindow = window.open('', 'RideHistory', 'width=400,height=600,scrollbars=yes');
  historyWindow.document.write(historyHtml);
  historyWindow.document.close();
}

// Generate HTML for ride history
function generateRideHistoryHtml(rideHistory) {
  var html = `
    <!DOCTYPE html>
    <html>
    <head>
      <title>Forces Recorder - Ride History</title>
      <meta name="viewport" content="width=device-width, initial-scale=1">
      <style>
        body { font-family: Arial, sans-serif; margin: 20px; background: #f5f5f5; }
        .ride { background: white; margin: 10px 0; padding: 15px; border-radius: 8px; box-shadow: 0 2px 4px rgba(0,0,0,0.1); }
        .ride-header { font-weight: bold; color: #2196F3; margin-bottom: 5px; }
        .ride-details { color: #666; font-size: 14px; }
        .g-forces { margin: 8px 0; }
        .max-g { color: #f44336; font-weight: bold; }
        .min-g { color: #2196F3; font-weight: bold; }
        .stats { background: #e3f2fd; padding: 10px; border-radius: 4px; margin-top: 20px; }
        h1 { color: #1976D2; text-align: center; }
        .export-btn { background: #4CAF50; color: white; padding: 10px 20px; border: none; border-radius: 4px; cursor: pointer; margin: 10px 5px; }
      </style>
    </head>
    <body>
      <h1>🎢 Forces Recorder - Ride History</h1>
      <button class="export-btn" onclick="exportCSV()">Export as CSV</button>
      <button class="export-btn" onclick="exportJSON()">Export as JSON</button>
  `;
  
  // Add ride statistics
  var totalRides = rideHistory.length;
  var totalTime = rideHistory.reduce((sum, ride) => sum + ride.duration_seconds, 0);
  var maxGForce = Math.max(...rideHistory.map(ride => ride.max_g_force));
  var minGForce = Math.min(...rideHistory.map(ride => ride.min_g_force));
  
  html += `
    <div class="stats">
      <strong>Statistics:</strong><br>
      Total Rides: ${totalRides}<br>
      Total Time: ${Math.floor(totalTime / 60)}m ${totalTime % 60}s<br>
      Highest G-Force: ${maxGForce.toFixed(2)}g<br>
      Lowest G-Force: ${minGForce.toFixed(2)}g
    </div>
  `;
  
  // Add individual rides
  rideHistory.reverse().forEach(function(ride) {
    var date = new Date(ride.timestamp).toLocaleDateString();
    var time = new Date(ride.timestamp).toLocaleTimeString();
    var duration = Math.floor(ride.duration_seconds / 60) + 'm ' + (ride.duration_seconds % 60) + 's';
    
    html += `
      <div class="ride">
        <div class="ride-header">${ride.coaster_name}</div>
        <div class="ride-details">${ride.park_name} • ${date} ${time}</div>
        <div class="ride-details">Duration: ${duration}</div>
        <div class="g-forces">
          <span class="max-g">Max: ${ride.max_g_force.toFixed(2)}g</span> • 
          <span class="min-g">Min: ${ride.min_g_force.toFixed(2)}g</span>
        </div>
      </div>
    `;
  });
  
  html += `
    <script>
      function exportCSV() {
        var csv = 'Date,Time,Park,Coaster,Duration(s),Max G-Force,Min G-Force\\n';
        ${JSON.stringify(rideHistory)}.forEach(function(ride) {
          var date = new Date(ride.timestamp);
          csv += date.toLocaleDateString() + ',' + 
                 date.toLocaleTimeString() + ',' +
                 '"' + ride.park_name + '",' +
                 '"' + ride.coaster_name + '",' +
                 ride.duration_seconds + ',' +
                 ride.max_g_force.toFixed(2) + ',' +
                 ride.min_g_force.toFixed(2) + '\\n';
        });
        
        var blob = new Blob([csv], { type: 'text/csv' });
        var url = window.URL.createObjectURL(blob);
        var a = document.createElement('a');
        a.href = url;
        a.download = 'forces-recorder-history.csv';
        a.click();
        window.URL.revokeObjectURL(url);
      }
      
      function exportJSON() {
        var data = JSON.stringify(${JSON.stringify(rideHistory)}, null, 2);
        var blob = new Blob([data], { type: 'application/json' });
        var url = window.URL.createObjectURL(blob);
        var a = document.createElement('a');
        a.href = url;
        a.download = 'forces-recorder-history.json';
        a.click();
        window.URL.revokeObjectURL(url);
      }
    </script>
    </body>
    </html>
  `;
  
  return html;
}

// Export ride data
function exportRideData() {
  var rideHistory = JSON.parse(localStorage.getItem('ride-history')) || [];
  
  if (rideHistory.length === 0) {
    alert('No ride data to export. Record some rides first!');
    return;
  }
  
  // Create CSV format
  var csv = 'Date,Time,Park,Coaster,Duration(s),Max G-Force,Min G-Force\n';
  rideHistory.forEach(function(ride) {
    var date = new Date(ride.timestamp);
    csv += date.toLocaleDateString() + ',' + 
           date.toLocaleTimeString() + ',' +
           '"' + ride.park_name + '",' +
           '"' + ride.coaster_name + '",' +
           ride.duration_seconds + ',' +
           ride.max_g_force.toFixed(2) + ',' +
           ride.min_g_force.toFixed(2) + '\n';
  });
  
  console.log('Ride data CSV:', csv);
  alert('Ride data exported to console. Check developer tools or use the View History page for download options.');
}

// Clear all ride data
function clearAllData() {
  localStorage.removeItem('ride-history');
  
  // Send message to watch to clear its data too
  Pebble.sendAppMessage({'CLEAR_WATCH_DATA': 1}, 
    function() {
      console.log('Clear data command sent to watch');
      alert('All ride data has been cleared.');
    },
    function(error) {
      console.log('Error clearing watch data:', error);
    }
  );
}

// Clear all custom coasters
function clearCustomCoasters() {
  localStorage.removeItem('custom-park-coasters');
  
  // Send message to watch to clear custom coaster data
  Pebble.sendAppMessage({'CLEAR_CUSTOM_COASTERS': 1}, 
    function() {
      console.log('Clear custom coasters command sent to watch');
      alert('All custom coasters have been cleared.');
    },
    function(error) {
      console.log('Error clearing custom coasters on watch:', error);
      alert('Custom coasters cleared from phone. Watch data may need manual clearing.');
    }
  );
}

// Add coaster to selected park
function addCoasterToPark(settings) {
  var selectedPark = settings.SELECTED_PARK;
  var coasterName = settings.NEW_COASTER_NAME;
  var coasterType = settings.NEW_COASTER_TYPE;
  var customParkName = settings.CUSTOM_PARK_NAME;
  
  if (!selectedPark || selectedPark === '') {
    alert('Please select a park first before adding a coaster.');
    return;
  }
  
  if (!coasterName || coasterName.trim() === '') {
    alert('Please enter a coaster name.');
    return;
  }
  
  // Handle custom park name for "other_park" selection
  var parkKey = selectedPark;
  var displayName = getParkDisplayName(selectedPark);
  
  if (selectedPark === 'other_park') {
    if (!customParkName || customParkName.trim() === '') {
      alert('Please enter a custom park name when selecting "Local/Other Park".');
      return;
    }
    parkKey = 'custom_' + customParkName.trim().toLowerCase().replace(/[^a-z0-9]/g, '_');
    displayName = customParkName.trim();
  }
  
  // Get existing park coasters from storage
  var parkCoasters = JSON.parse(localStorage.getItem('custom-park-coasters')) || {};
  
  if (!parkCoasters[parkKey]) {
    parkCoasters[parkKey] = [];
  }
  
  // Check if coaster already exists
  var existingCoaster = parkCoasters[parkKey].find(function(coaster) {
    return coaster.name.toLowerCase() === coasterName.trim().toLowerCase();
  });
  
  if (existingCoaster) {
    alert('A coaster named "' + coasterName.trim() + '" already exists in ' + displayName + '.');
    return;
  }
  
  // Add new coaster
  var newCoaster = {
    name: coasterName.trim(),
    type: formatCoasterType(coasterType),
    parkKey: parkKey,
    parkName: displayName,
    id: Date.now(), // Simple ID generation
    added_date: new Date().toISOString()
  };
  
  parkCoasters[parkKey].push(newCoaster);
  
  // Save to storage
  localStorage.setItem('custom-park-coasters', JSON.stringify(parkCoasters));
  
  // Send to watch
  sendCustomCoastersToWatch(parkKey, parkCoasters[parkKey]);
  
  alert('Successfully added "' + coasterName.trim() + '" (' + formatCoasterType(coasterType) + ') to ' + displayName + '!\n\nTotal custom coasters for this park: ' + parkCoasters[parkKey].length);
}

// View all custom coasters
function viewAllCustomCoasters() {
  var parkCoasters = JSON.parse(localStorage.getItem('custom-park-coasters')) || {};
  var allCoasters = [];
  
  // Collect all coasters from all parks
  Object.keys(parkCoasters).forEach(function(parkKey) {
    parkCoasters[parkKey].forEach(function(coaster) {
      var coasterCopy = {
        name: coaster.name,
        type: coaster.type,
        id: coaster.id,
        added_date: coaster.added_date,
        parkKey: parkKey,
        parkName: coaster.parkName || getParkDisplayName(parkKey)
      };
      allCoasters.push(coasterCopy);
    });
  });
  
  if (allCoasters.length === 0) {
    alert('No custom coasters found. Add some coasters first using the form above!');
    return;
  }
  
  // Sort by park name, then by coaster name
  allCoasters.sort(function(a, b) {
    if (a.parkName !== b.parkName) {
      return a.parkName.localeCompare(b.parkName);
    }
    return a.name.localeCompare(b.name);
  });
  
  // Create HTML page showing all custom coasters
  var coastersHtml = generateAllCoastersHtml(allCoasters);
  var coastersWindow = window.open('', 'AllCustomCoasters', 'width=500,height=700,scrollbars=yes');
  coastersWindow.document.write(coastersHtml);
  coastersWindow.document.close();
}

// Generate HTML for all custom coasters view
function generateAllCoastersHtml(allCoasters) {
  var html = `
    <!DOCTYPE html>
    <html>
    <head>
      <title>Forces Recorder - My Custom Coasters</title>
      <meta name="viewport" content="width=device-width, initial-scale=1">
      <style>
        body { font-family: Arial, sans-serif; margin: 20px; background: #f5f5f5; }
        .park-section { background: white; margin: 15px 0; padding: 20px; border-radius: 8px; box-shadow: 0 2px 4px rgba(0,0,0,0.1); }
        .park-name { font-weight: bold; color: #1976D2; margin-bottom: 15px; font-size: 18px; border-bottom: 2px solid #e3f2fd; padding-bottom: 10px; }
        .coaster { background: #f8f9fa; margin: 8px 0; padding: 12px; border-radius: 6px; border-left: 4px solid #2196F3; }
        .coaster-name { font-weight: bold; color: #333; margin-bottom: 4px; font-size: 15px; }
        .coaster-type { color: #666; font-size: 13px; margin-bottom: 4px; }
        .coaster-added { color: #999; font-size: 11px; }
        .stats { background: #e3f2fd; padding: 15px; border-radius: 6px; margin-bottom: 20px; }
        h1 { color: #1976D2; text-align: center; margin-bottom: 20px; }
        .type-steel { border-left-color: #607D8B; }
        .type-wood { border-left-color: #8D6E63; }
        .type-hybrid { border-left-color: #FF9800; }
        .type-launched { border-left-color: #F44336; }
        .type-inverted { border-left-color: #9C27B0; }
        .type-flying { border-left-color: #2196F3; }
        .type-other { border-left-color: #795548; }
        .export-btn { background: #4CAF50; color: white; padding: 8px 15px; border: none; border-radius: 4px; cursor: pointer; margin: 5px; }
        .clear-btn { background: #f44336; color: white; padding: 8px 15px; border: none; border-radius: 4px; cursor: pointer; margin: 5px; }
      </style>
    </head>
    <body>
      <h1>🎢 My Custom Coasters</h1>
  `;
  
  // Add statistics
  var totalCoasters = allCoasters.length;
  var parkCount = new Set(allCoasters.map(c => c.parkKey)).size;
  var typeCount = {};
  
  allCoasters.forEach(function(coaster) {
    var type = coaster.type.split(' ')[0].toLowerCase();
    typeCount[type] = (typeCount[type] || 0) + 1;
  });
  
  var typeStats = Object.keys(typeCount).map(function(type) {
    return type.charAt(0).toUpperCase() + type.slice(1) + ': ' + typeCount[type];
  }).join(' • ');
  
  html += `
    <div class="stats">
      <strong>📊 Summary:</strong><br>
      Total Custom Coasters: ${totalCoasters}<br>
      Parks with Custom Coasters: ${parkCount}<br>
      Types: ${typeStats || 'None'}
    </div>
  `;
  
  // Group coasters by park
  var coastersByPark = {};
  allCoasters.forEach(function(coaster) {
    if (!coastersByPark[coaster.parkKey]) {
      coastersByPark[coaster.parkKey] = {
        name: coaster.parkName,
        coasters: []
      };
    }
    coastersByPark[coaster.parkKey].coasters.push(coaster);
  });
  
  // Add park sections
  Object.keys(coastersByPark).forEach(function(parkKey) {
    var park = coastersByPark[parkKey];
    html += `
      <div class="park-section">
        <div class="park-name">🎡 ${park.name} (${park.coasters.length} coasters)</div>
    `;
    
    park.coasters.forEach(function(coaster) {
      var addedDate = new Date(coaster.added_date).toLocaleDateString();
      var typeClass = 'type-' + coaster.type.split(' ')[0].toLowerCase();
      
      html += `
        <div class="coaster ${typeClass}">
          <div class="coaster-name">${coaster.name}</div>
          <div class="coaster-type">${coaster.type}</div>
          <div class="coaster-added">Added: ${addedDate}</div>
        </div>
      `;
    });
    
    html += '</div>';
  });
  
  html += `
    <div style="text-align: center; margin-top: 30px; padding: 20px; background: white; border-radius: 8px;">
      <button class="export-btn" onclick="exportCoasterData()">📋 Export as JSON</button>
      <button class="clear-btn" onclick="clearAllCoasters()">🗑️ Clear All Custom Coasters</button>
    </div>
    
    <script>
      function exportCoasterData() {
        var data = ${JSON.stringify(allCoasters, null, 2)};
        var blob = new Blob([JSON.stringify(data, null, 2)], {type: 'application/json'});
        var url = URL.createObjectURL(blob);
        var a = document.createElement('a');
        a.href = url;
        a.download = 'my-custom-coasters.json';
        a.click();
        URL.revokeObjectURL(url);
      }
      
      function clearAllCoasters() {
        if (confirm('Are you sure you want to delete ALL custom coasters? This cannot be undone!')) {
          // This would need to communicate back to the parent window
          alert('Please use the "Clear Custom Coasters" button in the main settings to delete all data.');
        }
      }
    </script>
    </body>
    </html>
  `;
  
  return html;
}

// Format coaster type for display
function formatCoasterType(type) {
  var typeMap = {
    'steel': 'Steel Coaster',
    'wood': 'Wood Coaster',
    'hybrid': 'Hybrid Coaster',
    'launched_steel': 'Launched Steel',
    'inverted': 'Inverted Steel',
    'flying': 'Flying Coaster',
    'spinning': 'Spinning Coaster',
    'mine_train': 'Mine Train',
    'bobsled': 'Bobsled/Alpine Coaster',
    'water': 'Water Coaster',
    'dark_ride': 'Dark Ride Coaster',
    'other': 'Other/Unknown'
  };
  return typeMap[type] || 'Unknown';
}

// Get display name for park ID
function getParkDisplayName(parkId) {
  var parkMap = {
    'sixflags_stlouis': 'Six Flags St. Louis',
    'sixflags_mm': 'Six Flags Magic Mountain',
    'sixflags_ga': 'Six Flags Great Adventure',
    'sixflags_fiesta': 'Six Flags Fiesta Texas',
    'sixflags_texas': 'Six Flags Over Texas',
    'cedar_point': 'Cedar Point',
    'kings_island': 'Kings Island',
    'canadas_wonderland': 'Canada\'s Wonderland',
    'carowinds': 'Carowinds',
    'knotts': 'Knott\'s Berry Farm',
    'silver_dollar': 'Silver Dollar City',
    'dollywood': 'Dollywood',
    'magic_kingdom': 'Magic Kingdom',
    'epcot': 'EPCOT',
    'hollywood_studios': 'Disney\'s Hollywood Studios',
    'animal_kingdom': 'Disney\'s Animal Kingdom',
    'disneyland': 'Disneyland',
    'dca': 'Disney California Adventure',
    'universal_florida': 'Universal Studios Florida',
    'islands_adventure': 'Universal\'s Islands of Adventure',
    'universal_hollywood': 'Universal Studios Hollywood',
    'busch_williamsburg': 'Busch Gardens Williamsburg',
    'busch_tampa': 'Busch Gardens Tampa Bay',
    'seaworld_orlando': 'SeaWorld Orlando',
    'hersheypark': 'Hersheypark',
    'kennywood': 'Kennywood',
    'other_park': 'Local/Other Park'
  };
  
  // Handle custom parks
  if (parkId && parkId.startsWith('custom_')) {
    // Try to get the park name from stored data
    var parkCoasters = JSON.parse(localStorage.getItem('custom-park-coasters')) || {};
    if (parkCoasters[parkId] && parkCoasters[parkId].length > 0) {
      return parkCoasters[parkId][0].parkName || parkId.replace('custom_', '').replace(/_/g, ' ');
    }
    return parkId.replace('custom_', '').replace(/_/g, ' ');
  }
  
  return parkMap[parkId] || parkId;
}

// Send custom coasters to watch
function sendCustomCoastersToWatch(parkId, coasters) {
  // Send message to watch with custom coaster data
  var message = {
    'CUSTOM_COASTERS': 1,
    'PARK_ID': parkId,
    'COASTER_COUNT': coasters.length
  };
  
  // Add individual coaster data (limited by message size)
  coasters.slice(0, 10).forEach(function(coaster, index) {
    message['COASTER_' + index + '_NAME'] = coaster.name;
    message['COASTER_' + index + '_TYPE'] = coaster.type;
  });
  
  Pebble.sendAppMessage(message, 
    function() {
      console.log('Custom coasters sent to watch for park:', parkId);
    },
    function(error) {
      console.log('Error sending custom coasters:', error);
    }
  );
}

// Add custom park 
function addCustomPark(parkName, settings) {
  if (!parkName || parkName.trim() === '') {
    return;
  }
  
  var parkKey = 'custom_' + parkName.toLowerCase().replace(/[^a-z0-9]/g, '_');
  
  // This is handled automatically when adding coasters now
  console.log('Custom park name "' + parkName + '" will be used when adding coasters.');
  
  // If they also specified a coaster, add it immediately
  if (settings && settings.NEW_COASTER_NAME && settings.NEW_COASTER_NAME.trim()) {
    // This will be handled by the addCoasterToPark function
    return;
  }
  
  alert('Custom park "' + parkName + '" ready! Now add a coaster to this park using the form above.');
}

// Handle data export requests from watch
function handleDataExport(payload) {
  exportRideData();
}
