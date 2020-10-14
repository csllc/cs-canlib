'use strict';

const binary = require('node-pre-gyp');
const path = require('path');
const canlib_path = binary.find(path.resolve(path.join(__dirname,'./package.json')));
const canlib = require(canlib_path);

const canlibConsts = require('./lib/canlib_consts');

const { Duplex } = require('stream');

// Default configuration used unless overridden by caller
const DEFAULT_OPTIONS = {
  canRate: 250000,
  filters: [],
  loopback: false,
};

module.exports = class Canlib extends Duplex {

  constructor(options) {

    super({ objectMode: true });

    this.setOptions(options);
    this.requestQueue = [];
    this.channel = undefined
    this.handle = undefined;
    this.handle_thread = undefined;
    this.isReady = false;
    this.handle = null;
    this.flags = 0;
  }

  // Sets (or re-sets) the configuration options
  setOptions(options) {
    // Save for later use
    this.options = Object.assign(DEFAULT_OPTIONS, options);
  }

  // Returns a promise that resolves to a list of available CAN ports
  list() {
    var me = this;

    return new Promise(function(resolve) {
      // Initialize CAN library
      canlib.InitializeLibrary();

      if (process.platform == "win32") {
        // If running on Windows, get channel data
        resolve(canlib.ChannelInfo());
      } else {
        // If running on any other platform, we return an empty channel list in
        // an attempt to fail gracefully
        resolve([]);
      }
      
    })
      .then(function(channels) {
        // For compatibility, create a new property 'path' that is the
        // identifier for the CAN device in the system and can be passed
        // to the open() method
        for (var idx in channels) {
          channels[idx].path = channels[idx].channel;
        }
        return channels;
      })
      .catch(function(err) {
        me.emit('error', err);
        throw new Error(err);
      });
  }

  open(channel) { 
    var me = this;

    return new Promise(function(resolve) {
      // Save channel for posterity
      me.channel = channel;
      
      // Open the CAN channel and save the main thread's handle
      me.handle = canlib.OpenChannel(me.channel, me.flags);

      // Set channel baud and filter (if specified)
      me._configure();

      // Set loopback (aka transmit echo) for main thread's handle
      canlib.SetLoopback(me.handle, me.options.loopback);

      // Go on-bus with main thread's handle
      canlib.BusOn(me.handle);

      // Signal that the channel has been opened
      me.isReady = true;
      me.emit('open');
      resolve();
    })
      .then(function() {
        // Close event handler
        me.on('close', function() { // todo - do we really need function wrap?
          me.close();
        });

        // Data receive event handler
        me.on('_data', me._onData.bind(me));
        
        // Enable data event
        // - This opens a second handle to the channel and takes it on-bus
        me.handle_thread = canlib.EnableEvent(me.channel, me.flags, function() {
          me.emit('_data');
        });

        // Set loopback for the event thread's handle
        canlib.SetLoopback(me.handle_thread, me.options.loopback);

        // Set acceptance filter for the event thread's handle
        if (me.options.filters.length > 0) {
          for (let i in me.options.filters) {
            try {
              let filt = canlibConsts.convertAcceptanceFilter(me.options.filters[i]);
              canlib.SetAcceptanceFilter(me.handle_thread, filt.code,
                                         filt.mask, filt.ext);
            } catch (err) {
              console.error(err);
              throw new Error(err);
            }
          }
        }

      })
      .catch(function(err) {
        me.emit('error', err);
        throw new Error(err);
      });
  }

  close() {
    let me = this;

    return new Promise(function(resolve) {
      // Remove listeners
      me.removeAllListeners();

      if (me.isOpen()) {
        // Close event thread
        // TODO: verify that the channel goes off bus and all that
        canlib.DisableEvent(me.channel);

        // Take the main thread's handle off-bus
        canlib.BusOff(me.handle);

        // Close the main thread's handle to the channel
        canlib.Close(me.handle);
      }

      // Close stream
      me.emit('close');
      resolve();
    })
      .then(function() {
        me.handle = undefined;
        me.handle_thread = undefined;
        me.isReady = false;
        me.push(null);
      })
      .catch(function(err) {
        me.emit('error', err);
        throw new Error(err);
      });
  }

  write(msg) {
    let me = this;
    
    return new Promise(function(resolve) {
      if (msg.buf.length <= 8) {

        // Write message to bus
        let canlibMsg = canlibConsts.toCanlibMsg(msg);
        canlib.Write(me.handle, canlibMsg);
        me.emit('write', msg);
     
        resolve();
      } else {
        throw new Error("Tried to send invalid CAN data");
      }
    })
      .catch(function(err) {
        me.emit('error', err);
        throw new Error(err);
      });
  }

  status() {
    let me = this;

    return new Promise(function(resolve) {
      let status = canlib.Status(me.handle);
      resolve(status);
    })
      .catch(function(err) {
        me.emit('error', err);
        throw new Error(err);
      });
  }

  // Required function for cs-modbus GenericConnection
  isOpen() {
    return ((this.handle != undefined) &&
            (this.handle_thread != undefined) &&
            (this.isReady));
  }

  // Required function for cs-modbus GenericConnection
  isConnected() {
    return this.isOpen();
  }

  _onData() {
    let me = this;
    
    for (;;) {
      try {
        let read_obj = canlib.Read(me.handle_thread);
        me.push(read_obj); // Emits 'data' event
      } catch(err) {
        if (err.code != "No messages available") {
          console.error("_onData() error: " + err.code);
        }
        break;
      }
    }
  }

  // Required for a readable stream; we don't need to do anything
  _read() {}

  // Required for a writable stream; we don't need to do anything
  _write() {}

  _configure() {
    let me = this;

    // Set baud
    canlib.SetBusParams(me.handle, { freq: 250000,
                                     tseg1: 11,
                                     tseg2: 4,
                                     sjw: 4,
                                     noSamp: 3 } );

    // Set filters
    if (me.options.filters.length > 0) {
      for (let i in me.options.filters) {
        try {
          let filt = canlibConsts.convertAcceptanceFilter(me.options.filters[i]);
          canlib.SetAcceptanceFilter(me.handle, filt.code, filt.mask, filt.ext);
        } catch (err) {
          console.error(err);
          throw new Error(err);
        }
      }
    }
  }

}


