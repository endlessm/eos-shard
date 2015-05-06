
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
