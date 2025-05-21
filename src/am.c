#include <time.h>
#include <stdio.h>
#include <archive.h>
#include <string.h>
#include <libgen.h>
#include <unistd.h>
#include <getopt.h>

#include "am.h"
#include "file.h"
#include "browser.h"

void print_version()
{
	printf("am version %s\n", AM_VERSION);
}

int main(int argc, char *argv[])
{
	if (argc < 2) {
		printf("Usage: %s <file.{tar,zip,gzip}>\n", argv[0]);
		return 1;
	}

	if (argc == 2) {
		if (strcmp(argv[1], "-v") == 0 ||
			strcmp(argv[1], "--version") == 0)
		{
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

		while ((opt = getopt(argc, argv, "b:x:o:pv")) != -1) {
			switch (opt) {
				case 'b': browser_init(); return 0; break;
				case 'x': path = optarg; break;
				case 'o': out_dir = optarg; break;
				case 'p': preserve_structure = 1; break;
				case 'v': print_version(); break;
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

		printf("path: %s\n out: %s\n preserve: %d\n", path, out_dir, preserve_structure);

		File file = { .path = path };
		file_extract(&file, out_dir, flags, preserve_structure);

		clock_t end = clock();
		double duration = (double)(end - start) / CLOCKS_PER_SEC;
		printf("———————————————\n");
		printf("Extraction completed in %.2f seconds\n", duration);
	}

	return 0;
}
