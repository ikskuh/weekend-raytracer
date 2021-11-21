const std = @import("std");

pub fn build(b: *std.build.Builder) void {
    const target = b.standardTargetOptions(.{});
    const mode = b.standardReleaseOptions();

    const zig = b.addExecutable("raytracer-zig", "src/raytracer.zig");
    zig.setTarget(target);
    zig.setBuildMode(mode);
    zig.install();

    const cpp = b.addExecutable("raytracer-cpp", null);
    cpp.addCSourceFile("src/raytracer.cpp", &[_][]const u8{
        "-std=c++17",
        "-pedantic",
        "-Wall",
        "-Wextra",
        "-Werror=return-type",
    });
    cpp.linkLibC();
    cpp.linkLibCpp();
    cpp.setTarget(target);
    cpp.setBuildMode(mode);
    cpp.install();

    const c = b.addExecutable("raytracer-c", null);
    c.addCSourceFile("src/raytracer.c", &[_][]const u8{
        "-std=c11",
        "-pedantic",
        "-Wall",
        "-Wextra",
        "-Werror=return-type",
    });
    c.linkLibC();
    c.linkLibCpp();
    c.setTarget(target);
    c.setBuildMode(mode);
    c.install();
}
