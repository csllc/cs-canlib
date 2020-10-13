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
    this.port = undefined;
    this.port_thread = undefined;
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

  open(port) { 
    var me = this;

    return new Promise(function(resolve) {
      // Open the CAN port (aka "channel") with a given handle
      me.handle = canlib.OpenChannel(port, me.flags);

      // Save port (channel) for posterity
      me.port = port;

      // Set channel baud and filter (if any)
      me._configure();

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
        me.port_thread = canlib.EnableEvent(port, me.flags, function() {
          me.emit('_data');
        });
      })
      .then(function() {
        canlib.FlushTransmitQueue(port);
        canlib.FlushReceiveQueue(port);
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
        canlib.DisableEvent(me.port);
        canlib.BusOff(me.port);
        canlib.Close(me.port);
      }

      // Close stream
      me.emit('close');
      resolve();
    })
      .then(function() {
        me.port = undefined;
        me.port_thread = undefined;
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
        canlib.Write(me.port_thread, canlibMsg);
        me.emit('write', msg);
/*
        // If loopback is enabled, push message back into receive stream
        if (me.options.loopback) {
          me.push(tpcan.toMsg(tpcan_buffer));
        }
*/
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
      let status = canlib.Status(me.port);
      resolve(status);
    })
      .catch(function(err) {
        me.emit('error', err);
        throw new Error(err);
      });
  }

  // Required function for cs-modbus GenericConnection
  isOpen() {
    return ((this.port != undefined) &&
            (this.port_thread != undefined) &&
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
        let read_obj = canlib.Read(me.port_thread);
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

    // Set filters
    if (me.options.filters.length > 0) {
      // Ensure extended/standard filters aren't being mixed, because this
      // isn't supported by the hardware.
      let ext_option = me.options.filters[0].ext;

      for (let i in me.options.filters) {
        if (me.options.filters[i].ext != ext_option) {
          throw new Error("CANlib does not support mixing extended and standard filters.");
        }
      }
      
/*
      for (let i in me.options.filters) {
        if ((me.options.filters[i].code) && (me.options.filters[i].mask)) {
          if (me.options.filters[i].ext) {
            pcan.AcceptanceFilter29Bit(me.port, me.options.filters[i].code,
                                       me.options.filters[i].mask);
          } else {
            pcan.AcceptanceFilter11Bit(me.port, me.options.filters[i].code,
                                       me.options.filters[i].mask);
          }
        } else if ((me.options.filters[i].fromID) && (me.options.filters[i].toID)){

          pcan.FilterMessages(me.port, me.options.filters[i].fromID,
                              me.options.filters[i].toID, 0x02);

        } else if (me.options.filters[i].id) {

          // Split start/stop range and convert them to acceptance code + mask
          let fromID = parseInt(me.options.filters[i].id.split(' ')[0], 16);
          let toID = parseInt(me.options.filters[i].id.split(' ')[1], 16);

          pcan.FilterMessages(me.port, fromID, toID, 0x02);

        } else {
          throw new Error("Unknown or unspecified filter format.");
        }
      }
*/
    }

    // Set baud
        canlib.SetBusParams(me.handle, {
          freq: 250000,
          tseg1: 11,
          tseg2: 4,
          sjw: 4,
          noSamp: 3});
  }
}

