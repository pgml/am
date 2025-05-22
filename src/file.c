#include <archive.h>
#include <archive_entry.h>
#include <dirent.h>
#include <libgen.h>
#include <linux/limits.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>

#include "file.h"
#include "am.h"

struct FileExtract
{
	char *filename;
	char *out_path_buf;
	char *base_path;
	int need_preserve_structure;
};

static ExtractStatus   open_archives     (const File *f, struct archive **a);
static int             copy_data         (struct archive *ar, struct archive *aw);

static void            prepare_out_path  (char *out_dir);
static int             prompt_overwrite  (const char  *path,
                                          int         *force_extract,
                                          char        **new_name);
static char           *prompt_rename     ();
static char           *trim_ext          (char *filename);

char *file_name(const File *f, int trim_extension)
{
	char *path_dup = strdup(f->path);
	char *base = basename(path_dup);
	char *name = NULL;

	if (trim_extension) {
		name = trim_ext(base);
	}
	else {
		name = strdup(base);
	}

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

	if (open_archives(f, &a) != EXTRACT_OK) {
		archive_read_free(a);
		return;
	}

	printf("%10s  %-s \n", "Size", "Name");
	printf("%10s  %-s \n", "--------", "--------");

	long total_files = 0;
	size_t total_size = 0;

	while (archive_read_next_header(a, &entry) == ARCHIVE_OK) {
		const char *pathname = archive_entry_pathname(entry);
		long long size = archive_entry_size(entry);

		printf("%10lld  %-s \n", size, pathname);
		archive_read_data_skip(a);

		total_files++;
		total_size += size;
	}

	printf("%10s  %-s \n", "--------", "--------");
	printf("%10zu  %-ld files \n", total_size, total_files);

	archive_read_free(a);
}

static struct FileExtract *file_extract_new(const File *f)
{
	struct FileExtract *fp;

	fp = calloc(1, sizeof(*fp));

	if (fp == NULL) {
		return NULL;
	}

	fp->out_path_buf = calloc(1, PATH_MAX);
	fp->base_path = calloc(1, PATH_MAX);
	fp->need_preserve_structure = file_need_preserve_structure(f) > 0;

	return fp;
}

static void file_extract_paths_free(struct archive *a,
                                    struct archive *ext,
                                    struct FileExtract *fe)
{
	if (fe == NULL) {
		return;
	}

	if (fe->base_path) {
		free(fe->base_path);
	}
	if (fe->out_path_buf) {
		free(fe->out_path_buf);
		fe->out_path_buf = NULL;
	}
	if (fe->filename) {
		free(fe->filename);
	}

	free(fe);

	archive_read_free(a);
	archive_write_free(ext);
}

static void rename_out_path(struct FileExtract *fe, char *new_file)
{
	char *out_dup = strdup(fe->out_path_buf);
	char *dir = dirname(out_dup);
	char *tmp_buf = calloc(1, PATH_MAX);

	snprintf(tmp_buf, PATH_MAX, "%s/%s", dir, new_file);

	free(fe->out_path_buf);
	fe->out_path_buf = tmp_buf;
	free(out_dup);
}

ExtractStatus file_extract(const File *f,
                           char *out_dir,
                           int flags,
                           int preserve_structure,
                           int force_extract)
{
	struct archive        *a, *ext;
	struct archive_entry  *entry;
	struct FileExtract    *fe;
	int r;

	if (VERBOSE) printf("Starting extraction...\n");
	fflush(stdout);

	if (open_archives(f, &a) != EXTRACT_OK) {
		return EXTRACT_FAIL;
	}

	ext = archive_write_disk_new();
	archive_write_disk_set_options(ext, flags);

	fe = file_extract_new(f);
	prepare_out_path(out_dir);

	/*
	 * get name of the archive without extension, preallocate
	 * the final path `out_path_buf` and move `out_dir` to `base_path`
	 */
	fe->filename = file_name(f, 1);
	strncpy(fe->base_path, out_dir, PATH_MAX);
	fe->base_path[PATH_MAX-1] = '\0';

	for (;;) {
		r = archive_read_next_header(a, &entry);

		if (r == ARCHIVE_EOF) {
			break;
		}

		if (r != ARCHIVE_OK) {
			printf("%s\n", archive_error_string(a));
			return EXTRACT_FAIL;
		}

		/*
		 * If the top level directory of the archive contains several
		 * files then we change the directory we are extracting into
		 * to the filename of the archive to prevent having a bunch
		 * of lose files scattered across the current directory which
		 * annoys me greatly
		 */
		if (!preserve_structure  && !fe->need_preserve_structure) {
			snprintf(fe->base_path, PATH_MAX, "%s/", fe->filename);
		}

		/*
		 * Merge `out_dir` and a potential new archive-named parent
		 * directory with the indidivual archive files
		 */
		const char *curr_path = archive_entry_pathname(entry);
		snprintf(fe->out_path_buf, PATH_MAX, "%s%s", fe->base_path, curr_path);

		char *new_name = NULL;
		if (!force_extract) {
			/*
			 * Check if the file exists and prompt for info
			 * about what to do next
			 */
			int p;
			if ((p = prompt_overwrite(fe->out_path_buf,
			                          &force_extract,
			                          &new_name)))
			{
				switch (p) {
				case -2: // 0 doesn't work and I don't know why...
					file_extract_paths_free(a, ext, fe);
					return EXTRACT_CANCEL;
				case -1: continue;
				}
			}
		}

		/* renaming a file that would otherwise be overridden */
		if (new_name != NULL && fe->out_path_buf) {
			rename_out_path(fe, new_name);
		}

		printf("   extracting to: %s\n", fe->out_path_buf);

		archive_entry_set_pathname(entry, fe->out_path_buf);
		r = archive_write_header(ext, entry);

		if (r != ARCHIVE_OK) {
			printf("%s\n", archive_error_string(a));
		}
		else {
			r = copy_data(a, ext);
		}

		if (new_name != NULL) {
			free(new_name);
		}
	}

	file_extract_paths_free(a, ext, fe);
	return EXTRACT_OK;
}

