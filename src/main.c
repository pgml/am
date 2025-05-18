#include <stdio.h>
#include "file.h"

int main(int argc, char *argv[])
{
	if (argc < 2) {
		printf("Usage: %s <file.{tar,zip,gzip}>\n", argv[0]);
		return 1;
	}

	const char *path = argv[1];
	File file = { .path = path };

	if (!file_exists(&file)) {
		printf("File %s doesn't exists", file.path);
		return 1;
	}

	file_view_content(&file);

	return 0;
}
