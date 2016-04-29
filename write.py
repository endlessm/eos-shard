
import os
import struct
import string
import math

import gi
gi.require_version('EosShard', '0')
from gi.repository import EosShard

def rot13(s):
    t = string.maketrans( 
        "ABCDEFGHIJKLMabcdefghijklmNOPQRSTUVWXYZnopqrstuvwxyz", 
        "NOPQRSTUVWXYZnopqrstuvwxyzABCDEFGHIJKLMabcdefghijklm")
    return string.translate(s, t)

words = [w.rstrip().lower() for w in open('/usr/share/dict/words')]
words = sorted(words)[:10]

def write_jlist():
    writer = EosShard.Writer()
    at = EosShard.AliasTable.new(len(words))
    for i, word in enumerate(words):
        key = word
        value = rot13(key)
        at.add_entry(key, value)
    at.write_to_shard(writer)
    writer.write('foo.shard')

def test():
    shard = EosShard.ShardFile(path='foo.shard')
    shard.init(None)
    at = EosShard.AliasTable.new_from_shard(shard)
    results = at.find_entries(words)
    for word, (key, value) in zip(words, sorted(results.iteritems())):
        assert key == word
        assert(rot13(key) == value)

write_jlist()
test()
