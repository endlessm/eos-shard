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

describe('EosShardBlobStream', function () {
    afterEach(function () {
        let file = Gio.File.new_for_path(shard_path);
        try {
            file.delete(null);
        } catch (e) {
            // we don't really care if the file wasn't created, causing
            // g_file_delete to throw
        }
    });

    let shard_path;
    beforeEach(function () {
        let shard_writer = new EosShard.Writer();
        let [shard_file, iostream] = Gio.File.new_tmp('XXXXXXX.shard');
        shard_path = shard_file.get_path();

        shard_writer.add_record('7d97e98f8af710c7e7fe703abc8f639e0ee507c4');
        shard_writer.add_blob(EosShard.WriterBlob.METADATA,
                              TestUtils.getTestFile('7d97e98f8af710c7e7fe703abc8f639e0ee507c4.json'),
                              'application/json',
                              EosShard.BlobFlags.COMPRESSED_ZLIB);
        shard_writer.add_blob(EosShard.WriterBlob.DATA,
                              TestUtils.getTestFile('7d97e98f8af710c7e7fe703abc8f639e0ee507c4.blob'),
                              null,
                              EosShard.BlobFlags.COMPRESSED_ZLIB);

        shard_writer.add_record('f572d396fae9206628714fb2ce00f72e94f2258f');
        shard_writer.add_blob(EosShard.WriterBlob.METADATA,
                              TestUtils.getTestFile('f572d396fae9206628714fb2ce00f72e94f2258f.json'),
                              'application/json',
                              EosShard.BlobFlags.COMPRESSED_ZLIB);
        shard_writer.add_blob(EosShard.WriterBlob.DATA,
                              TestUtils.getTestFile('f572d396fae9206628714fb2ce00f72e94f2258f.blob'),
                              null,
                              EosShard.BlobFlags.NONE);

        shard_writer.write(shard_path);
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

    it('should be able to read uncompressed data', function() {
        let shard_file = new EosShard.ShardFile({ path: shard_path });
        shard_file.init(null);

        let record = shard_file.find_record_by_hex_name('f572d396fae9206628714fb2ce00f72e94f2258f');
        let dataStream = record.data.get_stream();
        let bytes = dataStream.read_bytes(0x100, null);
        let data = bytes.get_data().toString();
        expect(data).toMatch(/Lightsaber/);
    });

    it('should be able to read compressed data', function() {
        let shard_file = new EosShard.ShardFile({ path: shard_path });
        shard_file.init(null);

        let record = shard_file.find_record_by_hex_name('7d97e98f8af710c7e7fe703abc8f639e0ee507c4');
        let dataStream = record.data.get_stream();
        let bytes = dataStream.read_bytes(0x100, null);
        let data = bytes.get_data().toString();
        expect(data).toMatch(/hello/);
    });

    it('should be able to read uncompressed data from a mmap\'d shard', function() {
        let shard_file = new EosShard.ShardFile({ path: shard_path });
        shard_file.init(null);
        shard_file.mmap();

        let record = shard_file.find_record_by_hex_name('f572d396fae9206628714fb2ce00f72e94f2258f');
        let dataStream = record.data.get_stream();
        expect(GObject.type_is_a(dataStream, Gio.MemoryInputStream)).toBe(true);
        let bytes = dataStream.read_bytes(0x100, null);
        let data = bytes.get_data().toString();
        expect(data).toMatch(/Lightsaber/);
    });

    it('should be able to read compressed data from a mmap\'d shard', function() {
        let shard_file = new EosShard.ShardFile({ path: shard_path });
        shard_file.init(null);
        shard_file.mmap();

        let record = shard_file.find_record_by_hex_name('7d97e98f8af710c7e7fe703abc8f639e0ee507c4');
        let dataStream = record.data.get_stream();
        let bytes = dataStream.read_bytes(0x100, null);
        let data = bytes.get_data().toString();
        expect(data).toMatch(/hello/);
    });
});
