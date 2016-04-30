
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
words = sorted(words)

def write_jlist():
    writer = EosShard.Writer()
    at = EosShard.AliasTable.new(len(words))
    for word in words:
        at.add_entry(word, rot13(word))
    at.write_to_shard(writer)
    writer.write('foo.shard')

def test():
    shard = EosShard.ShardFile(path='foo.shard')
    shard.init(None)
    at = EosShard.AliasTable.new_from_shard(shard)
    results = at.find_entries(words)
    for word, value in results.iteritems():
        assert(value == rot13(word))

write_jlist()
test()
