function sendTimezoneToWatch() {
  // Get the number of seconds to add to convert localtime to utc
  var offsetMinutes = new Date().getTimezoneOffset() * 60;
  // Send it to the watch
  //console.log("offset: "+offsetMinutes);
  Pebble.sendAppMessage({ timezoneOffset: offsetMinutes });
}

// Listen for when an AppMessage is received
Pebble.addEventListener('ready', 
  function(e) {
    console.log("PebbleKit JS ready!");
    sendTimezoneToWatch();
  }
);

Pebble.addEventListener('appmessage',
  function(e) {
    console.log("AppMessage received!");
  }                     
);
