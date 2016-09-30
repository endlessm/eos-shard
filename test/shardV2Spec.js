/* Copyright 2015 Endless Mobile, Inc. */

/* This file is part of eos-shard.
 *
 * eos-shard is free software: you can redistribute it and/or modify
 * it under the terms of the GNU Lesser General Public License as published
 * by the Free Software Foundation, either version 2.1 of the License, or
 * (at your option) any later version.
 *
 * eos-shard is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public
 * License along with eos-shard.  If not, see
 * <http://www.gnu.org/licenses/>.
 */

const Gio = imports.gi.Gio;
const GObject = imports.gi.GObject;

const EosShard = imports.gi.EosShard;
const TestUtils = imports.utils;

describe('Basic Shard Writing', function () {
    let shard_path, ostream, shard_fd;
    beforeEach(function() {
        let [shard_file, iostream] = Gio.File.new_tmp('XXXXXXX.shard');
        ostream = iostream.get_output_stream();
        shard_path = shard_file.get_path();
        shard_fd = ostream.get_fd();
    });

    afterEach(function() {
        ostream.close(null);

        let file = Gio.File.new_for_path(shard_path);
        try {
            file.delete(null);
        } catch (e) {
            // we don't really care if the file wasn't created, causing
            // g_file_delete to throw
        }
    });

    describe('shards with single records', function() {
        beforeEach(function() {
            let shard_writer = new EosShard.WriterV2({ fd: shard_fd });

            let r = shard_writer.add_record('f572d396fae9206628714fb2ce00f72e94f2258f');
            shard_writer.add_blob_to_record(r, shard_writer.add_blob(EosShard.V2_BLOB_METADATA,
                                                                     TestUtils.getTestFile('f572d396fae9206628714fb2ce00f72e94f2258f.json'),
                                                                     'application/json',
                                                                     EosShard.BlobFlags.NONE));
            shard_writer.add_blob_to_record(r, shard_writer.add_blob(EosShard.V2_BLOB_DATA,
                                                                     TestUtils.getTestFile('f572d396fae9206628714fb2ce00f72e94f2258f.blob'),
                                                                     null,
                                                                     EosShard.BlobFlags.COMPRESSED_ZLIB));

            shard_writer.finish();
        });

        it('can create a shard file with a single record', function() {
            let shard_file = new EosShard.ShardFile({ path: shard_path });
            shard_file.init(null);
        });

        it('can find a record in a shard file', function() {
            let shard_file = new EosShard.ShardFile({ path: shard_path });
            shard_file.init(null);

            let record = shard_file.find_record_by_hex_name('f572d396fae9206628714fb2ce00f72e94f2258f');
            expect(record).not.toBe(null);
        });

        it('should return a null value if record is not found', function () {
            let shard_file = new EosShard.ShardFile({ path: shard_path });
            shard_file.init(null);

            let record = shard_file.find_record_by_hex_name('deadbeefdeadbeefdeadbeefdeadbeefbeef');
            expect(record).toEqual(null);
        });

        it('can read uncompressed contents from a shard file', function() {
            let shard_file = new EosShard.ShardFile({ path: shard_path });
            shard_file.init(null);

            let record = shard_file.find_record_by_hex_name('f572d396fae9206628714fb2ce00f72e94f2258f');
            expect(record).not.toBe(null);
            expect(record.metadata.get_flags() & EosShard.BlobFlags.COMPRESSED_ZLIB).not.toBeTruthy();

            let metadata = record.metadata.load_contents().get_data().toString();
            expect(metadata).toMatch(/eggs/);
        });

        it('can read compressed contents from a shard file', function() {
            let shard_file = new EosShard.ShardFile({ path: shard_path });
            shard_file.init(null);

            let record = shard_file.find_record_by_hex_name('f572d396fae9206628714fb2ce00f72e94f2258f');
            expect(record).not.toBe(null);
            expect(record.data.get_flags() & EosShard.BlobFlags.COMPRESSED_ZLIB).toBeTruthy();

            let data = record.data.load_contents().get_data().toString();
            expect(data).toMatch(/Lightsaber/);
        });
    });

    describe('multiple record shards', function() {
        beforeEach(function() {
            let shard_writer = new EosShard.WriterV2({ fd: shard_fd });

            let r = shard_writer.add_record('7d97e98f8af710c7e7fe703abc8f639e0ee507c4');
            shard_writer.add_blob_to_record(r, shard_writer.add_blob(EosShard.V2_BLOB_METADATA,
                                                                     TestUtils.getTestFile('7d97e98f8af710c7e7fe703abc8f639e0ee507c4.json'),
                                                                     'application/json',
                                                                     EosShard.BlobFlags.COMPRESSED_ZLIB));
            shard_writer.add_blob_to_record(r, shard_writer.add_blob(EosShard.V2_BLOB_DATA,
                                                                     TestUtils.getTestFile('7d97e98f8af710c7e7fe703abc8f639e0ee507c4.blob'),
                                                                     null,
                                                                     EosShard.BlobFlags.NONE));

            r = shard_writer.add_record('f572d396fae9206628714fb2ce00f72e94f2258f');
            shard_writer.add_blob_to_record(r, shard_writer.add_blob(EosShard.V2_BLOB_METADATA,
                                                                     TestUtils.getTestFile('f572d396fae9206628714fb2ce00f72e94f2258f.json'),
                                                                     'application/json',
                                                                     EosShard.BlobFlags.COMPRESSED_ZLIB));
            shard_writer.add_blob_to_record(r, shard_writer.add_blob(EosShard.V2_BLOB_DATA,
                                                                     TestUtils.getTestFile('f572d396fae9206628714fb2ce00f72e94f2258f.blob'),
                                                                     null,
                                                                     EosShard.BlobFlags.NONE));

            shard_writer.finish();
        });

        it('can create a shard file with multiple records', function() {
            let shard_file = new EosShard.ShardFile({ path: shard_path });
            shard_file.init(null);
        });

        it('can list records in a shard file', function() {
            let shard_file = new EosShard.ShardFile({ path: shard_path });
            shard_file.init(null);

            let records = shard_file.list_records();
            let record_names = records.map(function(record) { return record.get_hex_name(); });
            expect(record_names).toEqual(['7d97e98f8af710c7e7fe703abc8f639e0ee507c4',
                                          'f572d396fae9206628714fb2ce00f72e94f2258f']);
        });
    });

    describe('NULL errors', function() {
        it('handles NULLs in filenames correctly', function() {
            let shard_writer = new EosShard.WriterV2({ fd: shard_fd });

            // The record here has '00's which will translate to NULL bytes.
            let r = shard_writer.add_record('f500000000e9206628714fb2ce00f72e94f2258f');
            shard_writer.add_blob_to_record(r, shard_writer.add_blob(EosShard.V2_BLOB_METADATA,
                                                                     TestUtils.getTestFile('f572d396fae9206628714fb2ce00f72e94f2258f.json'),
                                                                     'application/json',
                                                                     EosShard.BlobFlags.NONE));
            shard_writer.add_blob_to_record(r, shard_writer.add_blob(EosShard.V2_BLOB_DATA,
                                                                     TestUtils.getTestFile('f572d396fae9206628714fb2ce00f72e94f2258f.blob'),
                                                                     null,
                                                                     EosShard.BlobFlags.NONE));
            shard_writer.finish();

            let shard_file = new EosShard.ShardFile({ path: shard_path });
            shard_file.init(null);

            let record = shard_file.find_record_by_hex_name('f500000000e9206628714fb2ce00f72e94f2258f');
            expect(record).not.toBeNull();
        });

        it('handles NULLs in checksums correctly', function() {
            // The string 'aaaaaaaaaaa' (eleven 'a' characters in a row),
            // without a newline at the end, has a NUL byte in its SHA-1 hash.
            // Use this to test whether our checksum logic is correct.
            //
            // Found by bash magic one-liner:
            //  $ for i in $(seq 1 100); do (printf 'a%.0s' $(seq 1 $i) | openssl dgst -sha1 -c | grep '00') && echo $i; done

            let shard_writer = new EosShard.WriterV2({ fd: shard_fd });

            let r = shard_writer.add_record('f572d396fae9206628714fb2ce00f72e94f2258f');
            shard_writer.add_blob_to_record(r, shard_writer.add_blob(EosShard.V2_BLOB_METADATA,
                                                                     TestUtils.getTestFile('f572d396fae9206628714fb2ce00f72e94f2258f.json'),
                                                                     'application/json',
                                                                     EosShard.BlobFlags.NONE));
            shard_writer.add_blob_to_record(r, shard_writer.add_blob(EosShard.V2_BLOB_DATA,
                                                                     TestUtils.getTestFile('nul_example'),
                                                                     null,
                                                                     EosShard.BlobFlags.NONE));
            shard_writer.finish();

            let shard_file = new EosShard.ShardFile({ path: shard_path });
            shard_file.init(null);

            let record = shard_file.find_record_by_hex_name('f572d396fae9206628714fb2ce00f72e94f2258f');
            expect(record).not.toBeNull();

            let data = record.data.load_contents().get_data().toString();
            expect(data).toEqual('aaaaaaaaaaa');
        });
    });

    describe('Alignment Conditions', function() {
        // This is to test a very specific error. Since we align shard contents
        // to 64 byte 'chunks', we want to make sure that there are no errors in
        // our math that allow us to write out invalid shards if we get the
        // wrong alignment. We do this simply by adjusting the content-type
        // field in the metadata blob, since it is variable-length. The rest
        // remain constant.

        it('aligns correctly', function() {
            for (var align = 0; align < 128; align++) {
                let content_type = Array(align).join('a');

                let shard_writer = new EosShard.WriterV2({ fd: shard_fd });
                let r = shard_writer.add_record('f572d396fae9206628714fb2ce00f72e94f2258f');
                shard_writer.add_blob_to_record(r, shard_writer.add_blob(EosShard.V2_BLOB_METADATA,
                                                                         TestUtils.getTestFile('f572d396fae9206628714fb2ce00f72e94f2258f.json'),
                                                                         content_type,
                                                                         EosShard.BlobFlags.NONE));
                shard_writer.add_blob_to_record(r, shard_writer.add_blob(EosShard.V2_BLOB_DATA,
                                                                         TestUtils.getTestFile('f572d396fae9206628714fb2ce00f72e94f2258f.blob'),
                                                                         'test',
                                                                         EosShard.BlobFlags.NONE));
                shard_writer.finish();

                let shard_file = new EosShard.ShardFile({ path: shard_path });
                shard_file.init(null);

                let record = shard_file.find_record_by_hex_name('f572d396fae9206628714fb2ce00f72e94f2258f');
                let metadata = record.metadata.load_contents().get_data().toString();
                expect(metadata).toMatch(/eggs/);
                let data = record.data.load_contents().get_data().toString();
                expect(data).toMatch(/Lightsaber/);
            }
        });
    });

    describe('Allow records with no metadata / data', function() {
        it('handles no metadata correctly', function() {
            let shard_writer = new EosShard.WriterV2({ fd: shard_fd });
            let r = shard_writer.add_record('f572d396fae9206628714fb2ce00f72e94f2258f');
            shard_writer.add_blob_to_record(r, shard_writer.add_blob(EosShard.V2_BLOB_DATA,
                                                                     TestUtils.getTestFile('f572d396fae9206628714fb2ce00f72e94f2258f.blob'),
                                                                     'test',
                                                                     EosShard.BlobFlags.NONE));
            shard_writer.finish();

            let shard_file = new EosShard.ShardFile({ path: shard_path });
            shard_file.init(null);

            let record = shard_file.find_record_by_hex_name('f572d396fae9206628714fb2ce00f72e94f2258f');
            expect(record.metadata).toBeNull();
            expect(record.data).not.toBeNull();
            let data_bytes = record.data.load_contents();
            expect(data_bytes).not.toBeNull();
            let data = data_bytes.get_data().toString();
            expect(data).toMatch(/Lightsaber/);
            let data_stream = record.data.get_stream();
            expect(data_stream).not.toBeNull();
        });

        it('handles no data correctly', function() {
            let shard_writer = new EosShard.WriterV2({ fd: shard_fd });
            let r = shard_writer.add_record('f572d396fae9206628714fb2ce00f72e94f2258f');
            shard_writer.add_blob_to_record(r, shard_writer.add_blob(EosShard.V2_BLOB_METADATA,
                                                                     TestUtils.getTestFile('f572d396fae9206628714fb2ce00f72e94f2258f.json'),
                                                                     'application/json',
                                                                     EosShard.BlobFlags.NONE));
            shard_writer.finish();

            let shard_file = new EosShard.ShardFile({ path: shard_path });
            shard_file.init(null);

            let record = shard_file.find_record_by_hex_name('f572d396fae9206628714fb2ce00f72e94f2258f');
            expect(record.metadata).not.toBeNull();
            expect(record.data).toBeNull();
            let metadata_bytes = record.metadata.load_contents();
            expect(metadata_bytes).not.toBeNull();
            let metadata = metadata_bytes.get_data().toString();
            expect(metadata).toMatch(/eggs/);
            let metadata_stream = record.metadata.get_stream();
            expect(metadata_stream).not.toBeNull();
        });
    });

    describe('streaming', function() {
        beforeEach(function () {
            let shard_writer = new EosShard.WriterV2({ fd: shard_fd });

            let r = shard_writer.add_record('7d97e98f8af710c7e7fe703abc8f639e0ee507c4');
            shard_writer.add_blob_to_record(r, shard_writer.add_blob(EosShard.V2_BLOB_METADATA,
                                                                     TestUtils.getTestFile('7d97e98f8af710c7e7fe703abc8f639e0ee507c4.json'),
                                                                     'application/json',
                                                                     EosShard.BlobFlags.COMPRESSED_ZLIB));
            shard_writer.add_blob_to_record(r, shard_writer.add_blob(EosShard.V2_BLOB_DATA,
                                                                     TestUtils.getTestFile('7d97e98f8af710c7e7fe703abc8f639e0ee507c4.blob'),
                                                                     null,
                                                                     EosShard.BlobFlags.COMPRESSED_ZLIB));

            r = shard_writer.add_record('f572d396fae9206628714fb2ce00f72e94f2258f');
            shard_writer.add_blob_to_record(r, shard_writer.add_blob(EosShard.V2_BLOB_METADATA,
                                                                     TestUtils.getTestFile('f572d396fae9206628714fb2ce00f72e94f2258f.json'),
                                                                     'application/json',
                                                                     EosShard.BlobFlags.COMPRESSED_ZLIB));
            shard_writer.add_blob_to_record(r, shard_writer.add_blob(EosShard.V2_BLOB_DATA,
                                                                     TestUtils.getTestFile('f572d396fae9206628714fb2ce00f72e94f2258f.blob'),
                                                                     null,
                                                                     EosShard.BlobFlags.NONE));

            shard_writer.finish();
        });

        it('should be seekable when uncompressed', function () {
            let shard_file = new EosShard.ShardFile({ path: shard_path });
            shard_file.init(null);

            let record = shard_file.find_record_by_hex_name('f572d396fae9206628714fb2ce00f72e94f2258f');
            let dataStream = record.data.get_stream();
            expect(GObject.type_is_a(dataStream, Gio.Seekable)).toBe(true);
        });

        it('should not be seekable when compressed', function () {
            let shard_file = new EosShard.ShardFile({ path: shard_path });
            shard_file.init(null);

            let record = shard_file.find_record_by_hex_name('7d97e98f8af710c7e7fe703abc8f639e0ee507c4');
            let dataStream = record.data.get_stream();
            expect(GObject.type_is_a(dataStream, Gio.Seekable)).toBe(false);
        });
    });

    describe('specific bugs', function () {
        it('handles interleaved compressed / uncompressed pairs correctly', function () {
            let shard_writer = new EosShard.WriterV2({ fd: shard_fd });
            let r = shard_writer.add_record('f572d396fae9206628714fb2ce00f72e94f2258f');
            shard_writer.add_blob_to_record(r, shard_writer.add_blob(EosShard.V2_BLOB_METADATA,
                                                                     TestUtils.getTestFile('f572d396fae9206628714fb2ce00f72e94f2258f.json'),
                                                                     'application/json',
                                                                     EosShard.BlobFlags.NONE));
            shard_writer.add_blob_to_record(r, shard_writer.add_blob(EosShard.V2_BLOB_DATA,
                                                                     TestUtils.getTestFile('words'),
                                                                     'application/json',
                                                                     EosShard.BlobFlags.COMPRESSED_ZLIB));

            r = shard_writer.add_record('f572d396fae9206628714fb2ce00f72e94f2259f');
            shard_writer.add_blob_to_record(r, shard_writer.add_blob(EosShard.V2_BLOB_METADATA,
                                                                     TestUtils.getTestFile('f572d396fae9206628714fb2ce00f72e94f2258f.json'),
                                                                     'application/json',
                                                                     EosShard.BlobFlags.NONE));
            shard_writer.add_blob_to_record(r, shard_writer.add_blob(EosShard.V2_BLOB_DATA,
                                                                     TestUtils.getTestFile('words'),
                                                                     'application/json',
                                                                     EosShard.BlobFlags.COMPRESSED_ZLIB));

            r = shard_writer.add_record('f572d396fae9206628714fb2ce00f72e94f225af');
            shard_writer.add_blob_to_record(r, shard_writer.add_blob(EosShard.V2_BLOB_METADATA,
                                                                     TestUtils.getTestFile('f572d396fae9206628714fb2ce00f72e94f2258f.json'),
                                                                     'application/json',
                                                                     EosShard.BlobFlags.NONE));
            shard_writer.add_blob_to_record(r, shard_writer.add_blob(EosShard.V2_BLOB_DATA,
                                                                     TestUtils.getTestFile('words'),
                                                                     'application/json',
                                                                     EosShard.BlobFlags.COMPRESSED_ZLIB));

            shard_writer.finish();

            let shard_file = new EosShard.ShardFile({ path: shard_path });
            shard_file.init(null);

            expect(function () {
                // Check that checksums match up.
                let record;
                record = shard_file.find_record_by_hex_name('f572d396fae9206628714fb2ce00f72e94f2258f');
                record.metadata.load_contents();
                record.data.load_contents();

                record = shard_file.find_record_by_hex_name('f572d396fae9206628714fb2ce00f72e94f2259f');
                record.metadata.load_contents();
                record.data.load_contents();

                record = shard_file.find_record_by_hex_name('f572d396fae9206628714fb2ce00f72e94f225af');
                record.metadata.load_contents();
                record.data.load_contents();
            }).not.toThrow();
        });

        it('works correctly with certain sized records', function () {
            // This is to test the issue where we go over a block boundary -- just
            // slightly -- after compression, and the code didn't take the header
            // size into account when calculating which blocks to chop.

            let shard_writer = new EosShard.WriterV2({ fd: shard_fd });
            let r = shard_writer.add_record('f572d396fae9206628714fb2ce00f72e94f2258f');

            // This blob is specially crafted to compress from 0x8000 bytes to
            // 0x1006 bytes. It was generated by:
            //
            // $ dd if=/dev/random of=random_data bs=1 count=3795
            // $ seq 8 | xargs -I -- cat random_data > random_data_8x
            // $ gzip -c < random_data_8x | wc -c
            // 4102
            //
            // Due to the nature of random data, it might take a few tries to
            // get something of that size, but since random data is uncompressable,
            // it should be near that target.

            shard_writer.add_blob_to_record(r, shard_writer.add_blob(EosShard.V2_BLOB_METADATA,
                                                                     TestUtils.getTestFile('random_data_8x'),
                                                                     'application/json',
                                                                     EosShard.BlobFlags.COMPRESSED_ZLIB));

            // The last blob never gets fallocated, so add one at the end.
            shard_writer.add_blob_to_record(r, shard_writer.add_blob(EosShard.V2_BLOB_DATA,
                                                                     TestUtils.getTestFile('words'),
                                                                     'application/json',
                                                                     EosShard.BlobFlags.COMPRESSED_ZLIB));

            shard_writer.finish();

            let shard_file = new EosShard.ShardFile({ path: shard_path });
            shard_file.init(null);

            expect(function () {
                // Check that checksums match up.
                let record;
                record = shard_file.find_record_by_hex_name('f572d396fae9206628714fb2ce00f72e94f2258f');
                record.metadata.load_contents();
                record.data.load_contents();
            }).not.toThrow();
        });
    });
});

