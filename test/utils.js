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

function writeTestShard(shard_path) {
    let shard_writer = new EosShard.Writer();

    shard_writer.add_record('7d97e98f8af710c7e7fe703abc8f639e0ee507c4');
    shard_writer.add_blob(EosShard.WriterBlob.METADATA,
                          getTestFile('7d97e98f8af710c7e7fe703abc8f639e0ee507c4.json'),
                          'application/json',
                          EosShard.BlobFlags.COMPRESSED_ZLIB);
    shard_writer.add_blob(EosShard.WriterBlob.DATA,
                          getTestFile('7d97e98f8af710c7e7fe703abc8f639e0ee507c4.blob'),
                          null,
                          EosShard.BlobFlags.COMPRESSED_ZLIB);

    shard_writer.add_record('f572d396fae9206628714fb2ce00f72e94f2258f');
    shard_writer.add_blob(EosShard.WriterBlob.METADATA,
                          getTestFile('f572d396fae9206628714fb2ce00f72e94f2258f.json'),
                          'application/json',
                          EosShard.BlobFlags.COMPRESSED_ZLIB);
    shard_writer.add_blob(EosShard.WriterBlob.DATA,
                          getTestFile('f572d396fae9206628714fb2ce00f72e94f2258f.blob'),
                          null,
                          EosShard.BlobFlags.NONE);

    shard_writer.write(shard_path);
}
