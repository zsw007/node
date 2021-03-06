'use strict';
const common = require('../common');
if (!common.hasCrypto)
  common.skip('missing crypto');

const assert = require('assert');
const stream = require('stream');
const crypto = require('crypto');

if (!common.hasFipsCrypto) {
  // Small stream to buffer converter
  class Stream2buffer extends stream.Writable {
    constructor(callback) {
      super();

      this._buffers = [];
      this.once('finish', function() {
        callback(null, Buffer.concat(this._buffers));
      });
    }

    _write(data, encodeing, done) {
      this._buffers.push(data);
      return done(null);
    }
  }

  // Create an md5 hash of "Hallo world"
  const hasher1 = crypto.createHash('md5');
  hasher1.pipe(new Stream2buffer(common.mustCall(function end(err, hash) {
    assert.strictEqual(err, null);
    assert.strictEqual(
      hash.toString('hex'), '06460dadb35d3d503047ce750ceb2d07'
    );
  })));
  hasher1.end('Hallo world');

  // Simpler check for unpipe, setEncoding, pause and resume
  crypto.createHash('md5').unpipe({});
  crypto.createHash('md5').setEncoding('utf8');
  crypto.createHash('md5').pause();
  crypto.createHash('md5').resume();
}

// Decipher._flush() should emit an error event, not an exception.
const key = Buffer.from('48fb56eb10ffeb13fc0ef551bbca3b1b', 'hex');
const badkey = Buffer.from('12341234123412341234123412341234', 'hex');
const iv = Buffer.from('6d358219d1f488f5f4eb12820a66d146', 'hex');
const cipher = crypto.createCipheriv('aes-128-cbc', key, iv);
const decipher = crypto.createDecipheriv('aes-128-cbc', badkey, iv);

cipher.pipe(decipher)
  .on('error', common.mustCall(function end(err) {
    assert(/bad decrypt/.test(err));
  }));

cipher.end('Papaya!');  // Should not cause an unhandled exception.
