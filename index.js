'use strict';

// Load the native addon: prebuilds/<platform>-<arch>/ if a matching prebuild
// is present, otherwise build/Release (or build/Debug).
//
// The addon links against canlib32.dll, which is installed by the Kvaser
// driver package (it is not distributed with this module). If the driver is
// not installed, Windows can't load the addon. In that case, the module still
// loads so that callers probing for several kinds of CAN adapter keep working:
// list() resolves to an empty list, and anything else throws loadError.
let canlib;
let loadError = null;
try {
  canlib = require('node-gyp-build')(__dirname);
} catch (err) {
  if (process.platform !== 'win32' || err.code !== 'ERR_DLOPEN_FAILED') {
    throw err;
  }
  loadError = new Error('cs-canlib: could not load canlib32.dll. Install the ' +
    'Kvaser CANlib driver ("Kvaser Drivers for Windows", ' +
    'https://kvaser.com/download/).', { cause: err });
  loadError.code = 'ERR_CANLIB_NOT_INSTALLED';
  canlib = new Proxy({}, {
    get() { throw loadError; },
  });
}

const canlibConsts = require('./lib/canlib_consts');

const { Duplex } = require('stream');

// CANlib error codes (see canstat.h) that mean the Kvaser driver is not
// installed or cannot be loaded. list() reports no channels for these instead
// of failing, so that callers probing for several kinds of CAN adapter still
// work on machines without the Kvaser driver.
const DRIVER_MISSING_ERRORS = [
  -12, // canERR_DRIVER
  -16, // canERR_DYNALOAD
  -17, // canERR_DYNALIB
  -18, // canERR_DYNAINIT
  -23, // canERR_DRIVERLOAD
];

// Errors thrown by the native module carry CANlib's error text in err.code,
// so compare against the text for each driver-missing status
function isDriverMissingError(err) {
  return DRIVER_MISSING_ERRORS.some((status) => {
    try {
      return err.code === canlib.GetErrorText(status);
    } catch {
      return false;
    }
  });
}

// Default configuration used unless overridden by caller
const DEFAULT_OPTIONS = {
  canRate: 250000,
  filters: [],
  loopback: false,
};

