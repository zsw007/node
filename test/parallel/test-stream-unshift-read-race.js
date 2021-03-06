'use strict';
const common = require('../common');
const assert = require('assert');

// This test verifies that:
// 1. unshift() does not cause colliding _read() calls.
// 2. unshift() after the 'end' event is an error, but after the EOF
//    signalling null, it is ok, and just creates a new readable chunk.
// 3. push() after the EOF signaling null is an error.
// 4. _read() is not called after pushing the EOF null chunk.

const stream = require('stream');
const hwm = 10;
const r = stream.Readable({ highWaterMark: hwm });
const chunks = 10;

const data = Buffer.allocUnsafe(chunks * hwm + Math.ceil(hwm / 2));
for (let i = 0; i < data.length; i++) {
  const c = 'asdf'.charCodeAt(i % 4);
  data[i] = c;
}

let pos = 0;
let pushedNull = false;
r._read = function(n) {
  assert(!pushedNull, '_read after null push');

  // every third chunk is fast
  push(!(chunks % 3));

  function push(fast) {
    assert(!pushedNull, 'push() after null push');
    const c = pos >= data.length ? null : data.slice(pos, pos + n);
    pushedNull = c === null;
    if (fast) {
      pos += n;
      r.push(c);
      if (c === null) pushError();
    } else {
      setTimeout(function() {
        pos += n;
        r.push(c);
        if (c === null) pushError();
      }, 1);
    }
  }
};

function pushError() {
  assert.throws(function() {
    r.push(Buffer.allocUnsafe(1));
  }, /^Error: stream.push\(\) after EOF$/);
}


const w = stream.Writable();
const written = [];
w._write = function(chunk, encoding, cb) {
  written.push(chunk.toString());
  cb();
};

r.on('end', common.mustCall(function() {
  assert.throws(function() {
    r.unshift(Buffer.allocUnsafe(1));
  }, /^Error: stream.unshift\(\) after end event$/);
  w.end();
}));

r.on('readable', function() {
  let chunk;
  while (null !== (chunk = r.read(10))) {
    w.write(chunk);
    if (chunk.length > 4)
      r.unshift(Buffer.from('1234'));
  }
});

w.on('finish', common.mustCall(function() {
  // each chunk should start with 1234, and then be asfdasdfasdf...
  // The first got pulled out before the first unshift('1234'), so it's
  // lacking that piece.
  assert.strictEqual(written[0], 'asdfasdfas');
  let asdf = 'd';
  console.error(`0: ${written[0]}`);
  for (let i = 1; i < written.length; i++) {
    console.error(`${i.toString(32)}: ${written[i]}`);
    assert.strictEqual(written[i].slice(0, 4), '1234');
    for (let j = 4; j < written[i].length; j++) {
      const c = written[i].charAt(j);
      assert.strictEqual(c, asdf);
      switch (asdf) {
        case 'a': asdf = 's'; break;
        case 's': asdf = 'd'; break;
        case 'd': asdf = 'f'; break;
        case 'f': asdf = 'a'; break;
      }
    }
  }
}));

process.on('exit', function() {
  assert.strictEqual(written.length, 18);
  console.log('ok');
});
