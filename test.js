
const Gio = imports.gi.Gio;

const Epak = imports.gi.Epak;

let pakw = new Epak.Writer();
pakw.add_entry("7d97e98f8af710c7e7fe703abc8f639e0ee507c4",
               Gio.File.new_for_path("foo/7d97e98f8af710c7e7fe703abc8f639e0ee507c4.json"),
               Epak.BlobFlags.BLOB_FLAG_COMPRESSED_ZLIB,
               Gio.File.new_for_path("foo/7d97e98f8af710c7e7fe703abc8f639e0ee507c4.blob"),
               Epak.BlobFlags.BLOB_FLAG_COMPRESSED_ZLIB);
pakw.write("fart.epak");

let pak = new Epak.Pak({ path: 'fart.epak' });
if (!pak.init(null))
    throw "failed to init";
let n = pak.list_entries();
let e = n[0];
let x = e.get_hex_name();

print(x);
print(pak.find_entry_by_hex_name(x));

print(e.metadata.load_contents().get_data());
print(e.data.load_contents().get_data());
