const std = @import("std");

const c = @cImport({
	@cInclude("miniz.h");
});

const ArchiveType = enum {
	tar,
	zip,
	gzip,
};

const File = struct {
	path: []const u8,

	pub fn name(self: *File) []const u8 {
		if (self.exists()) {
			return std.fs.path.basename(self.path);
		}
		return "";
	}

	pub fn exists(self: *File) bool {
		var ex = true;
		std.fs.cwd().access(self.path, .{}) catch |err| {
			ex = if (err == error.FileNotFound) false else true;
		};
		return ex;
	}

	pub fn archiveType(self: *File) !ArchiveType {
		const file = try std.fs.cwd().openFile(self.path, .{});
		defer file.close();

		var buffer: [4]u8 = undefined;
		_ = try file.readAll(&buffer);

		if (std.mem.eql(u8, buffer[0..2], "\x1F\x8B")) {
			return ArchiveType.gzip;
		}
		else if (std.mem.eql(u8, buffer[0..4], "PK\x03\x04")) {
			return ArchiveType.zip;
		}
		else {
			return ArchiveType.tar;
		}
	}

	pub fn viewContent(self: *File) !void {
		const file = try std.fs.cwd().openFile(self.path, .{});
		defer file.close();
		const archive_type = try self.archiveType();

		switch (archive_type) {
			.zip => { try self.viewZip(); },
			.gzip => { try self.viewZip(); },
			.tar => { try self.viewZip(); },
		}
	}

	fn viewZip(self: *File) !void {
		var archive: c.mz_zip_archive = std.mem.zeroes(c.mz_zip_archive);

		const c_path = self.path.ptr;
		const ok = c.mz_zip_reader_init_file(&archive, c_path, 0);

		if (ok == 0) {
			std.debug.print("Failed to open archive `{s}`\n", .{self.path});
			return error.FailedToOpenZip;
		}

		defer _ = c.mz_zip_reader_end(&archive);

		const file_count = c.mz_zip_reader_get_num_files(&archive);
		std.debug.print("`{s}`¸ contains {} file(s):\n", .{self.name(), file_count});
	}
};

pub fn main() !void {
	const allocator = std.heap.page_allocator;
	const args = try std.process.argsAlloc(allocator);
	defer std.process.argsFree(allocator, args);

	var file = File{ .path = args[1] };

	if (!file.exists()) {
		std.debug.print("No such file `{s}`\n", .{file.path});
		return;
	}

	//const archive_type = try file.archiveType();
	//std.debug.print("{s} {}\n", .{args, archive_type});

	try file.viewContent();

	//if (args.len < 3) {
		//std.debug.print("usage: {s} <view|extract> <archive>\n", .{args[0]});
		//return;
	//}
}
