const gnunet = @cImport({
    @cInclude("gnunet/gnunet_util_lib.h");
});

const std = @import("std");
const print = std.debug.print;

var id_svc: ?*gnunet.GNUNET_IDENTITY_Handle = undefined;

pub fn do_shutdown(cls: ?*anyopaque) callconv(.C) void {
    _ = cls;
    std.debug.print("Shutting down!\n", .{});
}

pub fn client_conn(cls: ?*anyopaque, c: ?*gnunet.GNUNET_SERVICE_Client, mq: ?*gnunet.GNUNET_MQ_Handle) callconv(.C) ?*anyopaque {
    _ = cls;
    _ = c;
    _ = mq;
    return null;
}

pub fn client_disco(cls: ?*anyopaque, c: ?*gnunet.GNUNET_SERVICE_Client, internal_cls: ?*anyopaque) callconv(.C) void {
    _ = cls;
    _ = c;
    _ = internal_cls;
}

pub fn run(cls: ?*anyopaque, cfg: ?*const gnunet.GNUNET_CONFIGURATION_Handle, svc: ?*gnunet.GNUNET_SERVICE_Handle) callconv(.C) void {
    // Explicit discard, C compiler usually only warns, zig errors out!
    _ = svc;
    _ = cfg;
    // Recover closure from anonymous anyopaque
    const hi: [*:0]u8 = @as([*:0]u8, @ptrCast(cls));
    std.debug.print("{s}\n", .{hi});
    // This does a lot of stuff internally, opening UDS, sending/receiving messages etc.
    _ = gnunet.GNUNET_SCHEDULER_add_shutdown(do_shutdown, null);
    return;
}

pub fn main() !u8 {
    // you are an anonymous empty object
    const argv = std.os.argv;
    const c_ptr: [*c][*c]u8 = @ptrCast(argv.ptr);
    const hi = "hihihi";

    const mh: *const gnunet.GNUNET_MQ_MessageHandler = &.{};
    const ret = gnunet.GNUNET_SERVICE_run_(gnunet.GNUNET_OS_project_data_gnunet(), @intCast(std.os.argv.len), c_ptr, "zig-example", gnunet.GNUNET_SERVICE_OPTION_NONE, &run, &client_conn, &client_disco, @constCast(hi), mh);
    if (ret != 0) {
        std.debug.print("Return val: {d}, expected: {d}\n", .{ ret, gnunet.GNUNET_OK });
    }
    return @intCast(ret);
    //try stdout.writeAll(res);
}
