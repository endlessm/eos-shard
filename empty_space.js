const EosShard = imports.gi.EosShard;

let s = new EosShard.ShardFile({
    path: 'encyclopedia-tiny.shard',
});
print('init');
s.init(null);

// store all blobs as offset/length pairs
let blocks = [];
print('list');
s.list_records().forEach(record => {
    let data = record.data;
    let metadata = record.metadata;

    if (data)
        blocks.push({ offset: data.get_offset(), length: data.get_packed_size() });
    if (metadata)
        blocks.push({ offset: metadata.get_offset(), length: metadata.get_packed_size() });
});
print('sort');
blocks.sort((a, b) => {
    if (a.offset < b.offset) return -1;
    else return 1;
});

let offset0 = blocks[0].offset;
let offsetN = blocks.pop().offset;
let lengthSum = blocks.reduce((a, block) => a + block.length, 0);
let emptyBytes = (offsetN - (offset0 + lengthSum));
let emptyKBytes = emptyBytes / 1000;
print(emptyKBytes, 'KB');
