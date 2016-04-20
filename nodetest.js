var Bloom = require('bloomfilter');

var n = 1000000;
var p = 0.001;
var optimal_m = Math.ceil(-1 * (n * Math.log(p)) / (Math.LN2 * Math.LN2));
var m = Math.ceil(optimal_m / 32) * 32;
var k = Math.ceil((m/n)*Math.LN2);
console.log('m', m, 'k', k);
var b = new Bloom.BloomFilter(m, k);

console.time('insert')
for (var i=0; i<n; i++)
    b.add('' + i);
console.timeEnd('insert')

var false_positives = 0;
var false_negatives = 0;
console.time('test1')
for (var i=0; i<n; i++) {
    if (b.test('f' + i))
        false_positives++;
}
console.timeEnd('test1');
console.time('test2')
for (var i=0; i<n; i++) {
    if (!b.test('' + i))
        false_negatives++;
}
console.timeEnd('test2');

console.log('false positives: ' + (false_positives/n) * 100 + '% (expected ' + p * 100 + '%)');
console.log('false negatives: ' + (false_negatives/n) * 100 + '% (expected 0%)');
