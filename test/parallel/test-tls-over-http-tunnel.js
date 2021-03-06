'use strict';
const common = require('../common');
if (!common.hasCrypto)
  common.skip('missing crypto');

const assert = require('assert');
const https = require('https');
const net = require('net');
const http = require('http');
const fixtures = require('../common/fixtures');

let gotRequest = false;

const key = fixtures.readKey('agent1-key.pem');
const cert = fixtures.readKey('agent1-cert.pem');

const options = {
  key: key,
  cert: cert
};

const server = https.createServer(options, function(req, res) {
  console.log('SERVER: got request');
  res.writeHead(200, {
    'content-type': 'text/plain'
  });
  console.log('SERVER: sending response');
  res.end('hello world\n');
});

const proxy = net.createServer(function(clientSocket) {
  console.log('PROXY: got a client connection');

  let serverSocket = null;

  clientSocket.on('data', function(chunk) {
    if (!serverSocket) {
      // Verify the CONNECT request
      assert.strictEqual(`CONNECT localhost:${server.address().port} ` +
                         'HTTP/1.1\r\n' +
                         'Proxy-Connections: keep-alive\r\n' +
                         `Host: localhost:${proxy.address().port}\r\n` +
                         'Connection: close\r\n\r\n',
                         chunk.toString());

      console.log('PROXY: got CONNECT request');
      console.log('PROXY: creating a tunnel');

      // create the tunnel
      serverSocket = net.connect(server.address().port, function() {
        console.log('PROXY: replying to client CONNECT request');

        // Send the response
        clientSocket.write('HTTP/1.1 200 OK\r\nProxy-Connections: keep' +
                           '-alive\r\nConnections: keep-alive\r\nVia: ' +
                           `localhost:${proxy.address().port}\r\n\r\n`);
      });

      serverSocket.on('data', function(chunk) {
        clientSocket.write(chunk);
      });

      serverSocket.on('end', function() {
        clientSocket.destroy();
      });
    } else {
      serverSocket.write(chunk);
    }
  });

  clientSocket.on('end', function() {
    serverSocket.destroy();
  });
});

server.listen(0);

proxy.listen(0, function() {
  console.log('CLIENT: Making CONNECT request');

  const req = http.request({
    port: this.address().port,
    method: 'CONNECT',
    path: `localhost:${server.address().port}`,
    headers: {
      'Proxy-Connections': 'keep-alive'
    }
  });
  req.useChunkedEncodingByDefault = false; // for v0.6
  req.on('response', onResponse); // for v0.6
  req.on('upgrade', onUpgrade);   // for v0.6
  req.on('connect', onConnect);   // for v0.7 or later
  req.end();

  function onResponse(res) {
    // Very hacky. This is necessary to avoid http-parser leaks.
    res.upgrade = true;
  }

  function onUpgrade(res, socket, head) {
    // Hacky.
    process.nextTick(function() {
      onConnect(res, socket, head);
    });
  }

  function onConnect(res, socket, header) {
    assert.strictEqual(200, res.statusCode);
    console.log('CLIENT: got CONNECT response');

    // detach the socket
    socket.removeAllListeners('data');
    socket.removeAllListeners('close');
    socket.removeAllListeners('error');
    socket.removeAllListeners('drain');
    socket.removeAllListeners('end');
    socket.ondata = null;
    socket.onend = null;
    socket.ondrain = null;

    console.log('CLIENT: Making HTTPS request');

    https.get({
      path: '/foo',
      key: key,
      cert: cert,
      socket: socket,  // reuse the socket
      agent: false,
      rejectUnauthorized: false
    }, function(res) {
      assert.strictEqual(200, res.statusCode);

      res.on('data', function(chunk) {
        assert.strictEqual('hello world\n', chunk.toString());
        console.log('CLIENT: got HTTPS response');
        gotRequest = true;
      });

      res.on('end', function() {
        proxy.close();
        server.close();
      });
    }).on('error', function(er) {
      // We're ok with getting ECONNRESET in this test, but it's
      // timing-dependent, and thus unreliable. Any other errors
      // are just failures, though.
      if (er.code !== 'ECONNRESET')
        throw er;
    }).end();
  }
});

process.on('exit', function() {
  assert.ok(gotRequest);
});
