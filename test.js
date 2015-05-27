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

const EosShard = imports.gi.EosShard;

let shard_writer = new EosShard.Writer();
shard_writer.add_record("7d97e98f8af710c7e7fe703abc8f639e0ee507c4");
shard_writer.add_blob(EosShard.WriterBlob.METADATA,
                      Gio.File.new_for_path("foo/7d97e98f8af710c7e7fe703abc8f639e0ee507c4.json"),
                      EosShard.BlobFlags.COMPRESSED_ZLIB);
shard_writer.add_blob(EosShard.WriterBlob.DATA,
                      Gio.File.new_for_path("foo/7d97e98f8af710c7e7fe703abc8f639e0ee507c4.blob"),
                      EosShard.BlobFlags.NONE);
shard_writer.write("test.shard");

let shard = new EosShard.ShardFile({ path: 'test.shard' });
if (!shard.init(null))
    throw "failed to init";
let n = shard.list_records();
let r = n[0];
let x = r.get_hex_name();

print(x);
print(shard.find_record_by_hex_name(x));

let stdout = Gio.UnixOutputStream.new(1, false);

// stream the metadata and data to stdout
stdout.splice(r.metadata.get_stream(), Gio.OutputStreamSpliceFlags.NONE, null)
stdout.splice(r.data.get_stream(), Gio.OutputStreamSpliceFlags.NONE, null)
