#ifndef ARCHIVE_H
#define ARCHIVE_H

typedef struct {
	char* path;
	int is_directory;
} ArchiveEntry;

typedef enum {
	ARCHIVE_TYPE_UNKNOWN,
	ARCHIVE_TYPE_ZIP,
	ARCHIVE_TYPE_TAR
} ArchiveType;

ArchiveType detect_archive_type(const char *path);

int display_entries(const char *path,
                    ArchiveEntry **entries_out,
                    int *count_out);

void free_archive_entries(ArchiveEntry *entries, int count);

#endif
