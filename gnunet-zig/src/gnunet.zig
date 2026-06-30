// Author: Martin Schanzenbach; License: AGPL3+

const gnunet = @cImport({
    @cInclude("gnunet/gnunet_util_lib.h");
    @cInclude("gnunet/gnunet_identity_service.h");
});

const std = @import("std");
const print = std.debug.print;

var id_svc: ?*gnunet.GNUNET_IDENTITY_Handle = undefined;

pub fn do_shutdown(cls: ?*anyopaque) callconv(.C) void {
    _ = cls;
    gnunet.GNUNET_IDENTITY_disconnect(id_svc);
}

pub fn id_cb(cls: ?*anyopaque, ego: ?*gnunet.GNUNET_IDENTITY_Ego, ctx: [*c]?*anyopaque, name: [*c]const u8) callconv(.C) void {
    _ = ctx;
    _ = cls;
    if (ego == null and name != null) {
        // Unreachable only in this demo, I think!
        unreachable;
    }
    if (ego == null) { // last iteration
        gnunet.GNUNET_SCHEDULER_shutdown();
        return;
    }
    // Remove to see fancy error
    if (name == null) {
        return;
    }
    var pk: gnunet.GNUNET_CRYPTO_PublicKey = .{};
    gnunet.GNUNET_IDENTITY_ego_get_public_key(ego, &pk);
    const pk_str = gnunet.GNUNET_CRYPTO_public_key_to_string(&pk);
    defer std.c.free(pk_str);
    std.debug.print("{s} - {s}\n", .{ name, pk_str });
}

pub fn run(cls: ?*anyopaque, args: [*c]const [*c]u8, cfgfile: [*c]const u8, cfg: ?*const gnunet.GNUNET_CONFIGURATION_Handle) callconv(.C) void {
    // Explicit discard, C compiler usually only warns, zig errors out!
    _ = cfgfile;
    _ = args;
    _ = cls;
    // This does a lot of stuff internally, opening UDS, sending/receiving messages etc.
    id_svc = gnunet.GNUNET_IDENTITY_connect(@constCast(cfg), id_cb, null);
    _ = gnunet.GNUNET_SCHEDULER_add_shutdown(do_shutdown, null);
    return;
}

pub fn main() !u8 {
    const options: [*c]const gnunet.GNUNET_GETOPT_CommandLineOption = &.{};
    const argv = std.os.argv;
    const c_ptr: [*c][*c]u8 = @ptrCast(argv.ptr);

    // Start GNUnet shenanigans (including scheduler)
    const ret = gnunet.GNUNET_PROGRAM_run(gnunet.GNUNET_OS_project_data_gnunet(), @intCast(std.os.argv.len), c_ptr, "gnunet-zig", "Gnunet zig demo", options, &run, null);
    if (ret != gnunet.GNUNET_OK)
        return 1;
    return 0;
    //try stdout.writeAll(res);
}