module.exports = class Canlib extends Duplex {

  constructor(options) {

    super({ objectMode: true });

    let me = this;

    this.setOptions(options);
    this.requestQueue = [];
    this.channel = undefined
    this.handle = undefined;
    this.handleThread = undefined;
    this.isReady = false;
    this.handle = null;
    this.flags = 0;
    this.status = {
      code: undefined,
      string: "",
      isError: false,
      notify() {
        let status = { code: this.code, string: this.string, isError: this.isError };
        me.emit('status', status);
      },
      get statusCode() { return this.code; },
      set statusCode(code) {
        let code_masked = (code & ~0x30); // Filter out pending flags
        let same = (this.code & code_masked) ? true : false;
        if (!same) {
          this.code = code_masked;
          if (this.code == 0) {
            this.string = "Bus OK";
            this.isError = false;
          } else if (this.code == 0x0008) {
            this.string = canlib.GetFlagText(this.code);
            this.isError = false;
          } else {
            this.isError = true;
            try {
              this.string = canlib.GetFlagText(this.code);
            } catch(err) {
              console.error(err);
            }
          }
          this.notify();
        }
      }
    };
  }

  // Sets (or re-sets) the configuration options
  setOptions(options) {
    // Save for later use
    this.options = Object.assign(DEFAULT_OPTIONS, options);
  }

  // Returns a promise that resolves to a list of available CAN channels
  // These channels get passed to this.open() and ultimately can.EnableEvent()
  // Rejects if CANlib reports an error, unless the error means the Kvaser
  // driver is not installed (or canlib32.dll could not be loaded at all), in
  // which case it resolves to an empty list.
  list() {
    return new Promise(function(resolve) {
      if (process.platform != "win32") {
        // CANlib is only supported on Windows; on any other platform, return
        // an empty channel list in an attempt to fail gracefully
        resolve([]);
        return;
      }

      // Kvaser driver not installed
      if (loadError) {
        resolve([]);
        return;
      }

      // Initialize CAN library
      canlib.InitializeLibrary();

      // Get channel data. An exception thrown here rejects the promise.
      let channels;
      try {
        channels = canlib.ChannelInfo();
      } catch(err) {
        if (!isDriverMissingError(err)) {
          throw err;
        }
        channels = [];
      }

      // For compatibility, create a new property 'path' that is the
      // identifier for the CAN device in the system and can be passed
      // to the open() method
      for (let channel of channels) {
        channel.path = channel.channel;
      }

      resolve(channels);
    });
  }

  open(channel) { 
    var me = this;

    return new Promise(function(resolve) {
      // Initialize CAN library, if not already done
      canlib.InitializeLibrary();

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

      resolve();
    })
      .then(function() {
        // Set up handler for 'close' event
        me.on('close', () => { me.close(); });

        // Set up handler for data receive event
        me.on('_data', me._onData.bind(me));
        
        // Enable data event
        // - This opens a second handle to the channel and takes it on-bus
        // The native code calls this from a Node-API thread-safe function.
        // Defer the actual work with setImmediate() so that an exception
        // thrown by a 'data' listener is reported as a normal uncaught
        // exception, instead of being swallowed by Node-API with a DEP0168
        // warning.
        me.handleThread = canlib.EnableEvent(me.channel, me.flags, function() {
          setImmediate(() => me.emit('_data'));
        });

        // Set loopback for the event thread's handle
        canlib.SetLoopback(me.handleThread, me.options.loopback);

        // Set acceptance filter for the event thread's handle
        me._setAcceptanceFilters(me.handleThread);

      })
      .then(function() {
        // Signal that the channel has been opened
        me.isReady = true;
        me.emit('open');
      })
    .catch(function(err) {
        me.emit('error', err); // emit() instead of throw, matching CAN-USB-COM behavior
      });
  }

  close() {
    let me = this;

    return new Promise(function(resolve, reject) {
      if (me.handle === undefined) {
        reject(new Error("CAN bus handle is undefined"));
      } else {
        // Remove listeners
        me.removeAllListeners();

        if (me.isOpen()) {
          // Close event thread
          canlib.DisableEvent(me.channel);

          // Take the main thread's handle off-bus
          canlib.BusOff(me.handle);

          // Close the main thread's handle to the channel
          canlib.Close(me.handle);
        }

        // Close stream
        me.emit('close');
        resolve();
      }
    })
      .then(function() {
        me.handle = undefined;
        me.handleThread = undefined;
        me.isReady = false;
        me.push(null);
      })
      .catch(function(err) {
        throw err;
      });
  }

  write(msg) {
    let me = this;
    
    return new Promise(function(resolve, reject) {
      if (me.handle === undefined) {
        reject(new Error("CAN bus handle is undefined"));
      } else {
        if (msg.buf.length <= 8) {
          // Write message to bus
          let canlibMsg = canlibConsts.toCanlibMsg(msg);
          canlib.Write(me.handle, canlibMsg);
          me.emit('write', msg);
          
          // If status changed, the following will emit a 'status' event
          canlib.RequestChipStatus(me.handle);
          me.status.statusCode = canlib.ReadStatus(me.handle);
          
          resolve();
        } else {
          reject(new Error("Tried to send invalid CAN data"));
        }
      }
    })
    .catch(function(err) {
      throw err;
    });
  }

  status() {
    let me = this;

    return new Promise(function(resolve, reject) {
      if (me.handle === undefined) {
        reject(new Error("CAN bus handle is undefined"));
      } else {
        let status = canlib.Status(me.handle);
        resolve(status);
      }
    })
      .catch(function(err) {
        throw err;
      });
  }

  identify() {
    let me = this;

    return new Promise(function(resolve) {
      // Briefly flash LED(s) on adapter
      canlib.FlashLeds(me.handle, 0, 10);
      canlib.FlashLeds(me.handle, 1, 10);
      resolve();
    })
    .catch(function(err) {
      me.emit('error', err);
      throw err;
    });
  }

  // Required function for cs-modbus GenericConnection
  isOpen() {
    return ((this.handle != undefined) &&
            (this.handleThread != undefined) &&
            (this.isReady));
  }

  // Required function for cs-modbus GenericConnection
  isConnected() {
    return this.isOpen();
  }

  _onData() {
    let me = this;
    
    if (me.handle === undefined) {
      throw new Error("CAN bus handle is undefined");
    } else {
      // Drain the receive queue first. Only canlib.Read() is inside the
      // try/catch: push() can emit 'data' synchronously, and an exception
      // thrown by a 'data' listener must not be mistaken for a read error
      // (that used to swallow the exception and leave messages in the queue).
      let messages = [];
      for (;;) {
        try {
          messages.push(canlib.Read(me.handleThread));
        } catch(err) {
          if (err.code != "No messages available") {
            console.error("_onData() read error:", err);
          }
          // Otherwise there are no more messages (or this event was signaled
          // for another reason)
          break;
        }
      }

      // If status changed, the following will emit a 'status' event
      canlib.RequestChipStatus(me.handleThread);
      me.status.statusCode = canlib.ReadStatus(me.handleThread);

      for (let read_obj of messages) {
        me.push(canlibConsts.toMsg(read_obj)); // Emits 'data' event
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
    canlib.SetBusParams(me.handle,
                        canlib.TranslateBaud(me.options.canRate));

    // Set filters
    me._setAcceptanceFilters(me.handle)
  }

  _setAcceptanceFilters(handle) {
    let me = this;

    if (me.options.filters.length > 0) {
      // Standard filter
      if (me.options.filters.some((f) => { return (f.ext == false); })) {
        // If any of the filters in the array are for standard messages, apply them
        for (let i in me.options.filters) {
          if (me.options.filters[i].ext == false) {
            try {
              let filt = canlibConsts.convertAcceptanceFilter(me.options.filters[i]);
              canlib.SetAcceptanceFilter(handle, filt.code, filt.mask, filt.ext);
            } catch (err) {
              me.emit('error', err);
            }
          }
        }
      } else {
        // Otherwise, apply a filter that blocks all standard messages
        canlib.SetAcceptanceFilter(handle, 0xFFFFFFFF, 0xFFFFFFFF, false);
      }

      if (me.options.filters.some((f) => { return (f.ext == true); })) {
        // If any of the filters in the array are for extended messages, apply them
        for (let i in me.options.filters) {
          if (me.options.filters[i].ext == true) {
            try {
              let filt = canlibConsts.convertAcceptanceFilter(me.options.filters[i]);
              canlib.SetAcceptanceFilter(handle, filt.code, filt.mask, filt.ext);
            } catch (err) {
              me.emit('error', err);
            }
          }
        }
      } else {
        // Otherwise, apply a filter that blocks all extended messages
        canlib.SetAcceptanceFilter(handle, 0xFFFFFFFF, 0xFFFFFFFF, true);
      }
    }
  }

}



