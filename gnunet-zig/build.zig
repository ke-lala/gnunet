const std = @import("std");

pub fn build(b: *std.Build) !void {
    const gnunet_prefix = b.option(
        []const u8,
        "gnunet-prefix",
        "Path to GNUnet",
    ) orelse "/usr/local";
    // FIXME parse the below with a separator?
    const additional_include_paths = b.option(
        []const u8,
        "additional-include-paths",
        "Path to additional includes",
    ) orelse "";
    const additional_lib_paths = b.option(
        []const u8,
        "additional-lib-paths",
        "Path to additional libs",
    ) orelse "";
    const target = b.standardTargetOptions(.{});
    const optimize = b.standardOptimizeOption(.{});
    const exe = b.addExecutable(.{
        .name = "gnunet-identity",
        .root_source_file = b.path("src/gnunet.zig"),
        .target = target,
        .optimize = optimize,
        .strip = false, // TODO make option
    });
    const svc_exe = b.addExecutable(.{
        .name = "gnunet-service-zig",
        .root_source_file = b.path("src/gnunet-service-zig.zig"),
        .target = target,
        .optimize = optimize,
        .strip = false, // TODO make option
    });
    // create our general purpose allocator
    var gpa = std.heap.GeneralPurposeAllocator(.{}){};

    // get an std.mem.Allocator from it
    const allocator = gpa.allocator();
    const gnunet_include_path = try std.fmt.allocPrint(allocator, "{s}/include", .{gnunet_prefix});
    const gnunet_lib_path = try std.fmt.allocPrint(allocator, "{s}/lib", .{gnunet_prefix});
    exe.addIncludePath(.{ .cwd_relative = gnunet_include_path });
    exe.addLibraryPath(.{ .cwd_relative = gnunet_lib_path });
    exe.addIncludePath(.{ .cwd_relative = additional_include_paths });
    exe.addLibraryPath(.{ .cwd_relative = additional_lib_paths });
    exe.linkSystemLibrary("gnunetutil");
    exe.linkSystemLibrary("gnunetidentity");
    b.installArtifact(exe);
    svc_exe.addIncludePath(.{ .cwd_relative = gnunet_include_path });
    svc_exe.addLibraryPath(.{ .cwd_relative = gnunet_lib_path });
    svc_exe.addIncludePath(.{ .cwd_relative = additional_include_paths });
    svc_exe.addLibraryPath(.{ .cwd_relative = additional_lib_paths });
    svc_exe.linkSystemLibrary("gnunetutil");
    b.installArtifact(svc_exe);
}