int file_need_preserve_structure(const File *f)
{
	struct archive *a;
	struct archive_entry *entry;

	if (open_archives(f, &a) != EXTRACT_OK) {
		archive_read_free(a);
		return 0;
	}

	int result = 0;
	int has_archive_named_root = 0;
	char *filename = file_name(f, 1);
	int i = 0;

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

static ExtractStatus open_archives(const File *f, struct archive **a)
{
	*a = archive_read_new();
	archive_read_support_format_all(*a);
	archive_read_support_filter_all(*a);

	if (archive_read_open_filename(*a, f->path, 65536)) {
		printf("%s\n", archive_error_string(*a));
		return EXTRACT_FAIL;
	}

	return EXTRACT_OK;
}

/*
 * Copies the actual file content into the empty shell of a file
 * created by `archive_write_header()`
 */
static int copy_data(struct archive *ar, struct archive *aw)
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

/*
 * If out_dir is not the current than manipulate out_dir so that
 * `./` is prepended and `/` is appended to make further stuff easier
 * and it looks nicer when printing the path
 */
static void prepare_out_path(char *out_dir)
{
	char cwd_lit[] = "./";
	if (strcmp(out_dir, cwd_lit) != 0) {
		size_t len = strlen(out_dir);
		size_t cwd_len = strlen(cwd_lit);

		memmove(out_dir + cwd_len, out_dir, len + 2);
		memcpy(out_dir, cwd_lit, cwd_len);

		out_dir[cwd_len + len] = '/';
		out_dir[cwd_len + len + 1] = '\0';
	}
}

static int prompt_overwrite(const char *path,
                            int *force_extract,
                            char **new_name)
{
	FILE *file;
	if ((file = fopen(path, "r"))) {
		fclose(file);

		printf("Replace %s? [y]es, [n]o, [A]ll, [N]one, [r]ename: ", path);

		char buf[16];
		if (!fgets(buf, sizeof(buf), stdin)) {
			return -1;
		}

		if (buf[0] != '\0' && buf[strlen(buf)-1] == '\n') {
			buf[strlen(buf)-1] = '\0';
		}

		switch (buf[0]) {
			case 'N': return -2;
			case 'n': return -1;
			case 'A': *force_extract = 1; return 1;
			case 'y': return 1;
			case 'r': *new_name = prompt_rename(); return 1;
			default:
				if (*new_name == NULL) {
					printf("error: invalid response: %c\n", buf[0]);
					prompt_overwrite(path, force_extract, new_name);
				}
		}
	}

	return 1;
}

static char *prompt_rename()
{
	char *new_name = malloc(PATH_MAX);
	printf("New name: ");
	scanf("%s", new_name);
	return new_name;
}

/*
 * Removes the file extension from a file string
 */
static char *trim_ext(char *filename)
{
	char *ext = strrchr(filename, '.');
	if (!ext || ext == filename) {
		return strdup(filename);
	}

	size_t len = ext - filename;
	char *name = malloc(len + 1);

	if (!name) {
		return NULL;
	}

	strncpy(name, filename, len);
	name[len] = '\0';

	/*
	 * check for tar archives this can probably be done better
	 * but it works for now
	 */
	const char *tar = strrchr(name, '.');
	char tar_str[] = ".tar";

	if (tar != NULL && strcmp(tar_str, tar) == 0) {
		int len = strlen(name) - strlen(tar);
		char *tmp = malloc(len + 1);

		strncpy(tmp, name, len);
		tmp[len] = '\0';

		free(name);
		name = tmp;
	}

	return name;
}
