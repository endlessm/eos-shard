/* Copyright 2015 Endless Mobile, Inc. */

/* This file is part of epak.
 *
 * epak is free software: you can redistribute it and/or modify
 * it under the terms of the GNU Lesser General Public License as published
 * by the Free Software Foundation, either version 2.1 of the License, or
 * (at your option) any later version.
 *
 * epak is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public
 * License along with epak.  If not, see
 * <http://www.gnu.org/licenses/>.
 */

const Gio = imports.gi.Gio;

const Epak = imports.gi.Epak;

let pakw = new Epak.Writer();
pakw.add_record("7d97e98f8af710c7e7fe703abc8f639e0ee507c4",
                Gio.File.new_for_path("foo/7d97e98f8af710c7e7fe703abc8f639e0ee507c4.json"),
                Epak.BlobFlags.COMPRESSED_ZLIB,
                Gio.File.new_for_path("foo/7d97e98f8af710c7e7fe703abc8f639e0ee507c4.blob"),
                Epak.BlobFlags.NONE);
pakw.write("test.epak");

let pak = new Epak.Pak({ path: 'test.epak' });
if (!pak.init(null))
    throw "failed to init";
let n = pak.list_records();
let r = n[0];
let x = r.get_hex_name();

print(x);
print(pak.find_record_by_hex_name(x));

let stdout = Gio.UnixOutputStream.new(1, false);

// stream the metadata and data to stdout
stdout.splice(r.metadata.get_stream(), Gio.OutputStreamSpliceFlags.NONE, null)
stdout.splice(r.data.get_stream(), Gio.OutputStreamSpliceFlags.NONE, null)
