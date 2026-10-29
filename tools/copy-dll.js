'use strict';

// Copy the Kvaser CANlib DLL next to the prebuilt binary made by prebuildify.
// Node loads addons with LOAD_WITH_ALTERED_SEARCH_PATH, so Windows looks for
// canlib32.dll in the addon's own folder before the system folders. This lets
// the module work on machines where the Kvaser driver is not installed yet.

const fs = require('fs');
const path = require('path');

const root = path.join(__dirname, '..');
const src = path.join(root, 'ext', 'canlib-win32-x64', 'bin', '64', 'canlib32.dll');
const destDir = path.join(root, 'prebuilds', 'win32-x64');

if (!fs.existsSync(destDir)) {
  console.error(`copy-dll: ${destDir} not found; run prebuildify first`);
  process.exit(1);
}

fs.copyFileSync(src, path.join(destDir, 'canlib32.dll'));
console.log(`copy-dll: copied canlib32.dll to ${path.relative(root, destDir)}`);
