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

const GLib = imports.gi.GLib;
const Gio = imports.gi.Gio;

const EosShard = imports.gi.EosShard;

function getTestFile(fn) {
    let datadir = GLib.getenv('G_TEST_SRCDIR');
    if (!datadir)
        datadir = '';
    return Gio.File.new_for_path(GLib.build_filenamev([datadir, 'test/data', fn]));
}

describe('Basic Shard Writing', function () {
    afterEach(function() {
        let file = Gio.File.new_for_path('test.shard');
        file.delete(null);
    });

    describe('shards with single records', function() {
        beforeEach(function() {
            let shard_writer = new EosShard.Writer();

            shard_writer.add_record('f572d396fae9206628714fb2ce00f72e94f2258f');
            shard_writer.add_blob(EosShard.WriterBlob.METADATA,
                                  getTestFile('f572d396fae9206628714fb2ce00f72e94f2258f.json'),
                                  'application/json',
                                  EosShard.BlobFlags.NONE);
            shard_writer.add_blob(EosShard.WriterBlob.DATA,
                                  getTestFile('f572d396fae9206628714fb2ce00f72e94f2258f.blob'),
                                  null,
                                  EosShard.BlobFlags.COMPRESSED_ZLIB);

            shard_writer.write('test.shard');
        });

        it('can create a shard file with a single record', function() {
            let shard_file = new EosShard.ShardFile({ path: 'test.shard' });
            shard_file.init(null);
        });

        it('can find a record in a shard file', function() {
            let shard_file = new EosShard.ShardFile({ path: 'test.shard' });
            shard_file.init(null);

            let record = shard_file.find_record_by_hex_name('f572d396fae9206628714fb2ce00f72e94f2258f');
            expect(record).not.toBe(null);
        });

        it('can read uncompressed contents from a shard file', function() {
            let shard_file = new EosShard.ShardFile({ path: 'test.shard' });
            shard_file.init(null);

            let record = shard_file.find_record_by_hex_name('f572d396fae9206628714fb2ce00f72e94f2258f');
            expect(record).not.toBe(null);
            expect(record.metadata.get_flags() & EosShard.BlobFlags.COMPRESSED_ZLIB).not.toBeTruthy();

            let metadata = record.metadata.load_contents().get_data().toString();
            expect(metadata).toMatch(/eggs/);
        });

        it('can read compressed contents from a shard file', function() {
            let shard_file = new EosShard.ShardFile({ path: 'test.shard' });
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
            let shard_writer = new EosShard.Writer();

            shard_writer.add_record('7d97e98f8af710c7e7fe703abc8f639e0ee507c4');
            shard_writer.add_blob(EosShard.WriterBlob.METADATA,
                                  getTestFile('7d97e98f8af710c7e7fe703abc8f639e0ee507c4.json'),
                                  'application/json',
                                  EosShard.BlobFlags.COMPRESSED_ZLIB);
            shard_writer.add_blob(EosShard.WriterBlob.DATA,
                                  getTestFile('7d97e98f8af710c7e7fe703abc8f639e0ee507c4.blob'),
                                  null,
                                  EosShard.BlobFlags.NONE);

            shard_writer.add_record('f572d396fae9206628714fb2ce00f72e94f2258f');
            shard_writer.add_blob(EosShard.WriterBlob.METADATA,
                                  getTestFile('f572d396fae9206628714fb2ce00f72e94f2258f.json'),
                                  'application/json',
                                  EosShard.BlobFlags.COMPRESSED_ZLIB);
            shard_writer.add_blob(EosShard.WriterBlob.DATA,
                                  getTestFile('f572d396fae9206628714fb2ce00f72e94f2258f.blob'),
                                  null,
                                  EosShard.BlobFlags.NONE);

            shard_writer.write('test.shard');
        });

        it('can create a shard file with multiple records', function() {
            let shard_file = new EosShard.ShardFile({ path: 'test.shard' });
            shard_file.init(null);
        });

        it('can list records in a shard file', function() {
            let shard_file = new EosShard.ShardFile({ path: 'test.shard' });
            shard_file.init(null);

            let records = shard_file.list_records();
            let record_names = records.map(function(record) { return record.get_hex_name(); });
            expect(record_names).toEqual(['7d97e98f8af710c7e7fe703abc8f639e0ee507c4',
                                          'f572d396fae9206628714fb2ce00f72e94f2258f']);
        });
    });

    describe('NULL errors', function() {
        it('handles NULLs in filenames correctly', function() {
            let shard_writer = new EosShard.Writer();
            // The record here has '00's which will translate to NULL bytes.
            shard_writer.add_record('f500000000e9206628714fb2ce00f72e94f2258f');
            shard_writer.add_blob(EosShard.WriterBlob.METADATA,
                                  getTestFile('f572d396fae9206628714fb2ce00f72e94f2258f.json'),
                                  'application/json',
                                  EosShard.BlobFlags.NONE);
            shard_writer.add_blob(EosShard.WriterBlob.DATA,
                                  getTestFile('f572d396fae9206628714fb2ce00f72e94f2258f.blob'),
                                  null,
                                  EosShard.BlobFlags.NONE);
            shard_writer.write('test.shard');

            let shard_file = new EosShard.ShardFile({ path: 'test.shard' });
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

            let shard_writer = new EosShard.Writer();
            shard_writer.add_record('f572d396fae9206628714fb2ce00f72e94f2258f');
            shard_writer.add_blob(EosShard.WriterBlob.METADATA,
                                  getTestFile('f572d396fae9206628714fb2ce00f72e94f2258f.json'),
                                  'application/json',
                                  EosShard.BlobFlags.NONE);
            shard_writer.add_blob(EosShard.WriterBlob.DATA,
                                  getTestFile('nul_example'),
                                  'text/plain',
                                  EosShard.BlobFlags.NONE);
            shard_writer.write('test.shard');

            let shard_file = new EosShard.ShardFile({ path: 'test.shard' });
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

                let shard_writer = new EosShard.Writer();
                shard_writer.add_record('f572d396fae9206628714fb2ce00f72e94f2258f');
                shard_writer.add_blob(EosShard.WriterBlob.METADATA,
                                      getTestFile('f572d396fae9206628714fb2ce00f72e94f2258f.json'),
                                      content_type,
                                      EosShard.BlobFlags.NONE);
                shard_writer.add_blob(EosShard.WriterBlob.DATA,
                                      getTestFile('f572d396fae9206628714fb2ce00f72e94f2258f.blob'),
                                      'test',
                                      EosShard.BlobFlags.NONE);
                shard_writer.write('test.shard');

                let shard_file = new EosShard.ShardFile({ path: 'test.shard' });
                shard_file.init(null);

                let record = shard_file.find_record_by_hex_name('f572d396fae9206628714fb2ce00f72e94f2258f');
                let metadata = record.metadata.load_contents().get_data().toString();
                expect(metadata).toMatch(/eggs/);
                let data = record.data.load_contents().get_data().toString();
                expect(data).toMatch(/Lightsaber/);
            }
        });
    });
});

