/* Copyright 2015 Endless Mobile, Inc. */

/* This file is part of eos-shard.
 *
 * eos-shard is free software: you can redistribute it and/or modify
 * it under the terms of the GNU Lesser General Public License as published
 * by the Free Software Foundation, either version 2.1 of the License, or
 * (at your option) any later version.
 *
 * eos-shard is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public
 * License along with eos-shard.  If not, see
 * <http://www.gnu.org/licenses/>.
 */

const Gio = imports.gi.Gio;

const EosShard = imports.gi.EosShard;
const TestUtils = imports.utils;

// Returns a limited list of lowercased, sorted list of words from /usr/share/dict
function read_usr_share_dict (limit) {
    let f = Gio.File.new_for_path('/usr/share/dict/words');
    let text = f.load_contents(null).toString();
    let words = text.split('\n').map((word) => word.toLowerCase());
    if (limit === undefined) limit = words.length;
    return words.sort().slice(0, limit);
}

describe('Dictionary', function () {
    let dict_path, dict_stream, dict_fd;
    beforeEach(function() {
        let [dict_file, iostream] = Gio.File.new_tmp('XXXXXXX.dict');
        dict_stream = iostream.get_output_stream();
        dict_path = dict_file.get_path();
        dict_fd = dict_stream.get_fd();
    });

    afterEach(function() {
        let file = Gio.File.new_for_path(dict_path);
        try {
            file.delete(null);
        } catch (e) {
            // we don't really care if the file wasn't created, causing
            // g_file_delete to throw
        }
    });

    describe('Writer basics', function () {
        it('can handle 0 entries', function () {
            let w = EosShard.DictionaryWriter.new_for_stream(dict_stream, 0);
            w.begin();
            w.finish();

            let d = EosShard.Dictionary.new_for_fd(dict_fd, 0);
            expect(d.lookup_key('hi')).toEqual(null);
        });

        it('can handle 1 entries', function () {
            let w = EosShard.DictionaryWriter.new_for_stream(dict_stream, 1);
            w.begin();
            w.add_entry('foo', 'bar');
            w.finish();

            let d = EosShard.Dictionary.new_for_fd(dict_fd, 0);
            expect(d.lookup_key('foo')).toEqual('bar');
        });

        it('can handle 1 entries', function () {
            let w = EosShard.DictionaryWriter.new_for_stream(dict_stream, 1);
            w.begin();
            w.add_entry('foo', 'bar');
            w.finish();

            let d = EosShard.Dictionary.new_for_fd(dict_fd, 0);
            expect(d.lookup_key('foo')).toEqual('bar');
        });

        it('throws an error when too few entries are added', function () {
            let w = EosShard.DictionaryWriter.new_for_stream(dict_stream, 2);
            w.begin();
            w.add_entry('a', 'foo');
            expect(() => w.finish()).toThrow();
        });

        it('throws an error when too many entries are added', function () {
            let w = EosShard.DictionaryWriter.new_for_stream(dict_stream, 1);
            w.begin();
            w.add_entry('a', 'foo');
            w.add_entry('b', 'bar');
            expect(() => w.finish()).toThrow();
        });

        it('throws an error when entries are added out of order', function () {
            let w = EosShard.DictionaryWriter.new_for_stream(dict_stream, 2);
            w.begin();
            w.add_entry('b', 'foo');
            expect(() => (w.add_entry('a', 'bar'))).toThrow();
            w.add_entry('c', 'baz');
            w.finish();

            let d = EosShard.Dictionary.new_for_fd(dict_fd, 0);
            expect(d.lookup_key('a')).toEqual(null);
            expect(d.lookup_key('b')).toEqual('foo');
            expect(d.lookup_key('c')).toEqual('baz');
        });
    });

    describe('stress test', function () {
        let dictionary, words;
        beforeEach(function () {
            words = read_usr_share_dict(1000);
            let w = EosShard.DictionaryWriter.new_for_stream(dict_stream, words.length);
            w.begin();
            words.forEach((word) => w.add_entry(word, word.toUpperCase()));
            w.finish();
            dictionary = EosShard.Dictionary.new_for_fd(dict_fd, 0);
        });

        it('never reports false negatives', function () {
            for (let word of words) {
                expect(dictionary.lookup_key(word)).toEqual(word.toUpperCase());
            }
        });

        it('never reports false positives', function () {
            for (let word of words) {
                word += ' fake';
                expect(dictionary.lookup_key(word)).toEqual(null);
            }
        });
    });
});
