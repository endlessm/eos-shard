const Epak = imports.gi.Epak;

let pak = new Epak.Pak({ path: 'foo.epak' });
pak.init(null);
let n = pak.list_entries();
print(n[0].get_hex_name());
print(n[0].read_data().get_data());
print(n[0].read_metadata().get_data());

