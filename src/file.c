#include <string.h>
#include <archive.h>
#include <archive_entry.h>

#include "file.h"

int file_exists(const File *f)
{
	struct stat sts;
	return stat(f->path, &sts) == 0;
}

ArchiveType file_type(const File *f)
{
	FILE *file = fopen(f->path, "rb");

	if (!file) {
		return ARCHIVE_TYPE_UNKNOWN;
	}

	unsigned char sig[4];
	fread(sig, 1, 4, file);
	fclose(file);

	if (memcmp(sig, "\x1F\x8B", 2) == 0) {
		return ARCHIVE_TYPE_GZIP;
	}
	else if (memcmp(sig, "PK\x03\x04", 4) == 0) {
		return ARCHIVE_TYPE_ZIP;
	}

	return ARCHIVE_TYPE_UNKNOWN;
}

void file_view_content(File *f)
{
	struct archive *a;
	struct archive_entry *entry;

	a = archive_read_new();
	archive_read_support_format_all(a);
	archive_read_support_filter_all(a);

	if (archive_read_open_filename(a, f->path, 10240) != ARCHIVE_OK) {
		fprintf(stderr, "Failed to open archive: %s\n", archive_error_string(a));
		archive_read_free(a);
		return;
	}

	printf(" %10s  %-s \n", "Size", "Name");
	printf(" %10s  %-s \n", "--------", "--------");

	long total_files = 0;
	long long total_size = 0;

	while (archive_read_next_header(a, &entry) == ARCHIVE_OK) {
		const char *pathname = archive_entry_pathname(entry);
		long long size = archive_entry_size(entry);

		printf(" %10lld  %-s \n", size, pathname);
		// skip file data, only listing
		archive_read_data_skip(a);

		total_files++;
		total_size += size;
	}

	printf(" %10s  %-s \n", "--------", "--------");
	printf(" %10lld  %-ld files \n", total_size, total_files);

	archive_read_close(a);
	archive_read_free(a);
}
