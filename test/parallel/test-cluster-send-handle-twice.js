'use strict';
// Testing to send an handle twice to the parent process.

const common = require('../common');
const assert = require('assert');
const cluster = require('cluster');
const net = require('net');

const workers = {
  toStart: 1
};

if (cluster.isMaster) {
  for (let i = 0; i < workers.toStart; ++i) {
    const worker = cluster.fork();
    worker.on('exit', common.mustCall(function(code, signal) {
      assert.strictEqual(code, 0, `Worker exited with an error code: ${code}`);
      assert.strictEqual(signal, null, `Worker exited by a signal: ${signal}`);
    }));
  }
} else {
  const server = net.createServer(function(socket) {
    process.send('send-handle-1', socket);
    process.send('send-handle-2', socket);
  });

  server.listen(0, function() {
    const client = net.connect({
      host: 'localhost',
      port: server.address().port
    });
    client.on('close', common.mustCall(() => { cluster.worker.disconnect(); }));
    setTimeout(function() { client.end(); }, 50);
  }).on('error', function(e) {
    console.error(e);
    assert.fail('server.listen failed');
    cluster.worker.disconnect();
  });
}
