var assert = require('assert');
var httpc = require('bindings')('binding');

assert(httpc.get('https://google.com'));
