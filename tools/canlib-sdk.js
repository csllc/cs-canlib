'use strict';

// Print the location of the Kvaser CANlib SDK, for binding.gyp.
//
// The SDK provides the header files and import library needed to build this
// module. Its license does not allow redistributing them, so they are not
// part of this package; install the SDK from Kvaser instead:
//   https://kvaser.com/developer/canlib-sdk
//
// The SDK folder is taken from the KVASER_CANLIB_SDK environment variable,
// or else the SDK installer's default location.

const fs = require('fs');
const path = require('path');

const DEFAULT_SDK = 'C:\\Program Files (x86)\\Kvaser\\Canlib';

// The module only builds on Windows; print nothing elsewhere so that gyp can
// still evaluate binding.gyp
if (process.platform !== 'win32') {
  process.exit(0);
}

const sdk = process.env.KVASER_CANLIB_SDK || DEFAULT_SDK;

for (const file of [ path.join('INC', 'canlib.h'), path.join('Lib', 'x64', 'canlib32.lib') ]) {
  if (!fs.existsSync(path.join(sdk, file))) {
    console.error(`cs-canlib: Kvaser CANlib SDK not found: ${path.join(sdk, file)} is missing.`);
    console.error('Install the CANlib SDK from https://kvaser.com/developer/canlib-sdk, or set');
    console.error('the KVASER_CANLIB_SDK environment variable to the folder where it is installed.');
    process.exit(1);
  }
}

// gyp prefers forward slashes
process.stdout.write(sdk.replace(/\\/g, '/'));
