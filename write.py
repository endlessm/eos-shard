
import os
import struct
import string
import math

import gi
gi.require_version('EosShard', '0')
from gi.repository import Gio, EosShard

def rot13(s):
    t = string.maketrans(
        "ABCDEFGHIJKLMabcdefghijklmNOPQRSTUVWXYZnopqrstuvwxyz",
        "NOPQRSTUVWXYZnopqrstuvwxyzABCDEFGHIJKLMabcdefghijklm")
    return string.translate(s, t)

words = [w.rstrip().lower() for w in open('/usr/share/dict/words')]
words = sorted(words)[:10]

def write_jlist():
    writer = EosShard.Writer()

    file, io_stream = Gio.File.new_tmp('jlist-XXXXXX')
    stream = io_stream.get_output_stream()

    jlw = EosShard.JListWriter.new_for_stream(stream, len(words))
    jlw.begin()
    for word in words:
        jlw.add_entry(word, rot13(word))
    jlw.finish()

    writer.add_record('0615faea17a5453e6a002cc82bc6f124f7153213')
    writer.add_blob(EosShard.WriterBlob.DATA,
                    file,
                    'application/x-endlessm-jlist',
                    EosShard.BlobFlags.NONE)
    writer.write('foo.shard')

def test():
    shard = EosShard.ShardFile(path='foo.shard')
    shard.init(None)
    record = shard.find_record_by_hex_name('0615faea17a5453e6a002cc82bc6f124f7153213')
    jl = record.data.load_as_jlist()

    for word in words:
        value = jl.lookup_key(word)
        assert(value == rot13(word))

    for word in ['asdfjoit4ua', 'jtrhraljvd cx;', 'adsofrea;iukmds']:
        value = jl.lookup_key(word)
        assert(value is None)

write_jlist()
test()
