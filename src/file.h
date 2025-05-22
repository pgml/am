#ifndef FILE_H
#define FILE_H

typedef enum {
	ARCHIVE_TYPE_UNKNOWN,
	ARCHIVE_TYPE_ZIP,
	ARCHIVE_TYPE_BZIP2,
	ARCHIVE_TYPE_GZIP,
	ARCHIVE_TYPE_TAR
} ArchiveType;

typedef enum {
	EXTRACT_OK,
	EXTRACT_FAIL,
	EXTRACT_CANCEL
} ExtractStatus;

typedef struct {
	const char* path;
} File;

int file_exists(const File *f);

void file_view_content(const File *f);

ExtractStatus file_extract(const File *f,
                           char *out_dir,
                           int flags,
                           int preserve_structure,
                           int force_extract);

int file_top_level_file_count(const File *f);

/*
 * Determines whether the archive has actually a reason to
 * be preserved when `--preserve-structure` is true
 */
int file_need_preserve_structure(const File *f);

#endif
