'use strict';

const path = require('path');

// Load the native addon directly (not the stream wrapper in index.js)
const canlib = require('node-gyp-build')(path.join(__dirname, '../..'));

canlib.InitializeLibrary();

let channels = canlib.ChannelInfo();
let flags = 0;

let channel = channels[0].channel;

let handle = canlib.OpenChannel(channel, flags);
console.log("handle = " + handle);

canlib.SetBusParams(handle, { freq: 250000,
                              tseg1: 11,
                              tseg2: 4,
                              sjw: 4,
                              noSamp: 3 } );

canlib.BusOn(handle);

let msg;

let handle_thread = canlib.EnableEvent(channel, flags, function() {
  for (;;) {
    try {
      msg = canlib.Read(handle_thread);
      msg.id += 1;
      canlib.Write(handle_thread, msg);
    } catch (err) {
      if (err.code != "No messages available") {
        console.log(err);
      }
      break;
    }
  }
});

console.log("handle_thread = " + handle_thread);

setInterval( function() {
  canlib.Write(handle, { id: 100,
                         dlc: 8,
                         flag: 0,
                         buf: Buffer.from([1, 2, 3, 4, 5, 6, 7, 8]) });
}, 1500);

//canlib.Close(handle);
