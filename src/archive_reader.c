#include <stdlib.h>
#include <string.h>
#include <zip.h>
#include <zipconf.h>

char **list_zip_entries(const char *path, int *count_out) {
	zip_t *zip = zip_open(path, ZIP_RDONLY, NULL);

	if (!zip) {
		return NULL;
	}

	int entries = zip_get_num_entries(zip, 0);
	char **list = malloc(sizeof(char*) * entries);

	for (int i = 0; i < entries; i++) {
		const char *name = zip_get_name(zip, i, 0);

		if (name) {
			list[i] = strdup(name);
		}
		else {
			list[i] = strdup("<unknown>");
		}
	}

	zip_close(zip);
	*count_out = entries;

	return list;
}

void free_zip_entries(char **list, int count) {
	for (int i = 0; i < count; i++) {
		free(list[i]);
	}
	free(list);
}
