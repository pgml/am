#ifndef FILE_H
#define FILE_H

typedef enum {
	ARCHIVE_TYPE_UNKNOWN,
	ARCHIVE_TYPE_ZIP,
	ARCHIVE_TYPE_GZIP,
	ARCHIVE_TYPE_TAR
} ArchiveType;

typedef struct {
	const char* path;
} File;

int file_exists(const File *f);
ArchiveType file_type(const File *f);
void file_view_content(File *f);

#endif
