#include <archive.h>
#include <getopt.h>
#include <string.h>

#include "am.h"
#include "file.h"

static void print_usage(char *argv[]);
static void print_version();

int VERBOSE = 0;

int main(int argc, char *argv[])
{
	if (argc < 2) {
		print_usage(argv);
		return 1;
	}

	if (argc == 2) {
		if (strcmp(argv[1], "--version") == 0) {
			print_version();
			return 0;
		}

		File file = { .path = argv[1] };

		if (!file_exists(&file)) {
			printf("File %s doesn't exists", file.path);
			return 1;
		}

		file_view_content(&file);
	}
	else if (argc > 2) {
		clock_t start = clock();

		int	flags = ARCHIVE_EXTRACT_TIME;
		char *path = NULL;
		char *out_dir = "./";
		int preserve_structure = 0;
		int opt;

		while ((opt = getopt(argc, argv, "x:o:pvh")) != -1) {
			switch (opt) {
				case 'x': path = optarg; break;
				case 'o': out_dir = optarg; break;
				case 'p': preserve_structure = 1; break;
				case 'v': VERBOSE = 1; break;
				case 'h': print_usage(argv); break;
			}
		}

		static struct option long_opts[] = {
			{"version", no_argument, 0, 'v'},
			{0, 0, 0, 0}
		};

		while ((opt = getopt_long(argc, argv, "hv", long_opts, NULL)) != -1) {
			switch (opt) {
				case 'v': print_version(); break;
				default:
					fprintf(stderr, "Try '%s --help' for more info.\n", argv[0]);
					return 1;
			}
		}

		// optind is for the extra arguments
		// which are not parsed
		//for(; optind < argc; optind++){
		//	printf("extra arguments: %s\n", argv[optind]);
		//}

		if (!path) {
			return 1;
		}

		if (VERBOSE) {
			printf("Archive: %s\nOutput directory: %s\nPreserve structure: %d\n",
			       path,
			       out_dir,
			       preserve_structure);
			printf("————————————————\n");
		}

		File file = { .path = path };

		if (file_extract(&file, out_dir, flags,
		                 preserve_structure, 0) == EXTRACT_OK)
		{
			clock_t end = clock();
			double duration = (double)(end - start) / CLOCKS_PER_SEC;
			if (VERBOSE) {
				printf("———————————————\n");
				printf("Extraction completed in %.2f seconds\n", duration);
			}
		}
		else {
			if (VERBOSE) {
				printf("———————————————\n");
				printf("Extraction aborted...\n");
			}
		}
	}

	return 0;
}

void print_usage(char *argv[])
{
	const char *help_text =
		"\n"
		"Example usage:\n"
		"  am archive.tar                    => lists the archive's content\n"
		"  am -x archive.tar                 => extracts archive's content to the\n"
		"                                       current working directory\n"
		"  am -x archive.tar -o ~/Downloads  => extracts archive's content to ~/Downloads\n"
		"  am -x archive.tar -p              => extracts archive's content to cwd\n"
		"                                       preserving the archives original structure.\n"
		"                                       If omitted `am` attempts to extract the files\n"
		"                                       in an archive-named directory (if necessary)\n"
		"                                       to avoid file vomit\n"
		"\n"
		"Options:\n"
		"  -h               Show this screen\n"
		"  -v               Show version information\n"
		"  -x               Extract archive to --out-dir\n"
		"  -o <directory>   Output directory (default: ./)\n"
		"  -p               Preserve archive structure\n"
		"  -v               More verbosity\n";

	print_version();

	printf("\nUsage: %s [arguments] <file> [-o <directory>] [optional-arguments]\n", argv[0]);
	printf("%s\n", help_text);
}

void print_version()
{
	printf("am version %s\n", AM_VERSION);
}
