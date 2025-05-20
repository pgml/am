#include <libgen.h>
#include <linux/limits.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <archive.h>
#include <archive_entry.h>
#include <bzlib.h>

#include "file.h"

int copy_data(struct archive *ar, struct archive *aw);
int file_top_level_file_count(const File *f);
int file_need_preserve_structure(const File *f);

char *file_name(const File *f, int skip_extension)
{
	char *path_dup = strdup(f->path);
	char *base = basename(path_dup);

	if (skip_extension) {
		char *dot = strchr(base, '.');
		size_t len = dot ? (size_t)(dot - base) : strlen(base);
		char *name = malloc(len + 1);

		if (!name) {
			return NULL;
		}

		strncpy(name, base, len);
		name[len] = '\0';
		free(path_dup);

		return name;
	}

	char *name = strdup(base);
	free(path_dup);
	return name;
}

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
	else if (memcmp(sig, "BZh", 3) == 0) {
		return ARCHIVE_TYPE_BZIP2;
	}

	return ARCHIVE_TYPE_UNKNOWN;
}

void file_view_content(const File *f)
{
	struct archive *a;
	struct archive_entry *entry;

	a = archive_read_new();
	archive_read_support_format_all(a);
	archive_read_support_filter_all(a);

	if (archive_read_open_filename(a, f->path, 65536) != ARCHIVE_OK) {
		fprintf(stderr,
		        "Failed to open archive: %s\n",
		        archive_error_string(a));
		archive_read_free(a);
		return;
	}

	printf("%10s  %-s \n", "Size", "Name");
	printf("%10s  %-s \n", "--------", "--------");

	long total_files = 0;
	long long total_size = 0;

	while (archive_read_next_header(a, &entry) == ARCHIVE_OK) {
		const char *pathname = archive_entry_pathname(entry);
		long long size = archive_entry_size(entry);

		printf("%10lld  %-s \n", size, pathname);
		archive_read_data_skip(a);

		total_files++;
		total_size += size;
	}

	printf("%10s  %-s \n", "--------", "--------");
	printf("%10lld  %-ld files \n", total_size, total_files);

	archive_read_free(a);
}

void file_extract(const File *f,
                  char *out_dir,
                  int flags,
                  int preserve_structure)
{
	struct archive *a;
	struct archive *ext;
	struct archive_entry *entry;
	int r;

	a = archive_read_new();
	ext = archive_write_disk_new();
	archive_write_disk_set_options(ext, flags);
	archive_read_support_format_all(a);
	archive_read_support_filter_all(a);

	printf("Starting extraction...\n");
	fflush(stdout);

	if ((r = archive_read_open_filename(a, f->path, 65536))) {
		printf("%s\n", archive_error_string(a));
		return;
	}

	char *filename = file_name(f, 1);
	char out_path_buf[PATH_MAX];
	char *base_path = strdup(out_dir);

	int need_preserve_structure = 0;
	if (!preserve_structure) {
		need_preserve_structure = file_need_preserve_structure(f) > 0;
	}

	for (;;) {
		int needcr = 0;
		r = archive_read_next_header(a, &entry);

		if (r == ARCHIVE_EOF) {
			break;
		}

		if (r != ARCHIVE_OK) {
			printf("%s\n", archive_error_string(a));
			return;
		}

		/*
		 * If the top level directory of the archive contains several files
		 * then we change the directory we are extracting into to the filename
		 * of the archive to prevent having a bunch of lose files scattered
		 * across the current directory which annoys me greatly
		 */
		if (preserve_structure == 0 && !need_preserve_structure) {
			snprintf(base_path, PATH_MAX, "%s", filename);
		}

		/*
		 * Merge `out_dir` and a potential new archive-named parent directory
		 * with the indidivual archive files
		 */
		const char *curr_path = archive_entry_pathname(entry);
		snprintf(out_path_buf, PATH_MAX, "%s/%s", base_path, curr_path);

		printf("- extracting to: %s\n", out_path_buf);

		archive_entry_set_pathname(entry, out_path_buf);
		r = archive_write_header(ext, entry);

		if (r != ARCHIVE_OK) {
			printf("%s\n", archive_error_string(a));
			needcr = 1;
		}
		else {
			r = copy_data(a, ext);
			if (r != ARCHIVE_OK) {
				needcr = 1;
			}
		}

		if (needcr) {
			printf("\n");
		}
	}

	free(base_path);
	free(filename);
	archive_read_free(a);
	archive_write_free(ext);
}

/*
 * Determines whether the archive has actually a reason to
 * be preserved when `--preserve-structure` is true
 */
int file_need_preserve_structure(const File *f)
{
	struct archive *a;
	struct archive_entry *entry;

	a = archive_read_new();
	archive_read_support_format_all(a);
	archive_read_support_filter_all(a);

	if (archive_read_open_filename(a, f->path, 65536) != ARCHIVE_OK) {
		fprintf(stderr,
		        "Failed to open archive: %s\n",
		        archive_error_string(a));
		archive_read_free(a);
		return 0;
	}

	char *filename = file_name(f, 1);
	int result = 0;
	int i = 0;
	int has_archive_named_root = 0;

	/*
	 * Iterate only twice through the archive files
	 * if there's more than one file there's a potential need to
	 * extract the files as they are in archive if `--preserver-structure`
	 * is true
	 */
	while (archive_read_next_header(a, &entry) == ARCHIVE_OK && i < 2) {
		const char *path = archive_entry_pathname(entry);
		char *path_dup = strdup(path);
		char *token = strtok(path_dup, "/");

		if (strcmp(token, filename) != 0) {
			has_archive_named_root = 1;
		}

		free(path_dup);
		archive_read_data_skip(a);

		if (i == 1) {
			result = 1;
			break;
		}

		i++;
	}

	/*
	 * no need to preserve the structure of the archive if there is
	 * only one file in the archive that is named like the archive itself
	 */
	if (i == 1 && has_archive_named_root) {
		result = 0;
	}

	free(filename);
	archive_read_free(a);

	return result;
}

/*
 * Copies the actual file content into the empty shell of a file
 * created by `archive_write_header()`
 */
int copy_data(struct archive *ar, struct archive *aw)
{
	int r;
	const void *buff;
	size_t size;
	int64_t offset;

	for (;;) {
		r = archive_read_data_block(ar, &buff, &size, &offset);

		if (r == ARCHIVE_EOF) {
			return (ARCHIVE_OK);
		}

		if (r != ARCHIVE_OK) {
			printf("%s\n", archive_error_string(ar));
			return (r);
		}

		r = archive_write_data_block(aw, buff, size, offset);

		if (r != ARCHIVE_OK) {
			printf("%s\n", archive_error_string(ar));
			return (r);
		}
	}
}
