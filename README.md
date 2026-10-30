# NodeJS CANlib Module

This module provides a NodeJS interface to the [Kvaser](https://www.kvaser.com/) [Leaf Light v2](https://www.kvaser.com/product/kvaser-leaf-light-hs-v2/) adapter, and possibly others that are supported by the [CANlib library](https://www.kvaser.com/developer/canlib-sdk/).

This module is meant to serve as a drop-in replacement for `@csllc/can-usb-com`, allowing Kvaser adapters to be used in place of [GridConnect CAN-USB-COM](https://www.gridconnect.com/products/usb-can-interface-can-usb-com).

This module uses native code through N-API and currently supports Windows only.

## Getting Started

The following assumes that [NodeJS](https://nodejs.org) is already installed. This module supports Windows x86_64 only, and Node 24 or later is recommended. Prebuilt binaries are included in the npm package, so installing it does not require a compiler.

The Kvaser CANlib driver must also be installed ("Kvaser Drivers for Windows", from the [Kvaser downloads page](https://kvaser.com/download/)). It provides `canlib32.dll`, which this module uses but does not include. Without the driver, the module still loads, but `list()` returns no channels and opening a channel fails with an error whose `code` is `ERR_CANLIB_NOT_INSTALLED`.

To install this module, run:
```
npm install @csllc/cs-canlib
```

The following is a sample script that opens the CAN port, writes a single message to the bus, and listens for and prints received messages from the bus.

```js
const canlib = require("@csllc/cs-canlib");

let can = new canlib({
  canRate: 250000,
  loopback: true,
  filters: [{
    ext: true,
    id: '10EF8000 10EFFFFF',
  }]

});

can.list()
  .then((ports) => {
    return can.open(ports[0].path)
      .catch((err) => {
        console.error(err);
        process.exit(1);
      });
  })
  .then(() => {
    can.write({ id: 0x10EF8001,
                ext: true,
                buf: Buffer.from([0x45, 0x00, 0x00, 0x04, 0x00, 0x00]) });
  })
  .catch((err) => {
    console.error(err);
    can.close();
    process.exit(1);
  });

can.on('data', function(msg) {
  console.log("data: " + JSON.stringify(msg, (key, value) => {
    if ((typeof value === 'number') && (key == "id")) {
      return value.toString(16).toUpperCase().padStart(8, '0');
    } else if (typeof value === 'number') {
      return value.toString(16).toUpperCase().padStart(2, '0');
    } else {
      return value;
    }
  }));
});

```

## Examples

Both JavaScript and Win32 examples are available in the `example` folder.

## Configuration

The constructor accepts an object that specifies the desired configuration. The CAN adapter is set up before the `open` method resolves, so once it is complete, the CAN interface is ready for use.

The default options are shown in the following example. Any of them may be omitted from the constructor's option object to use their default value.

```js
let can = new Can({

  // bit rate on the CAN bus
  canRate: 250000,

  // filters for incoming packets
  filters: [
  ],

  // useful for testing, each sent packet is also received
  loopback: false,
  });
```

### Streaming

`cs-canlib` extends the NodeJS stream interface, so it can be piped into other stream instances.

### Filtering

By default, all CAN message are captured, but this can be limited by applying a message filter. The following are examples of filter definitions that are passed to the `open` method.

```js
{
  ext: true, // Applies to extended (29-bit IDs)
  id: '10EF0000 10EFFFFF', // Allow IDs 0x10EF0000 through 0x10EFFFFF
}

{
  ext: true, // Applies to extended (29-bit IDs)
  fromID: 0x10EF0000, // Allow IDs 0x10EF0000 through 0x10EF7777
  toID: 0x10EF7777,
}

{
  ext: false, // Applies to standard (11-bit IDs)
  code: 0x081, // Acceptance code
  mask: 0x0F8, // Acceptance mask
}

```

The Kvaser Leaf Light v2 adapter supports two filters at a time: One for standard messages and one for extended messages. If multiple filters are specified in the array passed to the module's constructor, the last filters in the array take effect.

Three filter formats are supported by the package:

 - `code` and `mask` format, directly specifying the acceptance filter with a code and mask

        `code: 0x10EF0000, mask: 0xFFFF0000`

 - `fromID` and `toID` format, specifying a range of IDs to be accepted by the filter:

        `fromID: 0x10EF0000, toID: 0x10EFFFFF`

 - `id` format, specifying a string with a range of IDs to be accepted by the filter:

        `id: '10EF0000 10EFFFFF'`

Given a range of IDs to allow, it is possible to convert to an acceptance code and mask pair:

```
acceptance_code = fromID & toID
acceptance_mask = ~(fromID ^ toID)
```

This module implements the same behavior specified by the [CAN-USB-COM manual](https://gridconnect.box.com/shared/static/c7l5u2vgsid1a24y6r0a0wv5hwaunr71.pdf):
 - When [filtering is] disabled, all CAN messages are received regardless of the definitions in the list
 - When [filtering is] enabled, only messages which match filter definitions in the list will be passed through.
 
To achieve this, if only one type of filter (standard or extended) is specified, an all-blocking filter will be applied for the unspecified type.

### JavaScript Events

The module emits the following events:

 - `open` when the serial port is successfully opened
 - `error` if an error occurs
 - `data` when an incoming CAN bus frame is received
 - `write` when an outgoing CAN bus frame is sent to the device (the event is emitted before the frame is actually sent on the bus)
 - `close` when the port is closed

To listen for the events, use the typical NodeJS EventEmitter pattern:

```js
  can.on('open', function(){
    console.log( 'CAN bus opened');
  })

  can.on('data', function(msg){
    console.log( msg );
  })

  can.on('close', function(err){
    if( err && err.disconnected ) {
      console.log( 'CAN bus disconnected' );
    }
    else {
      console.log( 'CAN bus closed by application' );
    }
  })

  can.on('error', function(err){
    console.log( 'CAN bus error: ', err );
  })
```

### Asynchronous Message Reception and Callbacks

This module takes advantage of the asynchronous message receive notification offered by the CANlib API. It implements a 'light' callback function in the native code, which simply calls the JavaScript function provided when the event is enabled using `canlib.EventEnable()`. The JavaScript function is then expected to call `canlib.Read()` to retrieve the newly received message, which uses the API's `canRead()` function behind-the-scenes.

In the future, it may make sense to move the call to `canRead()` directly into the native code to improve performance.

This module receives a Win32 event handle from the CANlib API through the `canIoCtl(..., canIOCTL_GET_EVENTHANDLE, ...)` API call, which gets signaled upon internal events that occur in the driver, including receipt of a CAN message that is accepted by the message filter. A Win32 thread waits on the event using `WaitForMultipleObjects`, invokes a callback function when signaled, and resets itself for the next message.

CANlib differs from PCAN-Basic in how handles to CAN adapters are managed. Under CANlib, each thread must individually  open the channel (calling `canOpenChannel`) and go on-bus (`canBusOn`) before it can interact with the channel; handles to an adapter cannot be shared and passed between threads. Failing to do so and attempting to write messages to the bus will result in a transmit buffer overflow. Other API calls made against a handle not owned by the calling thread will fail silently.

## API

This module's API functions generally return Promises, which are resolved or rejected when a request is complete. 

Refer to the [documentation on Promises](https://developer.mozilla.org/en-US/docs/Web/JavaScript/Reference/Global_Objects/Promise) for details on how to chain requests together, detect errors, etc. 

Refer to the CANlib documentation for more details on the API that this module implements.

## Development

Before updating or modifying this package, please

 - Lint any changes using `eslint`.
 - Confirm that unit tests pass.

In order to run unit tests (`npm test`), at least one Kvaser device must be connected to the computer.

The `pingpong` test is currently configured to use `@csllc/cs-canlib` for one adapter, and `@csllc/can-usb-com` for the other. These adapters must be connected via a properly terminated bus.

Do not run any tests on a bus with active traffic, since receiving unexpected CAN packets may confuse the tests.

### Build Environment Setup

#### Windows x86_64

 1. Install NodeJS 24 or later. Ensure that `node` and `npm` are found when invoked through the Command Prompt.
 1. Install the native build tools, either by checking "Automatically install the necessary tools" in the NodeJS installer, or manually:
    - Visual Studio (or Visual Studio Build Tools) with the "Desktop development with C++" workload
    - Python 3
 1. Install the [Kvaser CANlib SDK](https://kvaser.com/developer/canlib-sdk) (which provides the header files and import library needed to build) and the Kvaser driver (which provides `canlib32.dll`). The build looks for the SDK in its default location, `C:\Program Files (x86)\Kvaser\Canlib`; if it is installed elsewhere, set the `KVASER_CANLIB_SDK` environment variable to its folder. Release builds use CANlib SDK 5.33, but any version from 5.33 on should work.
 1. Clone the `cs-canlib` repository:
    `git clone https://github.com/csllc/cs-canlib`
 1. Install dependencies:
    - `cd cs-canlib\`
    - `npm install`
 1. Build:
    - `npm run rebuild`
 1. Test:
    - `npm test`

#### Build Notes

The module is loaded with [`node-gyp-build`](https://github.com/prebuild/node-gyp-build), which uses a prebuilt binary from `prebuilds/win32-x64/` if one is present and otherwise falls back to `build/Release/`. When working on the C code, delete the `prebuilds/` folder (if you made one) so that your local build is the one that gets loaded.

If the test stage fails, useful error messages may be hidden by `mocha`. When tracking down the cause of an error, running the example code directly, for example `node examples/js/dump.js`, may be helpful.

It may also be helpful to recompile the module with `CANLIB_DEBUG` and `CANLIB_EVENT_DEBUG` defined to enable debugging messages.

#### Editor Support

A `compile_flags.txt` is included for [clangd](https://clangd.llvm.org/). It uses the Node-API headers from the `node-api-headers` dev dependency, so run `npm install` first, and the CANlib SDK headers from the SDK's default location (edit the `-I` line if the SDK is installed elsewhere).

## Distribution

The module uses only Node-API, so a single binary works with every supported NodeJS version. The published npm package includes a prebuilt binary for Windows x86_64 in `prebuilds/win32-x64/`. If the prebuilt binary is missing, `npm install` falls back to building from source, which requires the build environment described above, including the CANlib SDK.

To make the prebuilt binary locally:

 - `npm run prebuildify`

### Releasing

Releases are built and published by the GitHub Actions workflow in `.github/workflows/release.yml`, which runs when a version tag is pushed:

 1. Run the tests locally with a Kvaser adapter connected: `npm test` (the CI runners have no CAN hardware, so they only check that the binary builds and loads)
 1. `npm version patch` (or `minor` / `major`), which updates `package.json` and creates the tag `vX.Y.Z`
 1. `git push --follow-tags`

The workflow downloads a pinned version of the CANlib SDK from Kvaser (checking its SHA256 hash), builds the Windows x86_64 binary, checks that it loads, and publishes the package to npm with the binary included. Publishing uses npm trusted publishing, so no npm token is stored in GitHub; the package's trusted publisher on npmjs.com must name this repository and the workflow file `release.yml`.

## Functionality Limitations

### Untested Functionality

The following have not been tested due to hardware availability.
	
 - All FD functionality
 - Enumerating more than one Kvaser device connected to a computer

## Licensing and Dependencies

This project is licensed under the MIT license - see the [LICENSE](LICENSE) file for details.

It uses the following third-party components, which are not included in this repository or in the npm package:

* [Kvaser CANlib SDK](https://kvaser.com/developer/canlib-sdk) - Copyright KVASER AB, Sweden. The module is compiled against the SDK's header files and import library, which are subject to Kvaser's license for the SDK and are not redistributed here. Install the SDK from Kvaser to build the module from source.
* Kvaser CANlib driver - Copyright KVASER AB, Sweden. Provides `canlib32.dll` at runtime. Install it from Kvaser; it is not distributed with this package.
