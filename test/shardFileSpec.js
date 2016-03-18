const Gio = imports.gi.Gio;
const GLib = imports.gi.GLib;

const EosShard = imports.gi.EosShard;
const TestUtils = imports.utils;

describe('Shard file', function () {
    let shard_path;

    beforeEach(function () {
        let [shard_file, iostream] = Gio.File.new_tmp('XXXXXXX.shard');
        shard_path = shard_file.get_path();
        TestUtils.writeTestShard(shard_path);
    });

    afterEach(function () {
        let file = Gio.File.new_for_path(shard_path);
        try {
            file.delete(null);
        } catch (e) {
            // we don't really care if the file wasn't created, causing
            // g_file_delete to throw
        }
    });

    it('should not be set to an error state if async_init() is cancelled', function (done) {
        let shardFile = new EosShard.ShardFile({
            path: shard_path,
        });
    
        let cancellable1 = new Gio.Cancellable();
        let cancellable2 = new Gio.Cancellable();

        shardFile.init_async(0, cancellable1, (obj, res) => {
            expect(() => obj.init_finish(res)).toThrow();
        });

        shardFile.init_async(0, cancellable2, (obj, res) => {
            expect(() => obj.init_finish(res)).not.toThrow();
            done();
        });

        GLib.idle_add(GLib.PRIORITY_HIGH, function () {
            cancellable1.cancel();
        });
    });
});
