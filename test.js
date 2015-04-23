const Epak = imports.gi.Epak;

let pak = new Epak.Pak({ path: 'foo.epak' });
pak.init(null);
let n = pak.list_entries();
let e = n[0];
let x = e.get_hex_name();

print(x);
print(pak.find_entry_by_hex_name(x));

print(e.read_data().get_data());
print(e.read_metadata().get_data());

