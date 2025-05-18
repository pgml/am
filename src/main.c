#include <stdio.h>
#include "archive.h"

int main(int argc, char *argv[]) {
	if (argc < 2) {
		printf("Usage: %s <zipfile>\n", argv[0]);
		return 1;
	}

	const char *path = argv[1];
	ArchiveType type = detect_archive_type(path);

	if (type == ARCHIVE_TYPE_UNKNOWN) {
		printf("Unknown or unsupported archive type.\n");
		return 1;
	}

	ArchiveEntry *entries = NULL;
	int count = 0;

	if (!display_entries(path, &entries, &count)) {
		printf("Failed to read archive.\n");
		return 1;
	}

	printf("Contents of %s:\n", path);

	for (int i = 0; i < count; i++) {
		printf("  %s%s\n", entries[i].path, entries[i].is_directory ? "/" : "");
	}

	free_archive_entries(entries, count);
	return 0;
}
