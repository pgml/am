#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <zip.h>
#include <zipconf.h>
#include "archive.h"

ArchiveType detect_archive_type(const char *path)
{
	FILE *f = fopen(path, "rb");

	if (!f) {
		return ARCHIVE_TYPE_UNKNOWN;
	}

	unsigned char sig[4];
	fread(sig, 1, 4, f);
	fclose(f);

	if (memcmp(sig, "\x1F\x8B", 2) == 0) {
		return ARCHIVE_TYPE_GZIP;
	}
	else if (memcmp(sig, "PK\x03\x04", 4) == 0) {
		return ARCHIVE_TYPE_ZIP;
	}

	return ARCHIVE_TYPE_UNKNOWN;
}

int read_zip(const char *path,
             ArchiveEntry **entries_out,
             int *count_out)
{
	int err = 0;

	zip_t *zip = zip_open(path, ZIP_RDONLY, &err);

	if (!zip) {
		return 0;
	}

	zip_int64_t count = zip_get_num_entries(zip, 0);
	ArchiveEntry *entries = malloc(sizeof(ArchiveEntry) * count);

	for (zip_uint64_t i = 0; i < count; i++) {
		const char *name = zip_get_name(zip, i, 0);
		entries[i].path = strdup(name);
		entries[i].is_directory = name[strlen(name) - 1] == '/';
	}

	*entries_out = entries;
	*count_out = (int)count;

	zip_close(zip);

	return 1;
}

// to be implemented
int read_gzip(const char *path,
              ArchiveEntry **entries_out,
              int *count_out)
{
	int err = 0;
	return err;
}

// to be implemented
int read_tar(const char *path,
             ArchiveEntry **entries_out,
             int *count_out)
{
	int err = 0;
	return err;
}


int display_archive_content(const char *path,
                            ArchiveEntry **entries_out,
                            int *count_out)
{
	int err = 0;

	ArchiveType type = detect_archive_type(path);
	ArchiveEntry *entries = malloc(sizeof(ArchiveEntry));

	switch (type) {
		case ARCHIVE_TYPE_ZIP:
			err = read_zip(path, entries_out, count_out);
		case ARCHIVE_TYPE_GZIP:
			err = read_gzip(path, entries_out, count_out);
		case ARCHIVE_TYPE_TAR:
			err = read_tar(path, entries_out, count_out);
		case ARCHIVE_TYPE_UNKNOWN:
			err = 1;
	}

	return err;
}

void free_archive_entries(ArchiveEntry *entries, int count)
{
	for (int i = 0; i < count; i++) {
		free(entries[i].path);
	}
	free(entries);
}
