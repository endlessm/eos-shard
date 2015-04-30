
const Gio = imports.gi.Gio;

const Epak = imports.gi.Epak;

let pakw = new Epak.Writer();
pakw.add_entry("7d97e98f8af710c7e7fe703abc8f639e0ee507c4",
               Gio.File.new_for_path("foo/7d97e98f8af710c7e7fe703abc8f639e0ee507c4.json"),
               Epak.BlobFlags.COMPRESSED_ZLIB,
               Gio.File.new_for_path("foo/7d97e98f8af710c7e7fe703abc8f639e0ee507c4.blob"),
               Epak.BlobFlags.NONE);
pakw.write("test.epak");

let pak = new Epak.Pak({ path: 'test.epak' });
if (!pak.init(null))
    throw "failed to init";
let n = pak.list_entries();
let e = n[0];
let x = e.get_hex_name();

print(x);
print(pak.find_entry_by_hex_name(x));

let stdout = Gio.UnixOutputStream.new(1, false);

// stream the metadata and data to stdout
stdout.splice(e.metadata.get_stream(), Gio.OutputStreamSpliceFlags.NONE, null)
stdout.splice(e.data.get_stream(), Gio.OutputStreamSpliceFlags.NONE, null)
