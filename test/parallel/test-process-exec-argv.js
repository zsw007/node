'use strict';
const common = require('../common');
const assert = require('assert');
const spawn = require('child_process').spawn;

if (common.isZos)
  var execArgv = ['--stack-size=2048'];
else
  var execArgv = ['--stack-size=256'];

if (process.argv[2] === 'child') {
  process.stdout.write(JSON.stringify(process.execArgv));
} else {
  const args = [__filename, 'child', 'arg0'];
  const child = spawn(process.execPath, execArgv.concat(args));
  let out = '';

  child.stdout.on('data', function(chunk) {
    out += chunk;
  });

  child.on('close', function() {
    assert.deepStrictEqual(JSON.parse(out), execArgv);
  });
}
