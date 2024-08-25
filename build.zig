const std = @import("std");

pub fn build(b: *std.Build) void {
    const target = b.standardTargetOptions(.{});
    const optimize = b.standardOptimizeOption(.{});
    const exe = b.addExecutable(.{
        .name = "hello",
        .target = target,
        .optimize = optimize,
    });

    const includes = [_][]const u8{
        "./external/",
    };

    for (includes) |include| {
        exe.addIncludePath(b.path(include));
    }

    const srcs = [_][]const u8{
        "src/main.cpp",
        "src/raygui.cpp",
    };

    for (srcs) |src| {
        exe.addCSourceFile(.{
            .file = b.path(src),
        });
    }

    exe.linkLibC();
    exe.linkLibCpp();

    const raylib_dep = b.dependency("raylib", .{
        .target = target,
        .optimize = optimize,
    });
    exe.linkLibrary(raylib_dep.artifact("raylib"));

    b.installArtifact(exe);

    const run_exe = b.addRunArtifact(exe);

    const run_step = b.step("run", "Run the application");
    run_step.dependOn(&run_exe.step);
}
