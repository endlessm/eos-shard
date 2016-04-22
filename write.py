
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

CHUNK_SIZE = int(math.sqrt(len(words)))
def write_jlist():
    inf = open('/usr/share/dict/words')

    f = open('out.jlist', 'wb')
    f.write('JListV1 ')
    f.write(struct.pack('<Q', 0))

    f.seek(0x20, os.SEEK_SET)

    offsets = []
    for i, word in enumerate(words):
        if i % CHUNK_SIZE == 0:
            offsets.append(f.tell())
        key = word
        value = rot13(key)

        f.write(key + '\0')
        f.write(value + '\0')
    offsets.append(f.tell())

    offs_tbl = f.tell()
    f.write(struct.pack('<H', len(offsets) - 1))
    for offset, offset_next in zip(offsets, offsets[1:]):
        f.write(struct.pack('<QQ', offset, offset_next - offset))

    f.seek(8, os.SEEK_SET)
    f.write(struct.pack('<Q', offs_tbl))

def test(fp_rate):
    b = EosShard.BloomFilter.new_for_params(len(words), fp_rate)
    for i, word in enumerate(words):
        b.add(word)
    jlist = EosShard.BloomFilter.test_with_jlist(b, words, len(words))

write_jlist()
test(0.01)
