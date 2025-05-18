const std = @import("std");

const c = @cImport({
	@cInclude("archive_reader.h");
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
		var count: c_int = 0;
		//const path_c: [*c]const u8 = self.path.ptr;
		const path_c = std.mem.trimRight(u8, self.path, "\\0");
		const path_c_ptr = &path_c[0];
		const list = c.list_zip_entries(path_c_ptr, &count);

		if (list == null) {
			std.debug.print("Failed to open archive\n", .{});
			return;
		}

		const entry_count: usize = @intCast(count);
		std.debug.print("Contents of {s}:\n", .{self.path});

		var i: usize = 0;
		while (i < entry_count) : (i += 1) {
			const entry_ptr = list[i];
			const entry_str = std.mem.span(entry_ptr);
			std.debug.print("  {d}: {s}\n", .{i, entry_str});
		}

		c.free_zip_entries(list, count);
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
