'use strict';
const common = require('../common');

if (!common.hasCrypto)
  common.skip('missing crypto');

if (!process.features.tls_sni)
  common.skip('node compiled without OpenSSL or with old OpenSSL version.');

const assert = require('assert');
const tls = require('tls');
const fixtures = require('../common/fixtures');


function loadPEM(n) {
  return fixtures.readKey(`${n}.pem`);
}

const serverOptions = {
  key: loadPEM('agent2-key'),
  cert: loadPEM('agent2-cert')
};

const SNIContexts = {
  'a.example.com': {
    key: loadPEM('agent1-key'),
    cert: loadPEM('agent1-cert')
  },
  'asterisk.test.com': {
    key: loadPEM('agent3-key'),
    cert: loadPEM('agent3-cert')
  },
  'chain.example.com': {
    key: loadPEM('agent6-key'),
    // NOTE: Contains ca3 chain cert
    cert: loadPEM('agent6-cert')
  }
};

const clientsOptions = [{
  port: undefined,
  ca: [loadPEM('ca1-cert')],
  servername: 'a.example.com',
  rejectUnauthorized: false
}, {
  port: undefined,
  ca: [loadPEM('ca2-cert')],
  servername: 'b.test.com',
  rejectUnauthorized: false
}, {
  port: undefined,
  ca: [loadPEM('ca2-cert')],
  servername: 'a.b.test.com',
  rejectUnauthorized: false
}, {
  port: undefined,
  ca: [loadPEM('ca1-cert')],
  servername: 'c.wrong.com',
  rejectUnauthorized: false
}, {
  port: undefined,
  ca: [loadPEM('ca1-cert')],
  servername: 'chain.example.com',
  rejectUnauthorized: false
}];

const serverResults = [];
const clientResults = [];

const server = tls.createServer(serverOptions, function(c) {
  serverResults.push(c.servername);
});

server.addContext('a.example.com', SNIContexts['a.example.com']);
server.addContext('*.test.com', SNIContexts['asterisk.test.com']);
server.addContext('chain.example.com', SNIContexts['chain.example.com']);

server.listen(0, startTest);

function startTest() {
  let i = 0;
  function start() {
    // No options left
    if (i === clientsOptions.length)
      return server.close();

    const options = clientsOptions[i++];
    options.port = server.address().port;
    const client = tls.connect(options, function() {
      clientResults.push(
        client.authorizationError &&
        /Hostname\/IP doesn't/.test(client.authorizationError));
      client.destroy();

      // Continue
      start();
    });
  }

  start();
}

process.on('exit', function() {
  assert.deepStrictEqual(serverResults, [
    'a.example.com', 'b.test.com', 'a.b.test.com', 'c.wrong.com',
    'chain.example.com'
  ]);
  assert.deepStrictEqual(clientResults, [true, true, false, false, true]);
});
